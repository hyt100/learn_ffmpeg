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
#include <functional>

struct VideoDemuxer
{
    AVFormatContext *fmt_ctx;
    AVStream *video_stream;
    AVBSFContext *bsf_ctx;
    AVPacket *pkt1;
    AVPacket *pkt2;
    int video_stream_idx;
    int width;
    int height;
    int video_frame_count;
    std::function<void(AVPacket *pkt)> func;
};

int VideoDemuxerDestroy(VideoDemuxer *demux)
{
    avformat_close_input(&demux->fmt_ctx);
    av_bsf_free(&demux->bsf_ctx);
    av_packet_free(&demux->pkt1);
    av_packet_free(&demux->pkt2);
    delete demux;
    return 0;
}

VideoDemuxer *VideoDemuxerCreate(const char *filename)
{
    AVFormatContext *fmt_ctx = NULL;
    AVStream *video_stream = NULL;
    AVBSFContext *bsf_ctx = NULL;
    const AVBitStreamFilter *bsfilter;
    int video_stream_idx;
    VideoDemuxer *demux = new VideoDemuxer();

    int ret = 0;

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        printf("Could not open source file %s\n", filename);
        goto FAIL;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("Could not find stream information\n");
        goto FAIL;
    }

    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (ret < 0) {
        printf("Could not find video stream in input file \n");
        goto FAIL;
    }
    video_stream_idx = ret;
    video_stream = fmt_ctx->streams[video_stream_idx];

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, filename, 0);

    bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
    av_bsf_alloc(bsfilter, &bsf_ctx);
    avcodec_parameters_copy(bsf_ctx->par_in, video_stream->codecpar);
    av_bsf_init(bsf_ctx);

    demux->fmt_ctx = fmt_ctx;
    demux->video_stream = video_stream;
    demux->video_stream_idx = video_stream_idx;
    demux->bsf_ctx = bsf_ctx;
    demux->pkt1 = av_packet_alloc();
    demux->pkt2 = av_packet_alloc();
    return demux;
    
FAIL:
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    if (bsf_ctx) {
        av_bsf_free(&bsf_ctx);
    }
    delete demux;
    return nullptr;
}

static int bsf_process(VideoDemuxer *demux, AVPacket *pkt)
{
    av_bsf_send_packet(demux->bsf_ctx, demux->pkt1);

    while (av_bsf_receive_packet(demux->bsf_ctx, demux->pkt2) == 0)
    {
        demux->func(demux->pkt2);
        av_packet_unref(demux->pkt2);
    }

    return 0;
}

int VideoDemuxerRead(VideoDemuxer *demux, std::function<void(AVPacket *pkt)> func)
{
    demux->func = func;

    /* read frames from the file */
    while (av_read_frame(demux->fmt_ctx, demux->pkt1) >= 0) {
        
        bsf_process(demux, demux->pkt1);

        av_packet_unref(demux->pkt1);
    }

    // flush
    bsf_process(demux, NULL);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
#include "h264.h"

struct FrameHeader {
    int64_t pts;
    int seq;
    int type;
    int size;
};

int main(int argc, char *argv[])
{
    VideoDemuxer *demux = VideoDemuxerCreate("../res/640x360.mp4");
    if (!demux) {
        printf("VideoDemuxerCreate failed \n");
        return -1;
    }
    FILE *fp = fopen("out.h264", "wb");
    if (!fp) {
        printf("fopen failed \n");
        VideoDemuxerDestroy(demux);
        return -1;
    }

    int seq = 0;

    VideoDemuxerRead(demux, 
        [&](AVPacket *pkt) {
            FrameHeader header;
            header.pts = pkt->pts;
            header.seq = ++seq;
            header.type = (h264_get_frame_type(pkt->data, pkt->size) == BLOCK_FLAG_TYPE_I) ? 1 : 0;
            header.size = pkt->size;

            printf("got a packet: pts=%ld seq=%d  type=%d\n", pkt->pts, header.seq, header.type);
            fwrite(&header, sizeof(header), 1, fp);
            fwrite(pkt->data, pkt->size, 1, fp);
        }
    );

    fclose(fp);

    VideoDemuxerDestroy(demux);
}