#include <iostream>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "Image.h"

using namespace image;

#define IMAGE_DATA_ALIGN(x, align)   (((x) + (align) - 1) & (~(align)))

Frame* image::frame_alloc()
{
    return new Frame();
}

int image::frame_free(Frame *f)
{
    delete f;
    return 0;
}

int image::fill_array(uint8_t *plane[4], int stride[4], uint8_t *data, PixelFormat fmt, int width, int height, int align)
{
    if (!plane || !stride || fmt >= PIXFMT_MAX ||
        width <= 0 || height <= 0 || align <= 0) 
    {
        std::cout << "error para" << std::endl;
        return -1;
    }

    for (auto i = 0; i < 4; ++i) {
        plane[i] = nullptr;
        stride[i] = 0;
    }

    int size = -1;

    switch (fmt) {
        case PIXFMT_YUV420P:
            stride[0] = IMAGE_DATA_ALIGN(width, align);
            stride[1] = IMAGE_DATA_ALIGN(width/2, align);
            stride[2] = IMAGE_DATA_ALIGN(width/2, align);
            if (data) {
                plane[0] = data;
                plane[1] = data + stride[0] * height;
                plane[2] = data + stride[0] * height + stride[1] * height/2;
            }
            size = stride[0] * height + stride[1] * height/2 + stride[2] * height/2;
            break;

        case PIXFMT_NV12:
            stride[0] = IMAGE_DATA_ALIGN(width, align);
            stride[1] = IMAGE_DATA_ALIGN(width, align);
            if (data) {
                plane[0] = data;
                plane[1] = data + stride[0] * height;
            }
            size = stride[0] * height + stride[1] * height/2;
            break;
        
        case PIXFMT_YUY2:
            stride[0] = IMAGE_DATA_ALIGN(width * 2, align);
            plane[0] = data;
            size = stride[0] * height;
            break;

        case PIXFMT_RGB:
            stride[0] = IMAGE_DATA_ALIGN(width * 3, align);
            plane[0] = data;
            size = stride[0] * height;
            break;

        case PIXFMT_RGBA:
            stride[0] = IMAGE_DATA_ALIGN(width * 4, align);
            plane[0] = data;
            size = stride[0] * height;
            break;

        default:
            break;
    }
    return size;
}

int image::frame_size(PixelFormat fmt, int width, int height, int align)
{
    int stride[4] = {0};
    uint8_t *plane[4] = {nullptr};
    
    return fill_array(plane, stride, nullptr, fmt, width, height, align);
}

bool image::frame_is_contiguous(Frame *f)
{
    switch (f->fmt) {    
        case PIXFMT_YUV420P:
            if (f->stride[0] == f->width   && 
                f->stride[1] == f->width/2 &&
                f->stride[2] == f->width/2 &&
                (f->plane[1] - f->plane[0]) == f->stride[0] * f->height &&
                (f->plane[2] - f->plane[1]) == f->stride[0] * f->height/2)
            {
                return true;
            }
            break;

        case PIXFMT_NV12:
            if (f->stride[0] == f->width   && 
                f->stride[1] == f->width   &&
                (f->plane[1] - f->plane[0]) == f->stride[0] * f->height)
            {
                return true;
            }
            break;

        case PIXFMT_YUY2:
            if (f->stride[0] == f->width * 2)
                return true;
            break;

        case PIXFMT_RGB:
            if (f->stride[0] == f->width * 3)
                return true;
            break;
        
        case PIXFMT_RGBA:
            if (f->stride[0] == f->width * 4)
                return true;
            break;

        default:
            std::cout << "not suport pixel format" << std::endl;
            break;
    }

    return false;
}

static bool endswith(const std::string& str, const std::string& end)
{
    int srclen = str.size();
    int endlen = end.size();
    if (srclen >= endlen) {
        std::string temp = str.substr(srclen - endlen, endlen);
        if(temp == end)
            return true;
    }

    return false;
}

FileFormat image::guess_file_format(const char *filename)
{
    std::string str = filename;

    std::for_each(str.begin(), str.end(), [](char &c) { c = ::tolower(c); });

    if (endswith(str, std::string{".jpg"}) || endswith(str, std::string{".jpeg"})) {
        return FILE_FORMAT_JPG;
    } else if (endswith(str, std::string{".png"})) {
        return FILE_FORMAT_PNG;
    } else if (endswith(str, std::string{".bmp"})) {
        return FILE_FORMAT_BMP;
    } else if (endswith(str, std::string{".mp4"})) {
        return FILE_FORMAT_MP4;
    } else {
        std::cout << "not suport file format" << std::endl;
        return FILE_FORMAT_MAX;
    }
}

