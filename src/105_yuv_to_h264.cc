#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>
}
#include <iostream>
#include "Image.h"
#include "ImageFF.h"

// ffplay 640x360.h264


enum VideoCodecResult {
    VIDEOCODEC_RET_NORMAL = 0,
    VIDEOCODEC_RET_EOF,
    VIDEOCODEC_RET_WAIT,
    VIDEOCODEC_RET_ERROR
};

typedef void (*EncodeCallback)(VideoCodecResult result, AVPacket *pkt, void *ctx);

struct VideoEncoder {
    AVCodecContext *codec_ctx;
    AVPacket *pkt;
    AVFrame *frame;
    const AVCodec *codec;
    EncodeCallback callback;
    void *ctx;
    int width;
    int height;
    int yuv_size;
};

VideoEncoder* VideoEncoderCreate(const char *codec_name, 
        int width, int height, EncodeCallback callback, void *ctx)
{
    int ret;
    VideoEncoder *encoder = new VideoEncoder;
    encoder->width = width;
    encoder->height = height;
    encoder->callback = callback;
    encoder->ctx = ctx;
    encoder->yuv_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);

    /* find the mpeg1video encoder */
    encoder->codec = avcodec_find_encoder_by_name(codec_name);
    if (!encoder->codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        goto FAIL;
    }

    encoder->codec_ctx = avcodec_alloc_context3(encoder->codec);
    if (!encoder->codec_ctx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        goto FAIL;
    }

    encoder->pkt = av_packet_alloc();

    /* put sample parameters */
    encoder->codec_ctx->bit_rate = 400000;
    /* resolution must be a multiple of two */
    encoder->codec_ctx->width = width;
    encoder->codec_ctx->height = height;
    /* frames per second */
    encoder->codec_ctx->time_base = (AVRational){1, 30};
    encoder->codec_ctx->framerate = (AVRational){30, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    encoder->codec_ctx->gop_size = 10;
    encoder->codec_ctx->max_b_frames = 1;
    encoder->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (encoder->codec->id == AV_CODEC_ID_H264)
        av_opt_set(encoder->codec_ctx->priv_data, "preset", "slow", 0);

    /* open it */
    ret = avcodec_open2(encoder->codec_ctx, encoder->codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec\n");
        goto FAIL;
    }

    encoder->frame = av_frame_alloc();
    if (!encoder->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        goto FAIL;
    }
    encoder->frame->pts = 0;
    encoder->frame->format = encoder->codec_ctx->pix_fmt;
    encoder->frame->width  = encoder->codec_ctx->width;
    encoder->frame->height = encoder->codec_ctx->height;

    ret = av_frame_get_buffer(encoder->frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        goto FAIL;
    }

    return encoder;

FAIL:
    if (encoder->codec_ctx)
        avcodec_free_context(&encoder->codec_ctx);
    if (encoder->frame)
        av_frame_free(&encoder->frame);
    if (encoder->pkt)
        av_packet_free(&encoder->pkt);
    return NULL;
}

int VideoEncoderDestroy(VideoEncoder *encoder)
{
    if (encoder->codec_ctx)
        avcodec_free_context(&encoder->codec_ctx);
    if (encoder->frame)
        av_frame_free(&encoder->frame);
    if (encoder->pkt)
        av_packet_free(&encoder->pkt);
    return 0;
}

int VideoEncoderEncodeDoing(VideoEncoder *encoder, AVFrame *frame)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %ld\n", frame->pts);

    ret = avcodec_send_frame(encoder->codec_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        return -1;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder->codec_ctx, encoder->pkt);
        if (ret == AVERROR(EAGAIN)) {
            encoder->callback(VIDEOCODEC_RET_WAIT, NULL, encoder->ctx);
            return 0;
        } else if (ret == AVERROR_EOF) {
            encoder->callback(VIDEOCODEC_RET_EOF, NULL, encoder->ctx);
            return 0;
        } else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            return -1;
        }

        // printf("Write packet %ld (size=%5d)\n", encoder->pkt->pts, encoder->pkt->size);
        encoder->callback(VIDEOCODEC_RET_NORMAL, encoder->pkt, encoder->ctx);
        av_packet_unref(encoder->pkt);
    }

    return 0;
}

