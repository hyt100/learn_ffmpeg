#include "ImageFF.h"
#include <iostream>

using std::cout;
using std::endl;
using namespace imageff;

int imageff::ff_save_frame(const char *filename, AVFrame *frame)
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

int imageff::ff_save_frame(const char *filename, AVPacket *pkt)
{
    if (pkt->size <= 0) {
        cout << "ff_save_frame failed (size is 0)" << endl;
        return -1;
    }
    FILE *fp = fopen(filename, "wb");
    fwrite(pkt->data, 1, pkt->size, fp);
    fclose(fp);
    return 0;
}



