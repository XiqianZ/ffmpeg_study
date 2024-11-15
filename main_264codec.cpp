//ffmpeg
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/timestamp.h>

#include <libavdevice/avdevice.h>
}


#include<dshow.h>

//std
#include<vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

//opencv
#include <opencv2/opencv.hpp>

int ret = 0;

//void prepareH264Decoder() {
//	const AVCodec* p264Codec = avcodec_find_decoder(AV_CODEC_ID_H264);
//    AVFormatContext* formatContext = avformat_alloc_context();
//    if (avformat_open_input(&formatContext, "input.h264", nullptr, nullptr) != 0) {
//        std::cerr << "Error: Could not open video file" << std::endl;
//        return;
//    }
//    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
//        std::cerr << "Error: Could not find stream information" << std::endl;
//        avformat_close_input(&formatContext);
//        return;
//    }
//    // Find the first video stream
//    int videoStreamIndex = -1;
//    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
//        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
//            videoStreamIndex = i;
//            break;
//        }
//    }
//    if (videoStreamIndex == -1) {
//        std::cerr << "Error: Could not find video stream" << std::endl;
//        avformat_close_input(&formatContext);
//        return;
//    }
//    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
//    const AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
//    if (!codec) {
//        std::cerr << "Error: Unsupported codec" << std::endl;
//        avformat_close_input(&formatContext);
//        return;
//    }
//}


