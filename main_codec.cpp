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



int main()
{
	std::cout << "Hello CMake codec app" << std::endl;


	AVFormatContext* pFormatContext = NULL;
	AVCodecContext* pCodecContext = NULL;
	AVFrame* pFrame = NULL;
	AVPacket* pPacket = NULL;
	AVCodecParameters* pCodecParameters = NULL;

	cv::Mat openCVframe;

	int videoStreamIndex = -1;


	avdevice_register_all();
	const char* deviceName = "video=Integrated Camera";
	const AVInputFormat* pInputFormat = av_find_input_format("dshow");
	AVDictionary* pDictionary = NULL;
	av_dict_set(&pDictionary, "video_size", "960x540", 0);
	av_dict_set(&pDictionary, "framerate", "30", 0);
	av_dict_set(&pDictionary, "fflags", "nobuffer", 0); av_dict_set(&pDictionary, "framedrop", "1", 0);
	av_dict_set(&pDictionary, "flags", "low_delay", 0);

	avformat_open_input(&pFormatContext, deviceName, pInputFormat, &pDictionary);
	avformat_find_stream_info(pFormatContext, NULL);

	for (unsigned int i = 0; i < pFormatContext->nb_streams; i++) {
		if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {   //So here video stream index indeed is 0
			videoStreamIndex = i;
			break;
		}
	}
	std::cout << "videoStreamIndex: " << videoStreamIndex << std::endl;
	if (videoStreamIndex == -1) {   //If no video stream found
		std::cout << "Could not find video stream" << std::endl;
		return -1;
	}

	pCodecParameters = pFormatContext->streams[videoStreamIndex]->codecpar;
	std::cout << "Codec ID: " << pCodecParameters->codec_id <<std::endl;
	std::cout << "Codec Name: " << avcodec_get_name(pCodecParameters->codec_id) << std::endl;
	std::cout << "Codec Type Name: " << avcodec_get_type(pCodecParameters->codec_id) << std::endl;


	const AVCodec* pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
	pCodecContext = avcodec_alloc_context3(pCodec);
	pCodecContext->skip_frame = AVDISCARD_NONREF;

	avcodec_parameters_to_context(pCodecContext, pCodecParameters);
	avcodec_open2(pCodecContext, pCodec, NULL);



	pFrame = av_frame_alloc();
	pPacket = av_packet_alloc();
	while (av_read_frame(pFormatContext, pPacket) >= 0) {
		if (pPacket->stream_index == videoStreamIndex) {
			avcodec_send_packet(pCodecContext, pPacket);
			avcodec_receive_frame(pCodecContext, pFrame);
		}
		av_packet_unref(pPacket);

		struct SwsContext* sws_ctx = sws_getContext(
			pCodecContext->width,
			pCodecContext->height,
			pCodecContext->pix_fmt,
			pCodecContext->width,
			pCodecContext->height,
			AV_PIX_FMT_BGR24,
			SWS_BILINEAR,
			nullptr, nullptr, nullptr
		);

		// Create a cv::Mat to hold the BGR image
		openCVframe.create(pCodecContext->height, pCodecContext->width, CV_8UC3);

		// Allocate a buffer to store the converted data
		uint8_t* dest[4] = { openCVframe.data, nullptr, nullptr, nullptr };
		int dest_linesize[4] = { openCVframe.step[0], 0, 0, 0 };
		sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 0, pCodecContext->height, dest, dest_linesize);
		int frameSizeKB = (openCVframe.cols * openCVframe.rows * openCVframe.channels()) / 1024;
		sws_freeContext(sws_ctx);
		cv::imshow("Codec Test", openCVframe);	
		cv::waitKey(1);
	}





	avformat_close_input(&pFormatContext);
	av_frame_free(&pFrame);
	av_packet_free(&pPacket);
	


	return 0;
}
