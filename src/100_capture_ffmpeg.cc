#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
}
#include <SDL2/SDL.h>
#include <iostream>
#include "BlockingQueue.h"

//启用则配置摄像头输出YUY2，否则配置输出为MJPEG
#define CAMERA_CONFIG_YUY2     1

using std::cout;
using std::endl;


#define MY_FLUSH_EVT   (SDL_USEREVENT + 1)
#define CAMERA_DEV     "/dev/video0"

static bool thread_exit = false;
static int width = 640;
static int height = 480;
static int screen_width = width;
static int screen_height = height;
static BlockingQueue<AVFrame *> frameQueue;
static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *codec_ctx = NULL;

static int open_stream()
{
    avdevice_register_all();

    AVInputFormat *input_fmt = av_find_input_format("v4l2");
    if (!input_fmt) {
        fprintf(stderr, "Could not find v4l2\n");
        exit(1);
    }

    AVDictionary *dict = NULL;
    av_dict_set(&dict, "input_format", "mjpeg", 0);
    av_dict_set(&dict, "video_size", "640x480", 0);
    av_dict_set(&dict, "framerate", "30", 0);

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, CAMERA_DEV, input_fmt, &dict) < 0) {
        fprintf(stderr, "Could not open source file %s\n", CAMERA_DEV);
        exit(1);
    }
    av_dict_free(&dict);

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, CAMERA_DEV, 0);

    int stream_num = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (stream_num < 0) {
        fprintf(stderr, "Could not find %s stream in input file \n", av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        exit(1);
    }
    AVStream *st = fmt_ctx->streams[stream_num];

    /* find decoder for the stream */
    AVCodec *codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "Failed to find codec\n");
        exit(1);
    }

    /* Allocate a codec context for the decoder */
    codec_ctx = avcodec_alloc_context3(codec);

    /* Copy codec parameters from input stream to output codec context */
    if (avcodec_parameters_to_context(codec_ctx, st->codecpar) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
        exit(1);
    }

    /* Init the decoders */
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        exit(1);
    }
    return 0;
}

static int close_stream()
{
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return 0;
}

static int update(void *data)
{
    (void)data;
    SDL_Event event;
    /* initialize packet, set data to NULL, let the demuxer fill it */
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    AVFrame *frame = av_frame_alloc();

    while (!thread_exit) {
        /* read frames from the file */
        if (av_read_frame(fmt_ctx, &pkt) != 0) {
            fprintf(stderr, "Failed to read frame\n");
            break;
        }

        avcodec_send_packet(codec_ctx, &pkt);

        av_packet_unref(&pkt);

        if (avcodec_receive_frame(codec_ctx, frame) < 0) {
            fprintf(stderr, "Failed to get frame\n");
            break;
        }

        AVFrame *temp = av_frame_alloc();
        av_frame_move_ref(temp, frame);

        frameQueue.put(temp);

        // 发送事件，通知渲染
        event.type = MY_FLUSH_EVT;
        SDL_PushEvent(&event);

        usleep(5*1000);
    }

    av_frame_free(&frame);

    thread_exit = true;
    cout << "update exit ..." << endl;
    return 0;
}

int main(int argc, char *argv[])
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
    SDL_Thread *tid;

    if (open_stream() != 0) {
        close_stream();
        return -1;
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);
    window = SDL_CreateWindow("capture", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); 
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    tid = SDL_CreateThread(update, "update", NULL);

    while (!thread_exit) {
        SDL_Event event;
        int ret = SDL_WaitEvent(&event);
        if (!ret) {
            cout << "wait error" << endl;
            break;
        }

        if (event.type == SDL_QUIT) {
            cout << "got event: QUIT" << endl;
            break;
        }
        else if (event.type == MY_FLUSH_EVT) {
            AVFrame *frame = NULL;
            if (frameQueue.take(&frame, 0)) {
                SDL_UpdateYUVTexture(texture, NULL, 
                        frame->data[0], frame->linesize[0], 
                        frame->data[1], frame->linesize[1], 
                        frame->data[2], frame->linesize[2]);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                av_frame_free(&frame);
            }
        }
    }

    thread_exit = 1;
    SDL_WaitThread(tid, NULL);

    // clear buffer
    while (1) {
        AVFrame *frame = NULL;
        if (frameQueue.take(&frame, 0)) {
            av_frame_free(&frame);
        } else {
            break;
        }
    }

OUT:
    if (texture)
        SDL_DestroyTexture(texture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
    close_stream();
    return 0;
}
