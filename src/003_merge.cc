#include <iostream>
#include <assert.h>
#include <string.h>
#include "Image.h"

// ffplay -f rawvideo -pixel_format nv12 -video_size 1024x512 merge.nv12

using std::cout;
using std::endl;
using namespace image;

static const char *filename = "../res/lena_512x512.nv12";
static const int width  = 512;
static const int height = 512;

int main(int argc, char *argv[])
{
    Frame *f1 = image_read(filename, PIXFMT_NV12, width, height, 1);
    Frame *f2 = image_alloc(PIXFMT_NV12, width*2, height, 1);
    assert(f1 != nullptr);
    assert(f2 != nullptr);

    uint8_t *src_ptr, *dst_ptr;

    // copy Y
    src_ptr = f1->plane[0];
    dst_ptr = f2->plane[0];
    for (int i = 0; i < f1->height; ++i) {
        memcpy(dst_ptr, src_ptr, width);
        memcpy(dst_ptr + width, src_ptr, width);
        dst_ptr += f2->stride[0];
        src_ptr += f1->stride[0];
    }

    // copy UV
    src_ptr = f1->plane[1];
    dst_ptr = f2->plane[1];
    for (int i = 0; i < f1->height/2; ++i) {
        memcpy(dst_ptr, src_ptr, width);
        memcpy(dst_ptr + width, src_ptr, width);
        dst_ptr += f2->stride[1];
        src_ptr += f1->stride[1];
    }

    image_save("merge.nv12", f2);
    image_free(f1);
    image_free(f2);
    return 0;
}