#include <SDL2/SDL.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <array>
#include "BlockingQueue.h"
#include "Image.h"

using std::cout;
using std::endl;
using namespace image;

// 说明： 目前只支持YUY2格式渲染 (YUY2 也称 YUYV422)

struct video_framebuf 
{
    void *start;
    int len;

    video_framebuf()
        :start(MAP_FAILED), len(0)
    {}
} ;

#define MY_FLUSH_EVT   (SDL_USEREVENT + 1)
#define VIDEO_FRAMEBUF_CNT     20
#define CAMERA_DEV     "/dev/video0"

static bool thread_exit = false;
static int width = 640;
static int height = 480;
static int screen_width = width;
static int screen_height = height;
static int video_fd = -1;
static std::array<video_framebuf, VIDEO_FRAMEBUF_CNT> framebuf;
static BlockingQueue<Frame *> frameQueue;

static int open_stream()
{
    // open camera device
    video_fd = open(CAMERA_DEV, O_RDWR);
    if (video_fd < 0) {
        perror("camera open failed");
        return -1;
    }

    // get camera capability
    struct v4l2_capability cap;
    if (ioctl(video_fd, VIDIOC_QUERYCAP, &cap) == -1) {
        perror("ioctl VIDIOC_QUERYCAP failed");
        return -1;
    }

    // set format
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(video_fd, VIDIOC_S_FMT, &fmt) == -1) {
        perror("ioctl VIDIOC_S_FMT failed");
        return -1;
    }
    if (ioctl(video_fd, VIDIOC_G_FMT, &fmt) == -1) {
        perror("ioctl VIDIOC_G_FMT failed");
        return -1;
    }

    // set stream parameter
    struct v4l2_streamparm parm;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = 30;
    parm.parm.capture.capturemode = 0;
    if (ioctl(video_fd, VIDIOC_S_PARM, &parm) == -1) {
        perror("ioctl VIDIOC_S_PARM failed");
        return -1;
    }

    // request memory
    struct v4l2_requestbuffers reqbuf;
    reqbuf.count = VIDEO_FRAMEBUF_CNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(video_fd, VIDIOC_REQBUFS, &reqbuf) == -1) {
        perror("ioctl VIDIOC_REQBUFS failed");
        return -1;
    }
    for (int i = 0; i < VIDEO_FRAMEBUF_CNT; ++i) {
        struct v4l2_buffer buf;

        // query buffer
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("ioctl VIDIOC_QUERYBUF failed");
            return -1;
        }

        // mmap buffer
        framebuf[i].len = buf.length;
        framebuf[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buf.m.offset);
        if (framebuf[i].start == MAP_FAILED) {
            perror("mmap failed");
            return -1;
        }

        //buffer queue
        if (ioctl(video_fd, VIDIOC_QBUF, &buf) == -1) {
            perror("ioctl VIDIOC_QBUF failed");
            return -1;
        }
    }

    return 0;
}

static int close_stream()
{
    if (video_fd >= 0)
        close(video_fd);
    for (int i = 0; i < VIDEO_FRAMEBUF_CNT; ++i) {
        if (framebuf[i].start != MAP_FAILED) {
            munmap(framebuf[i].start, framebuf[i].len);
        }
    }
    return 0;
}

static int update(void *data)
{
    (void)data;

    struct v4l2_buffer buf;
    SDL_Event event;
    fd_set rfds;
    struct timeval tv;

    // start capture
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &type) == -1) {
        perror("ioctl VIDIOC_STREAMON failed");
        return -1;
    }

    while (!thread_exit) {
        FD_ZERO(&rfds);
        FD_SET(video_fd, &rfds);
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        int ret = select(video_fd+1, &rfds, NULL, NULL, &tv);
        if (ret == -1) {
            perror("select error");
            break;
        } else if (ret == 0) {
            cout << "select timeout" << endl;
            break;
        }

        // 出队
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(video_fd, VIDIOC_DQBUF, &buf) == -1) {
            perror("ioctl VIDIOC_DQBUF failed");
            break;
        }
        int index = buf.index;

        // 拷贝数据
        Frame *frame = image_alloc_with_data(PIXFMT_YUY2, width, height, (uint8_t *)framebuf[index].start, framebuf[index].len);
        frameQueue.put(frame);

        // 发送事件，通知渲染
        event.type = MY_FLUSH_EVT;
        SDL_PushEvent(&event);
    
        // 重新入队
        if (ioctl(video_fd, VIDIOC_QBUF, &buf) == -1) {
            perror("ioctl VIDIOC_QBUF failed");
            break;
        }
    }

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

    while (1) {
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
            Frame *frame = nullptr;

            if (frameQueue.take(&frame, 100)) {
                SDL_UpdateTexture(texture, NULL, frame->plane[0], frame->stride[0]);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);

                image_free(frame);
            }
        }
    }

    thread_exit = 1;
    SDL_WaitThread(tid, NULL);

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
