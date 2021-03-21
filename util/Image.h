#pragma once

#include <stdint.h>

namespace image {

enum FileFormat {
    FILE_FORMAT_JPG = 0,
    FILE_FORMAT_PNG,
    FILE_FORMAT_BMP,
    FILE_FORMAT_MP4,
    FILE_FORMAT_MAX
};

enum PixelFormat {
    PIXFMT_YUV420P = 0,
    PIXFMT_NV12,
    PIXFMT_YUY2,   //alias YUYV422
    PIXFMT_RGB,    //alias RGB24
    PIXFMT_RGBA,
    PIXFMT_MAX
};

struct Frame
{
    PixelFormat fmt;
    int width;
    int height;
    int stride[4];
    uint8_t *plane[4];
    uint8_t *data;

    Frame()
        :fmt(PIXFMT_MAX), width(0), height(0), data(nullptr)
    {
        for (auto i = 0; i < 4; ++i) {
            stride[i] = 0;
            plane[i] = nullptr;
        }
    }
    ~Frame() {}
};


FileFormat guess_file_format(const char *filename);
int fill_array(uint8_t *plane[4], int stride[4], uint8_t *data, PixelFormat fmt, int width, int height, int align);

int image_size(PixelFormat fmt, int width, int height, int align);
bool image_is_contiguous(Frame *f);

// The allocated Frame has to be freed by using image_free.
Frame* image_alloc(PixelFormat fmt, int width, int height, int align);
// The allocated Frame has to be freed by using image_free.
Frame* image_alloc_with_data(PixelFormat fmt, int width, int height, uint8_t *data, int len);
// The allocated Frame has to be freed by using image_free.
Frame* image_read(const char *filename, PixelFormat fmt, int width, int height, int align);

int image_free(Frame *f);

int image_save(const char* filename, Frame *f);
int image_copy(uint8_t *dst_plane[4], int dst_stride[4],
               uint8_t *src_plane[4], int src_stride[4],
               PixelFormat fmt, int width, int height);
int image_copy(Frame *dst, Frame *src);

}