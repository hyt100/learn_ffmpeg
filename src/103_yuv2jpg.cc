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
    FILE *fp = fopen("../res/720x720.yuv", "rb");
    if (!fp) {
        printf("fopen failed. \n");
        return -1;
    }
    int width = 720;
    int height = 720;
    int size = width * height * 3/2;
    uint8_t *data = new uint8_t[size];

    int ret = fread(data, 1, size, fp);
    if (ret != size) {
        printf("fread error\n");
        delete[] data;
        return -1;
    }

    image_encode("my.jpg", data, size, width, height, AV_PIX_FMT_YUV420P);

    fclose(fp);
    delete[] data;
    return 0;
}