int image::image_save(const char* filename, Frame *f)
{
    if (f == nullptr || f->data == nullptr || f->fmt >= PIXFMT_MAX) {
        std::cout << "invalid frame" << std::endl;
        return -1;
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        std::cout << "fopen failed" << std::endl;
        return -1;
    }

    uint8_t* ptr;

    switch (f->fmt) {    
        case PIXFMT_YUV420P: {
            //write Y
            ptr = f->plane[0];
            for (int i = 0; i < f->height; ++i) {
                fwrite(ptr, f->width, 1, fp);
                ptr += f->stride[0];
            }
            //write U
            ptr = f->plane[1];
            for (int i = 0; i < f->height / 2; ++i) {
                fwrite(ptr, f->width/2, 1, fp);
                ptr += f->stride[1];
            }
            //write V
            ptr = f->plane[2];
            for (int i = 0; i < f->height / 2; ++i) {
                fwrite(ptr, f->width/2, 1, fp);
                ptr += f->stride[2];
            }
        }
        break;

        case PIXFMT_NV12: {
            //write Y
            ptr = f->plane[0];
            for (int i = 0; i < f->height; ++i) {
                fwrite(ptr, f->width, 1, fp);
                ptr += f->stride[0];
            }
            //write UV
            ptr = f->plane[1];
            for (int i = 0; i < f->height / 2; ++i) {
                fwrite(ptr, f->width, 1, fp);
                ptr += f->stride[1];
            }
        }
        break;

        case PIXFMT_YUY2: {
            //write YUV
            ptr = f->plane[0];
            for (int i = 0; i < f->height; ++i) {
                fwrite(ptr, f->width*2, 1, fp);
                ptr += f->stride[0];
            }
        }
        break;

        case PIXFMT_RGB: {
            //write RGB
            ptr = f->plane[0];
            for (int i = 0; i < f->height; ++i) {
                fwrite(ptr, f->width*3, 1, fp);
                ptr += f->stride[0];
            }
        }
        break;

        case PIXFMT_RGBA: {
            //write RGBA
            ptr = f->plane[0];
            for (int i = 0; i < f->height; ++i) {
                fwrite(ptr, f->width*4, 1, fp);
                ptr += f->stride[0];
            }
        }
        break;

        default:
            break;
    }

    fclose(fp);
    return 0;
}

int image::image_copy(uint8_t *dst_plane[4], int dst_stride[4],
                   const uint8_t *src_plane[4], const int src_stride[4],
                   PixelFormat fmt, int width, int height)
{
    if (dst_plane == nullptr || src_plane == nullptr || fmt >= PIXFMT_MAX) {
        std::cout << "invalid para" << std::endl;
        return -1;
    }

    const uint8_t* src_ptr;
    uint8_t* dst_ptr;

    switch (fmt) {
        case PIXFMT_YUV420P: {
            //write Y
            src_ptr = src_plane[0];
            dst_ptr = dst_plane[0];
            for (int i = 0; i < height; ++i) {
                memcpy(dst_ptr, src_ptr, width);
                src_ptr += src_stride[0];
                dst_ptr += dst_stride[0];
            }
            //write U
            src_ptr = src_plane[1];
            dst_ptr = dst_plane[1];
            for (int i = 0; i < height / 2; ++i) {
                memcpy(dst_ptr, src_ptr, width/2);
                src_ptr += src_stride[1];
                dst_ptr += dst_stride[1];
            }
            //write V
            src_ptr = src_plane[2];
            dst_ptr = dst_plane[2];
            for (int i = 0; i < height / 2; ++i) {
                memcpy(dst_ptr, src_ptr, width/2);
                src_ptr += src_stride[2];
                dst_ptr += dst_stride[2];
            }
        }
        break;

        case PIXFMT_NV12: {
            //write Y
            src_ptr = src_plane[0];
            dst_ptr = dst_plane[0];
            for (int i = 0; i < height; ++i) {
                memcpy(dst_ptr, src_ptr, width);
                src_ptr += src_stride[0];
                dst_ptr += dst_stride[0];
            }
            //write UV
            src_ptr = src_plane[1];
            dst_ptr = dst_plane[1];
            for (int i = 0; i < height / 2; ++i) {
                memcpy(dst_ptr, src_ptr, width);
                src_ptr += src_stride[1];
                dst_ptr += dst_stride[1];
            }
        }
        break;

        case PIXFMT_YUY2: {
            //write YUV
            src_ptr = src_plane[0];
            dst_ptr = dst_plane[0];
            for (int i = 0; i < height; ++i) {
                memcpy(dst_ptr, src_ptr, width*2);
                src_ptr += src_stride[0];
                dst_ptr += dst_stride[0];
            }
        }
        break;

        case PIXFMT_RGB: {
            //write RGB
            src_ptr = src_plane[0];
            dst_ptr = dst_plane[0];
            for (int i = 0; i < height; ++i) {
                memcpy(dst_ptr, src_ptr, width*3);
                src_ptr += src_stride[0];
                dst_ptr += dst_stride[0];
            }
        }
        break;

        case PIXFMT_RGBA: {
            //write RGBA
            src_ptr = src_plane[0];
            dst_ptr = dst_plane[0];
            for (int i = 0; i < height; ++i) {
                memcpy(dst_ptr, src_ptr, width*4);
                src_ptr += src_stride[0];
                dst_ptr += dst_stride[0];
            }
        }
        break;

        default:
            break;
    }

    return 0;
}

Frame* image::image_alloc(PixelFormat fmt, int width, int height, int align)
{
    if (fmt >= PIXFMT_MAX) {
        std::cout << "not support pixel format" << std::endl;
        return nullptr;
    }

    Frame *f = frame_alloc();
    f->fmt = fmt;
    f->width = width;
    f->height = height;
    f->data = new uint8_t[frame_size(fmt, width, height, align)];

    fill_array(f->plane, f->stride, f->data, fmt, width, height, align);
    return f;
}

int image::image_free(Frame *f)
{
    if (!f)
        return -1;

    delete f->data;
    frame_free(f);
    return 0;
}
