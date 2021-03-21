#include <iostream>
#include <assert.h>
#include <string.h>
#include "Image.h"

// ffplay -f rawvideo -pixel_format nv12 -video_size 512x512 merge.nv12

using std::cout;
using std::endl;
using namespace image;

static const char *filename1 = "../res/lena_512x512.nv12";
static const char *filename2 = "../res/lena_128x128.nv12";
static const int width1  = 512;
static const int height1 = 512;
static const int width2  = 128;
static const int height2 = 128;

int main(int argc, char *argv[])
{
    Frame *f1 = image_read(filename1, PIXFMT_NV12, width1, height1, 1);
    Frame *f2 = image_read(filename2, PIXFMT_NV12, width2, height2, 1);
    Frame *f3 = image_alloc(PIXFMT_NV12, width1, height1, 1);
    assert(f1 != nullptr);
    assert(f2 != nullptr);
    assert(f3 != nullptr);

    image_copy(f3, f1);

    uint8_t *src_ptr, *dst_ptr;

    // copy Y
    src_ptr = f2->plane[0];
    dst_ptr = f3->plane[0];
    for (int i = 0; i < f2->height; ++i) {
        memcpy(dst_ptr, src_ptr, f2->width);
        dst_ptr += f3->stride[0];
        src_ptr += f2->stride[0];
    }

    // copy UV
    src_ptr = f2->plane[1];
    dst_ptr = f3->plane[1];
    for (int i = 0; i < f2->height/2; ++i) {
        memcpy(dst_ptr, src_ptr, f2->width);
        dst_ptr += f3->stride[1];
        src_ptr += f2->stride[1];
    }

    image_save("merge.nv12", f3);
    image_free(f1);
    image_free(f2);
    image_free(f3);
    return 0;
}