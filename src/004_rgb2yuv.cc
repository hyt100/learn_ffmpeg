#include <iostream>
#include <assert.h>
#include <string.h>
#include "Image.h"

// ffmpeg -i lena_512x512.jpg -f rawvideo -pix_fmt rgb24 lena_512x512.rgb24
// ffplay -f rawvideo -pixel_format nv12 -video_size 512x512 lena_512x512.nv12

using std::cout;
using std::endl;
using namespace image;

static const char *filename = "../res/lena_512x512.rgb24";
static const int width  = 512;
static const int height = 512;

static void rgb24_to_nv12(Frame *nv12, Frame *rgb24)
{
    int R, G, B, Y, U, V;
    uint8_t y, u, v;

    uint8_t *src_ptr = rgb24->plane[0];
    uint8_t *y_ptr = nv12->plane[0];
    uint8_t *uv_ptr = nv12->plane[1];
    int padding = nv12->stride[0] - nv12->width;

    for (int i = 0; i < rgb24->height; ++i) {
        for (int j = 0; j < rgb24->width; ++j) {
            int start = j * 3;

            R = src_ptr[start + 0];
            G = src_ptr[start + 1];
            B = src_ptr[start + 2];

            // RGB to YUV
            Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
            U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
            V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

            y = (uint8_t)((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
            u = (uint8_t)((U < 0) ? 0 : ((U > 255) ? 255 : U));
            v = (uint8_t)((V < 0) ? 0 : ((V > 255) ? 255 : V));

            *y_ptr++  = y;
            if (i%2 == 0 && j%2 == 0) {
                *uv_ptr++ = u;
                *uv_ptr++ = v;
            }
        }
        src_ptr += rgb24->stride[0];

        if (i%2 == 0) {
            y_ptr  += padding;
            uv_ptr += padding;
        }
    }
}

int main(int argc, char *argv[])
{
    Frame *f1 = image_read(filename, PIXFMT_RGB, width, height, 1);
    Frame *f2 = image_alloc(PIXFMT_NV12, width, height, 1);
    assert(f1 != nullptr);
    assert(f2 != nullptr);

    rgb24_to_nv12(f2, f1);

    image_save("lena_512x512.nv12", f2);
    image_free(f1);
    image_free(f2);
    return 0;
}