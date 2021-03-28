#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>
}
#include <iostream>
#include "ImageFF.h"

// ffmpeg -i 640x360.mp4 -codec copy -bsf: h264_mp4toannexb -f h264 640x360.h264
// ffplay 640x360.h264
// ffplay -f rawvideo -pixel_format yuv420p -video_size 640x360 640x360.yuv

#define INBUF_SIZE 4096

typedef void (*DecodeCallback)(int result, AVFrame *frame, void *ctx);

struct VideoDecoder {
    AVCodecParserContext *parser_ctx;
    AVCodecContext *codec_ctx;
    AVPacket *pkt;
    AVFrame *frame;
    const AVCodec *codec;
    DecodeCallback callback;
    void *ctx;
};

VideoDecoder *VideoDecoderCreate(enum AVCodecID id, DecodeCallback callback, void *ctx)
{
    VideoDecoder *decoder = new VideoDecoder;
    decoder->callback = callback;
    decoder->ctx = ctx;

    decoder->pkt = av_packet_alloc();
    if (!decoder->pkt) {
        fprintf(stderr, "alloc packet failed\n");
        goto FAIL;
    }

    decoder->frame = av_frame_alloc();
    if (!decoder->frame) {
        fprintf(stderr, "alloc frame failed\n");
        goto FAIL;
    }
    
    decoder->codec = avcodec_find_decoder(id);
    if (!decoder->codec) {
        fprintf(stderr, "Codec not found\n");
        goto FAIL;
    }

    decoder->parser_ctx = av_parser_init(decoder->codec->id);
    if (!decoder->parser_ctx) {
        fprintf(stderr, "parser not found\n");
        goto FAIL;
    }

    decoder->codec_ctx = avcodec_alloc_context3(decoder->codec);
    if (!decoder->codec_ctx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        goto FAIL;
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(decoder->codec_ctx, decoder->codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        goto FAIL;
    }

    return decoder;

FAIL:
    if (decoder->pkt)
        av_packet_free(&decoder->pkt);
    if (decoder->frame)
        av_frame_free(&decoder->frame);
    if (decoder->codec_ctx)
        avcodec_free_context(&decoder->codec_ctx);
    if (decoder->parser_ctx)
        av_parser_close(decoder->parser_ctx);
    delete decoder;
    return NULL;
}

int VideoDecoderDestroy(VideoDecoder *decoder)
{
    if (decoder->pkt)
        av_packet_free(&decoder->pkt);
    if (decoder->frame)
        av_frame_free(&decoder->frame);
    if (decoder->codec_ctx)
        avcodec_free_context(&decoder->codec_ctx);
    if (decoder->parser_ctx)
        av_parser_close(decoder->parser_ctx);
    delete decoder;
    return 0;
}

// It willl flush the decoder when pkt is NULL
int VideoDecoderDecodeDoing(VideoDecoder *decoder, AVPacket *pkt)
{
    int ret;

    ret = avcodec_send_packet(decoder->codec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        return -1;
    }

    while (ret >= 0) {
        //avcodec_receive_frame内部会先调用av_frame_unref(frame)
        ret = avcodec_receive_frame(decoder->codec_ctx, decoder->frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            decoder->callback(ret, NULL, decoder->ctx);
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return -1;
        }

        // printf("Decode frame %3d\n", decoder->codec_ctx->frame_number);
        decoder->callback(0, decoder->frame, decoder->ctx);

        // av_frame_unref(decoder->frame);  //可以不需要
    }
    return 0;
}

int VideoDecoderDecode(VideoDecoder *decoder, uint8_t *buf, int len)
{
    /* flush the decoder */
    if (!buf) {
        return VideoDecoderDecodeDoing(decoder, NULL);
    }

    uint8_t *p = buf;
    int left = len;

    /* use the parser to split the data into decoder->pkt */
    while (left > 0) {
        // 从ffmpeg源码的h264_parse函数中可知，poutbuf不需用户区释放
        // av_parser_init() -> parser_list -> ff_h264_parser -> h264_parse()
        int ret = av_parser_parse2(decoder->parser_ctx, decoder->codec_ctx, 
                    &decoder->pkt->data, &decoder->pkt->size, 
                    p, left,
                    AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            fprintf(stderr, "Error while parsing\n");
            return -1;
        }

        p += ret;
        left -= ret;

        if (decoder->pkt->size) {
            // printf("ret = %d  size = %d \n", ret, decoder->pkt->size);
            printf("pic type: %c     keyframe:%d    %dx%d    \n", 
                    av_get_picture_type_char((enum AVPictureType)decoder->parser_ctx->pict_type), 
                    decoder->parser_ctx->key_frame,
                    decoder->parser_ctx->width, decoder->parser_ctx->height);

            if (VideoDecoderDecodeDoing(decoder, decoder->pkt) < 0) {
                fprintf(stderr, "Error while do decode \n");
                return -1;
            }
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////
// static const char *src_file = "../res/640x360.h264";
static const char *src_file = "../res/bad/640x360.h264";
static const char *dst_file = "640x360.yuv";

void decode_callback(int result, AVFrame *frame, void *ctx)
{
    // printf("callback...\n");
    if (result == AVERROR(EAGAIN)) {
        // printf("decode wait...\n");
    } else if (result == AVERROR_EOF) {
        printf("decode eof...\n");
    } else {
        // printf("decode ok: %dx%d  %s \n", frame->width, frame->height, av_get_pix_fmt_name((enum AVPixelFormat)frame->format));
        imageff::ff_save_frame((FILE *)ctx, frame);
    }
}

int main(int argc, char **argv)
{
    uint8_t *inbuf = new uint8_t[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    FILE *fp = fopen(src_file, "rb");
    if (!fp) {
        fprintf(stderr, "Could not open file\n");
        return -1;
    }
    FILE *fp_out = fopen(dst_file, "wb");

    VideoDecoder *decoder = VideoDecoderCreate(AV_CODEC_ID_H264, decode_callback, (void *)fp_out);
    if (!decoder) {
        fprintf(stderr, "Could not create decoder.\n");
        fclose(fp);
        fclose(fp_out);
        return -1;
    }

    printf("ready ...\n");

    while (!feof(fp)) {
        /* read raw data from the input file */
        int size = fread(inbuf, 1, INBUF_SIZE, fp);
        if (!size)
            break;

        if (VideoDecoderDecode(decoder, inbuf, size) < 0) {
            fprintf(stderr, "decode error.\n");
            exit(1);
        }
    }

    printf("flush...\n");

    /* flush the decoder */
    VideoDecoderDecode(decoder, NULL, 0);

    VideoDecoderDestroy(decoder);
    fclose(fp);
    fclose(fp_out);
    delete inbuf;
    return 0;
}
