#pragma once
#include <stdint.h>
#include <libavcodec/avcodec.h>

int image_encode(const char *filename, AVFrame *frame);
int image_encode(const char *filename, uint8_t *buf, int size, int width, int height, int pix_fmt);

