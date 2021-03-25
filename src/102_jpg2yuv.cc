#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdlib.h>
}

int save_frame(const char *filename, AVFrame *frame)
{
    /* allocate image where the decoded image will be put */
    uint8_t *video_dst_data[4] = {NULL};
    int      video_dst_linesize[4];
    int video_dst_bufsize = av_image_alloc(video_dst_data, video_dst_linesize,
                             frame->width, frame->height, (enum AVPixelFormat)frame->format, 1);

    /* copy decoded frame to destination buffer:
     * this is required since rawvideo expects non aligned data */
    av_image_copy(video_dst_data, video_dst_linesize,
                  (const uint8_t **)(frame->data), frame->linesize,
                  (enum AVPixelFormat)frame->format, frame->width, frame->height);

    /* write to rawvideo file */
    FILE *fp = fopen(filename, "wb");
    fwrite(video_dst_data[0], 1, video_dst_bufsize, fp);
    fclose(fp);

    /* free image */
    av_free(video_dst_data[0]);
    return 0;
}

int main(int argc, char *argv[])
{
    const char *src_filename = "../resources/xiyang.jpeg";
    const char *dst_filename = "out.yuv";

    AVFormatContext *fmt_ctx = NULL;

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);

    int stream_num = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (stream_num < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n", av_get_media_type_string(AVMEDIA_TYPE_VIDEO), src_filename);
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
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);

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

    /* initialize packet, set data to NULL, let the demuxer fill it */
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    /* read frames from the file */
    if (av_read_frame(fmt_ctx, &pkt) != 0) {
        fprintf(stderr, "Failed to read frame\n");
        exit(1);
    }

    // submit the packet to the decoder
    if (avcodec_send_packet(codec_ctx, &pkt) != 0) {
        fprintf(stderr, "Failed to send packet\n");
        exit(1);
    }

    // get all the available frames from the decoder
    AVFrame *frame = av_frame_alloc();
    if (avcodec_receive_frame(codec_ctx, frame) < 0) {
        fprintf(stderr, "Failed to get frame\n");
        exit(1);
    }


    printf("decode ok: %dx%d  %s \n", frame->width, frame->height, av_get_pix_fmt_name((enum AVPixelFormat)frame->format));
    save_frame(dst_filename, frame);

    ////////////////////////////////////


    av_frame_unref(frame);

    av_packet_unref(&pkt);

    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return 0;
}