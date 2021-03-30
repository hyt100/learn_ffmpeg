#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>
}
#include <iostream>
#include "File.h"
#include "ImageEncode.h"

int main(int argc, char *argv[])
{
    FileReader f("../res/video_640x360.yuv420p");
    if (f.is_error()) {
        printf("read file failed\n");
        exit(1);
    }

    image_encode("my.jpg", f.data(), f.size(), 640, 360, AV_PIX_FMT_YUV420P);
    return 0;
}