// support yuv420p
int VideoEncoderEncode(VideoEncoder *encoder, uint8_t *buf, int size)
{
    /* flush the encoder */
    if (!buf) {
        return VideoEncoderEncodeDoing(encoder, NULL);
    }

    if (size != encoder->yuv_size) {
        fprintf(stderr, "size error. \n");
        return -1;
    }

    int ret = av_frame_make_writable(encoder->frame);
    if (ret < 0) {
        fprintf(stderr, "frame make writable failed. \n");
        return -1;
    }

    encoder->frame->pts ++;

    // copy yuv420p data
    // memcpy(encoder->frame->data[0], buf, encoder->width * encoder->height);
    // memcpy(encoder->frame->data[1], buf + encoder->width * encoder->height, encoder->width * encoder->height / 4);
    // memcpy(encoder->frame->data[2], buf + encoder->width * encoder->height * 5/4, encoder->width * encoder->height / 4);

    AVFrame *frame = encoder->frame;
    int width = encoder->width;
    int height = encoder->height;
    uint8_t *dst;
    uint8_t *src;

    // copy Y
    dst = frame->data[0];
    src = buf;
    for (int i = 0; i < height; ++i) {
        memcpy(dst, src, width);
        dst += frame->linesize[0];
        src += width;
    }
    // copy U
    dst = frame->data[1];
    src = buf + width * height;
    for (int i = 0; i < height/2; ++i) {
        memcpy(dst, src, width/2);
        dst += frame->linesize[1];
        src += width/2;
    }
    // copy V
    dst = frame->data[2];
    src = buf + width * height * 5/4;
    for (int i = 0; i < height/2; ++i) {
        memcpy(dst, src, width/2);
        dst += frame->linesize[2];
        src += width/2;
    }

    return VideoEncoderEncodeDoing(encoder, encoder->frame);
}

///////////////////////////////////////////////////////////////////////////

void encode_callback(VideoCodecResult result, AVPacket *pkt, void *ctx)
{
    FILE *out = (FILE *)ctx;

    if (result == VIDEOCODEC_RET_EOF) {
        printf("result eof...\n");
    } else if (result == VIDEOCODEC_RET_WAIT) {
        // printf("result wait...\n");
    } else if (result == VIDEOCODEC_RET_NORMAL) {
        imageff::ff_save_packet(out, pkt);
    }
}


int main(int argc, char **argv)
{
    FILE *fp = fopen("../res/video_640x360.yuv420p", "rb");
    if (!fp) {
        fprintf(stderr, "Could not open file\n");
        exit(1);
    }
    FILE *fp_out = fopen("640x360.h264", "wb");

    int width = 640;
    int height = 360;
    int yuv_size = width * height * 3/2;

    //codec名字查找： codec_list.c中的codec_list数组
    VideoEncoder *encoder = VideoEncoderCreate("libx264", width, height, encode_callback, (void *)fp_out);
    if (!encoder) {
        fclose(fp);
        fclose(fp_out);
        return -1;
    }

    printf("ready ...\n");
    uint8_t *inbuf = new uint8_t[yuv_size];

    while (!feof(fp)) {
        /* read raw data from the input file */
        int size = fread(inbuf, 1, yuv_size, fp);
        if (size != yuv_size)
            break;

        if (VideoEncoderEncode(encoder, inbuf, yuv_size) < 0) {
            fprintf(stderr, "decode error.\n");
            exit(1);
        }
    }

    printf("flush...\n");

    /* flush the decoder */
    VideoEncoderEncode(encoder, NULL, 0);

    VideoEncoderDestroy(encoder);
    fclose(fp);
    fclose(fp_out);

    return 0;
}