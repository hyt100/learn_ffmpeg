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
#include "Common.h"

//本程序配置摄像头输出为YUY2

using std::cout;
using std::endl;


#define MY_FLUSH_EVT   (SDL_USEREVENT + 1)
#define CAMERA_DEV     "/dev/video0"

static bool thread_exit = false;
static int width = 640;
static int height = 480;
static int screen_width = width;
static int screen_height = height;
static BlockingQueue<AVPacket *> frameQueue;
static AVFormatContext *fmt_ctx = NULL;

static int open_stream()
{
    avdevice_register_all();

    AVInputFormat *input_fmt = av_find_input_format("v4l2");
    if (!input_fmt) {
        fprintf(stderr, "Could not find v4l2\n");
        exit(1);
    }

    AVDictionary *dict = NULL;
    av_dict_set(&dict, "input_format", "yuyv422", 0); //这里配置为YUY2 (alias yuyv422)
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
    return 0;
}

static int close_stream()
{
    avformat_close_input(&fmt_ctx);
    return 0;
}

static int update(void *data)
{
    (void)data;
    SDL_Event event;

    // int64_t start_time = get_cur_time();

    while (!thread_exit) {
        AVPacket *pkt = av_packet_alloc();

        /* read frames: av_read_frame读取摄像头数据时会阻塞，无需在循环中加延时 */
        if (av_read_frame(fmt_ctx, pkt) != 0) {
            fprintf(stderr, "Failed to read frame\n");
            break;
        }

        // cout << "size: " << pkt->size << endl;

        // cout << "time: " << get_cur_time() - start_time << endl;

        frameQueue.put(pkt);

        // 发送事件，通知渲染
        event.type = MY_FLUSH_EVT;
        SDL_PushEvent(&event);
    }

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
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, width, height);

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
            AVPacket *pkt = NULL;
            if (frameQueue.take(&pkt, 0)) {
                SDL_UpdateTexture(texture, NULL, pkt->data, width*2);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                av_packet_free(&pkt);
            }
        }
    }

    thread_exit = 1;
    SDL_WaitThread(tid, NULL);

    // clear buffer
    while (1) {
        AVPacket *pkt = NULL;
        if (frameQueue.take(&pkt, 0)) {
            av_packet_free(&pkt);
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
