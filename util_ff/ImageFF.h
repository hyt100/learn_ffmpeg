#pragma once

#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>
}

namespace imageff {


int ff_save_frame(const char *filename, AVFrame *frame);
int ff_save_frame(const char *filename, AVPacket *pkt);


}