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



int image_encode(const char *filename, AVFrame* pFrame)
{
    int width = pFrame->width;
    int height = pFrame->height;
 
    // 分配AVFormatContext对象
    AVFormatContext* pFormatCtx = avformat_alloc_context();
 
    // 设置输出文件格式
    pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
 
    // 创建并初始化一个和该url相关的AVIOContext
    if( avio_open(&pFormatCtx->pb, filename, AVIO_FLAG_READ_WRITE) < 0)
    {
        printf("Couldn't open output file.");
        return -1;
    }
 
    // 构建一个新stream
    AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
    if( pAVStream == NULL )
    {
        return -1;
    }
 
    // 设置该stream的信息
    AVCodecContext* pCodecCtx = pAVStream->codec;
 
    pCodecCtx->codec_id   = pFormatCtx->oformat->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt    = AV_PIX_FMT_YUVJ420P;
    pCodecCtx->width      = width;
    pCodecCtx->height     = height;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
 
    //打印输出相关信息
    av_dump_format(pFormatCtx, 0, filename, 1);
 
    //================================== 查找编码器 ==================================//
    AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if( !pCodec )
    {
        printf("Codec not found.");
        return -1;
    }
 
    // 设置pCodecCtx的解码器为pCodec
    if( avcodec_open2(pCodecCtx, pCodec, NULL) < 0 )
    {
        printf("Could not open codec.");
        return -1;
    }
 
    //================================Write Header ===============================//
    avformat_write_header(pFormatCtx, NULL);
 
    int y_size = pCodecCtx->width * pCodecCtx->height;
 
    //==================================== 编码 ==================================//
    // 给AVPacket分配足够大的空间
    AVPacket pkt;
    av_new_packet(&pkt, y_size * 3);
 
    //
    int got_picture = 0;
    int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
    if( ret < 0 )
    {
        printf("Encode Error.\n");
        return -1;
    }
    if( got_picture == 1 )
    {
        pkt.stream_index = pAVStream->index;
        ret = av_write_frame(pFormatCtx, &pkt);
    }
 
    av_free_packet(&pkt);
 
    //Write Trailer
    av_write_trailer(pFormatCtx);
 
 
    if( pAVStream )
    {
        avcodec_close(pAVStream->codec);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
 
    return 0;
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

        // copy yuv420p data
        memcpy(frame->data[0], buf, width * height);
        memcpy(frame->data[1], buf + width * height, width * height / 4);
        memcpy(frame->data[2], buf + width * height * 5/4, width * height / 4);

        ret = image_encode(filename, frame);
        if (ret < 0) {
            fprintf(stderr, "image_encode failed. \n");
            break;
        }

    } while (0);

    av_frame_free(&frame);
    return ret;
}