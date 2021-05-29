// libyuv linux编译：  (CMakeLists.txt中去掉JPEG_FOUND，不编译libjpeg相关代码)
//      cmake .. -DCMAKE_INSTALL_PREFIX=out/ -DCMAKE_BUILD_TYPE=Release
//      make; make install

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "libyuv.h"
#include "File.h"

namespace yuv_until {
// degree: 90,180,270
void rotate_yuv420p(uint8_t *yuv, uint8_t *rotate_yuv, int width, int height, int degree)
{
    libyuv::RotationModeEnum mode;

    if (degree == 90)
        mode = libyuv::kRotate90;
    else if (degree == 180)
        mode = libyuv::kRotate180;
    else if (degree == 270)
        mode = libyuv::kRotate270;
    else
        return;
    
    int y_size = width * height;
    int u_size = width * height / 4;

    uint8_t* src_y = yuv;
    uint8_t* src_u = yuv + y_size;
    uint8_t* src_v = yuv + y_size + u_size;

    uint8_t* dst_y = rotate_yuv;
    uint8_t* dst_u = rotate_yuv + y_size;
    uint8_t* dst_v = rotate_yuv + y_size + u_size;
    
    if (degree == 90 || degree == 270) {
        libyuv::I420Rotate(src_y, width,
                           src_u, width/2,
                           src_v, width/2,
                           dst_y, height,
                           dst_u, height/2,
                           dst_v, height/2,
                           width, height, mode);
    } else {
        libyuv::I420Rotate(src_y, width,
                           src_u, width/2,
                           src_v, width/2,
                           dst_y, width,
                           dst_u, width/2,
                           dst_v, width/2,
                           width, width, mode);
    }
}
}


int main(int argc, char *argv[])
{
    FileReader file("../res/lena_128x128.yuv");
    if (file.is_error())
        return -1;

    FILE *fp = fopen("out.yuv", "wb");
    uint8_t *rotate_yuv = new uint8_t[file.size()];

    yuv_until::rotate_yuv420p(file.data(), rotate_yuv, 128, 128, 270);

    fwrite(rotate_yuv, file.size(), 1, fp);
    fclose(fp);
    delete[] rotate_yuv;
    return 0;
}