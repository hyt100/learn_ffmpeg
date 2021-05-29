#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
}
#include <iostream>


static int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                       AVStream *st, AVFrame *frame)
{
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame to the encoder \n");
        exit(1);
    }

    while (ret >= 0) {
        AVPacket pkt = { 0 };

        ret = avcodec_receive_packet(c, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding a frame\n");
            return -1;
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(&pkt, c->time_base, st->time_base);
        pkt.stream_index = st->index;

        /* Write the compressed frame to the media file. */
        ret = av_interleaved_write_frame(fmt_ctx, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) {
            fprintf(stderr, "Error while writing output packet\n");
            return -1;
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}

int image_encode(const char *filename, AVFrame *frame)
{
    AVOutputFormat *fmt = NULL;
    AVFormatContext *oc = NULL;
    AVCodec *codec = NULL;
    AVCodecContext *codec_ctx = NULL;
    AVStream *st = NULL;
    int ret;
    AVDictionary *opt = NULL;

    int width = frame->width;
    int height = frame->height;

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("Could not deduce output format from file extension\n");
        goto FAIL;
    }
    fmt = oc->oformat;

    /* find the encoder */
    codec = avcodec_find_encoder(fmt->video_codec);
    if (!codec) {
        fprintf(stderr, "Could not find encoder \n");
        goto FAIL;
    }

    st = avformat_new_stream(oc, NULL);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        goto FAIL;
    }
    st->id = oc->nb_streams-1;

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        goto FAIL;
    }

    codec_ctx->codec_id = oc->oformat->video_codec;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->time_base = (AVRational){1, 25};

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* open the codec */
    ret = avcodec_open2(codec_ctx, codec, &opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec\n");
        goto FAIL;
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(st->codecpar, codec_ctx);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        goto FAIL;
    }

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
            goto FAIL;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto FAIL;
    }

    ret = write_frame(oc, codec_ctx, st, frame);
    if (ret < 0) {
        printf("encode failed\n");
        goto FAIL;
    }

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    avcodec_free_context(&codec_ctx);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);

    if (opt) {
        av_dict_free(&opt);
    }

    return 0;

FAIL:
    if (codec_ctx)
        avcodec_free_context(&codec_ctx);
    if (oc)
        avformat_free_context(oc);
    if (opt)
        av_dict_free(&opt);
    return -1;
}

int image_encode(const char *filename, uint8_t *buf, int size, int width, int height, int pix_fmt)
{
    if (pix_fmt != AV_PIX_FMT_YUV420P) {
        fprintf(stderr, "only support YUV420P \n");
        return -1;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "alloc frame failed \n");
        return -1;
    }
    frame->pts = 0;
    frame->format = pix_fmt;
    frame->width  = width;
    frame->height = height;

    int ret = -1;

    do {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate the video frame data\n");
            break;
        }

        ret = av_frame_make_writable(frame);
        if (ret < 0) {
            fprintf(stderr, "frame make writable failed. \n");
            break;
        }

        //Fixed: 如果是按照1字节对齐，才可以按照如下方式进行块拷贝；否则要按照行拷贝。
        // 块拷贝
        // memcpy(frame->data[0], buf, width * height);
        // memcpy(frame->data[1], buf + width * height, width * height / 4);
        // memcpy(frame->data[2], buf + width * height * 5/4, width * height / 4);

        // 行拷贝
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

        ret = image_encode(filename, frame);
        if (ret < 0) {
            fprintf(stderr, "image_encode failed. \n");
            break;
        }

    } while (0);

    av_frame_free(&frame);
    return ret;
}