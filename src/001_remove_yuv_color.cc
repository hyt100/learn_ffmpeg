#include <iostream>
#include "Image.h"

// ffplay -f rawvideo -pixel_format nv12 -video_size 650x850 650x850.nv12

using std::cout;
using std::endl;
using namespace image;

void nv12_remove_color(const char *in_file, const char *out_file, int width, int height)
{
    Frame *f = image_read(in_file, PIXFMT_NV12, width, height, 1);
    if (!f)
        return;
    
    uint8_t *ptr = f->plane[1];
    for (auto i = 0; i < f->height/2; ++i) {
        for (auto j = 0; j < f->width; ++j) {
            ptr[j] = 0x80;
        }
        ptr += f->stride[1];
    }

    image_save(out_file, f);
    image_free(f);
}

int main(int argc, char *argv[])
{
    nv12_remove_color("../res/650x850.nv12", "../res/650x850_gray.nv12", 650, 850);
    return 0;
}