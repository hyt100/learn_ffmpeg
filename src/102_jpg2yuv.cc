#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>
}
#include "ImageFF.h"

// ffplay -f rawvideo -pixel_format yuv420p -video_size 512x512 lena_512x512.yuv420p


int image_decode(const char *filename, AVFrame *frame)
{
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVStream *st;
    AVCodec *codec;
    int stream_num;
    int ret = -1;

    /* initialize packet, set data to NULL, let the demuxer fill it */
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", filename);
        goto END;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto END;
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, filename, 0);

    stream_num = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (stream_num < 0) {
        fprintf(stderr, "Could not find %s stream\n", av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        goto END;
    }
    st = fmt_ctx->streams[stream_num];

    /* find decoder for the stream */
    codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "Failed to find codec\n");
        goto END;
    }

    /* Allocate a codec context for the decoder */
    codec_ctx = avcodec_alloc_context3(codec);

    /* Copy codec parameters from input stream to output codec context */
    if (avcodec_parameters_to_context(codec_ctx, st->codecpar) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
        goto END;
    }

    /* Init the decoders */
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        goto END;
    }

    /* read frames from the file */
    if (av_read_frame(fmt_ctx, &pkt) != 0) {
        fprintf(stderr, "Failed to read frame\n");
        goto END;
    }

    // submit the packet to the decoder
    if (avcodec_send_packet(codec_ctx, &pkt) != 0) {
        fprintf(stderr, "Failed to send packet\n");
        goto END;
    }

    // get all the available frames from the decoder
    
    if (avcodec_receive_frame(codec_ctx, frame) < 0) {
        fprintf(stderr, "Failed to get frame\n");
        goto END;
    }

    ret = 0;

END:
    av_packet_unref(&pkt);

    if (codec_ctx)
        avcodec_free_context(&codec_ctx);
    if (fmt_ctx)
        avformat_close_input(&fmt_ctx);
    return ret;
}

int main(int argc, char *argv[])
{
    const char *src_filename = "../res/lena_512x512.jpg";
    const char *dst_filename = "lena_512x512.yuv420p";

    AVFrame *frame = av_frame_alloc();
    if (image_decode(src_filename, frame) == 0) {
        printf("decode ok: %dx%d  %s \n", frame->width, frame->height, av_get_pix_fmt_name((enum AVPixelFormat)frame->format));
        imageff::ff_save_frame(dst_filename, frame);
    }
    av_frame_free(&frame);
}