void displayFrame(AVFrame* frame, AVCodecContext* codecContext) {
    SwsContext* swsCtx = sws_getContext(
        codecContext->width, codecContext->height, codecContext->pix_fmt,
        codecContext->width, codecContext->height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    cv::Mat frameBGR(codecContext->height, codecContext->width, CV_8UC3);
    uint8_t* data[1] = { frameBGR.data };
    int linesize[1] = { static_cast<int>(frameBGR.step[0]) };

    sws_scale(swsCtx, frame->data, frame->linesize, 0, codecContext->height, data, linesize);
    //std::cout<<"linesize: "<<frame->linesize[0]<<" | "<< frame->linesize[1]<<" | "<<frame->linesize[2] 
    //    <<" | "<<frame->linesize[3]<<" | "<<frame->linesize[4]<<" | "<<frame->linesize[5]
    //    <<" | "<<frame->linesize[6]<<" | "<<frame->linesize[7] <<std::endl;
        
    std::cout << "linesize frameBGR: " << static_cast<int>(frameBGR.step[0]) << std::endl;

    cv::imshow("Video H264", frameBGR);
    cv::pollKey();

    sws_freeContext(swsCtx);
}



int main() {
    std::cout<<"Hello [1], [2] [3] [4] FFmpeg!" << std::endl;

    // Open the default camera using OpenCV
    cv::VideoCapture cap(0);
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 960);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 540);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        return -1;
    }

    // Set up FFmpeg codec context for H.264
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Error: Could not find H.264 codec" << std::endl;
        return -1;
    }

    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Error: Could not allocate codec context" << std::endl;
        return -1;
    }

    codecContext->bit_rate = 400000;
    codecContext->width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    codecContext->height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    codecContext->time_base = { 1, 30 };
    codecContext->framerate = { 30, 1 };
    codecContext->gop_size = 1;
    codecContext->max_b_frames = 1;
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    //codecContext->pix_fmt = AV_PIX_FMT_YUYV422;

    CoUninitialize();
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "Error: Could not open codec" << std::endl;
        avcodec_free_context(&codecContext);
        return -1;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Error: Could not allocate frame" << std::endl;
        avcodec_free_context(&codecContext);
        return -1;
    }
    frame->format = codecContext->pix_fmt;
    frame->width = codecContext->width;
    frame->height = codecContext->height;

    if (av_frame_get_buffer(frame, 0) < 0) {
        std::cerr << "Error: Could not allocate frame buffer" << std::endl;
        av_frame_free(&frame);
        avcodec_free_context(&codecContext);
        return -1;
    }

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        std::cerr << "Error: Could not allocate packet" << std::endl;
        av_frame_free(&frame);
        avcodec_free_context(&codecContext);
        return -1;
    }

    SwsContext* swsCtx = sws_getContext(
        codecContext->width, codecContext->height, AV_PIX_FMT_BGR24,
        codecContext->width, codecContext->height, codecContext->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx) {
        std::cerr << "Error: Could not initialize sws context" << std::endl;
        av_packet_free(&pkt);
        av_frame_free(&frame);
        avcodec_free_context(&codecContext);
        return -1;
    }

    //Test
    const AVCodec* pH264Codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    AVCodecContext* pH264CodecContext = avcodec_alloc_context3(pH264Codec);
    if (!pH264CodecContext) {
		std::cerr << "Error: Could not allocate codec context" << std::endl;
		return -1;
	}
    pH264CodecContext->bit_rate = 400000;
    pH264CodecContext->width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    pH264CodecContext->height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    pH264CodecContext->time_base = { 1, 30 };
    pH264CodecContext->framerate = { 30, 1 };
    pH264CodecContext->gop_size = 1;
    pH264CodecContext->max_b_frames = 1;
    pH264CodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    if (avcodec_open2(pH264CodecContext, pH264Codec, nullptr) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        avcodec_free_context(&pH264CodecContext);
        return -1;
    }
    //Test




    cv::Mat frameBGR;
    int64_t pts = 0;
    while (true) {
        cap >> frameBGR;
        cv::imshow("Origin", frameBGR);
        cv::waitKey(1);
        if (frameBGR.empty()) break;
        int size_in_KB = frameBGR.total() * frameBGR.elemSize() / 1024;
        std::cout<<"Frame size: "<<frameBGR.size() <<" | Total Size : "<<size_in_KB <<"KB" <<std::endl;

        const int stride[] = { static_cast<int>(frameBGR.step[0]) };
        sws_scale(swsCtx, &frameBGR.data, stride, 0, codecContext->height, frame->data, frame->linesize);

        //frame->pts += av_rescale_q(1, codecContext->time_base, codecContext->time_base);
        frame->pts = pts++;
        frame->key_frame = 1;
        std::cout<<"Frame pts: "<<frame->pts<<std::endl;

        if (avcodec_send_frame(codecContext, frame) < 0) {
            std::cerr << "Error: Could not send frame" << std::endl;
            break;
        }

        //if (avcodec_receive_packet(codecContext, pkt) < 0) {
        //    std::cout << "Packet size: " << pkt->size/1024 << "KB | Frame Size: "<<frame->pkt_size<<std::endl;
        //}
        int loop_count = 0;
        while (avcodec_receive_packet(codecContext, pkt) == 0) {
            std::cout << "Packet size: " << pkt->size/1024 <<" Loop: "<< ++loop_count<<std::endl;
		}
			

        //std::cout << "Framd data: " << frame->data[0] << " | " << frame->data[1] << " | " << frame->data[2] << " | " << frame->data[3] << std::endl;
        std::cout<<"Frame data[0] size: "<<frame->linesize[0]<<std::endl;
        std::cout<<"Frame data[1] size: "<<frame->linesize[1]<<std::endl;
        std::cout<<"Frame data[2] size: "<<frame->linesize[2]<<std::endl;
        std::cout<<"Pkt size: "<<pkt->size/1024<<"KB" << std::endl;
        
        //Testpkt

        //AVPacket* pH264Packet = av_packet_alloc();
        AVFrame* pH264Frame = av_frame_alloc();
        //ret = avcodec_send_packet(pH264CodecContext, pkt);
        ret = avcodec_send_packet(codecContext, pkt);
      
        if (ret < 0) {
            std::cout << "decodeH264Frame avcodec_send_packet ret: " << ret << std::endl;
        }
        //ret = avcodec_receive_frame(pH264CodecContext, pH264Frame);
        ret = avcodec_receive_frame(codecContext, pH264Frame);
        if (ret < 0) {
            std::cout << "decodeH264Frame avcodec_receive_frame ret: " << ret << std::endl;
        }
        //displayFrame(pH264Frame, codecContext);

        //Test

        displayFrame(frame, codecContext);
        //avcodec_send_packet(codecContext, pkt);
        //avcodec_receive_frame(codecContext, frame);
 

        av_packet_unref(pkt);
    }

    sws_freeContext(swsCtx);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&codecContext);
    cap.release();

    return 0;
}