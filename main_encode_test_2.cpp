extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/timestamp.h>

#include <libavdevice/avdevice.h>
}


//std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

//opencv
#include <opencv2/opencv.hpp>


static void encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt, FILE* outfile) {
    int ret;
    if (frame) {
        std::cout<<"Send frame "<<frame->pts<<std::endl;
    }
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }
        std::cout<<"Write packet "<<pkt->pts<<" (size="<<pkt->size<<")"<<std::endl;
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

int main(int argc, char** argv)
{
    const char* filename, * codec_name;
    const AVCodec* codec;
    AVCodecContext* c = NULL;
    int i, ret, x, y;
    int FPS = 30;
    FILE* f;
    AVFrame* frame;
    AVPacket* pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    std::cout<<"Hello encode test"<<std::endl;

    filename = "test2.mp4"; //argv[1];

    /* find the mpeg1video encoder */
    //codec = avcodec_find_encoder_by_name("mpeg1video");
    //if (!codec) {
    //    fprintf(stderr, "Codec '%s' not found\n", codec_name);
    //    exit(1);
    //}
    /* end of find the mpeg1video encoder */

    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
    pkt = av_packet_alloc();
    if (!pkt) exit(1);

    /* put sample parameters */
    c->bit_rate = 400000;
    c->width = 960;
    c->height = 540;
    c->time_base = { 1, FPS };
    c->framerate = { FPS, 1 };
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 10;   //Todo
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec->id == AV_CODEC_ID_H264) av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        std::cout<<"Could not open codec: "<<ret<<std::endl;
        exit(1);
    }
    /*Prepare output file*/
    const char* outfilename = "out.mp4";
    f = fopen(outfilename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", outfilename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    /* the image can be allocated by any means and av_image_alloc() is
    	 * just the most convenient way if av_malloc() is to be used */
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    /* encode 1 second of video */
    for (i = 0; i < FPS; i++) {
        fflush(stdout);
        /* Make sure the frame data is writable.
           On the first round, the frame is fresh from av_frame_get_buffer()
           and therefore we know it is writable.
           But on the next rounds, encode() will have called
           avcodec_send_frame(), and the codec may have kept a reference to
           the frame in its internal structures, that makes the frame
           unwritable.
           av_frame_make_writable() checks that and allocates a new buffer
           for the frame only if necessary.
         */
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);

        /* Prepare a dummy image.
           In real code, this is where you would have your own logic for
           filling the frame. FFmpeg does not care what you put in the
           frame.
         */
         /* Y */
        for (y = 0; y < c->height; y++) {
            for (x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }

        /* Cb and Cr */
        for (y = 0; y < c->height / 2; y++) {
            for (x = 0; x < c->width / 2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        frame->pts = i;

        /* encode the image */
        encode(c, frame, pkt, f);
    }

    /* flush the encoder */
    encode(c, NULL, pkt, f);

    /* Add sequence end code to have a real MPEG file.
       It makes only sense because this tiny examples writes packets
       directly. This is called "elementary stream" and only works for some
       codecs. To create a valid file, you usually need to write packets
       into a proper file format or protocol; see mux.c.
     */
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}