extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/timestamp.h>

#include <libavdevice/avdevice.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}


//std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

//opencv
#include <opencv2/opencv.hpp>


int main(int argc, char** argv) {

	avdevice_register_all();
	
	//Codec & Codec Context
	const AVCodec* pCodecOut;
	//AVCodec* pCodecInCam;	will need to be defined in the future
	AVCodecContext* pCodecCtxOut = NULL;
	AVCodecContext* pCodecCtxInCam = NULL;

	char args_cam[512]; //filter graph

	int i, ret, got_output, video_stream_idx_cam;

	AVFrame* pCamFrame, * pOutFrame, *pFilterFrame;
	AVPacket* pPacket;

	//const char* pOutFileName = "output.mp4";
	const char* pOutFileName = "output.webm";

	const AVOutputFormat* pOutputFormat;	//will need to be defined in the future
	AVStream* pOutStream = NULL;

	const AVInputFormat* pInputFormat = av_find_input_format("dshow");
	AVDictionary* pInputOptions = NULL;

	AVFormatContext* pFormatCtxOut = NULL;
	AVFormatContext* pFormatCtxInCam = NULL;

	//to create the filter graph
	const AVFilter* pBufferSrc_cam = avfilter_get_by_name("buffer");
	const AVFilter* pBufferSink = avfilter_get_by_name("buffersink");

	AVFilterInOut* pOutputs = avfilter_inout_alloc();
	AVFilterInOut* pInputs = avfilter_inout_alloc();

	AVFilterContext* pBuffersink_ctx = NULL;
	AVFilterContext* pBuffersrc_ctx_cam = NULL;
	AVFilterGraph * pFilterGraph = avfilter_graph_alloc();

	//AVBufferSinkParams* pBuffersink_params = av_buffersink_params_alloc();

	const char* filter_str = "scale='w=-1:h=480',format='yuv420p'";

	pFormatCtxInCam = avformat_alloc_context();

	av_dict_set(&pInputOptions, "video_size", "640x480", 0);
	//av_dict_set(&pInputOptions, "pixel_format", "mjpeg", 0);
	av_dict_set(&pInputOptions, "framerate", "30", 0);

	ret = avformat_open_input(&pFormatCtxInCam, "video=Integrated Camera", pInputFormat, &pInputOptions);
	if (avformat_find_stream_info(pFormatCtxInCam, NULL) < 0) {
		std::cout<<"Couldn't find stream information."<<std::endl;
		return -1;
	}
	video_stream_idx_cam = -1;
	for (i = 0; i < pFormatCtxInCam->nb_streams; i++) {
		if (pFormatCtxInCam->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_idx_cam = i;
			break;
		}
	}
	if (video_stream_idx_cam == -1) {
		std::cout<<"Couldn't find a video stream."<<std::endl;
		return -1;
	}

	//Set codec context & codec
	AVCodecParameters* pCodecParameters;
	pCodecParameters = pFormatCtxInCam->streams[video_stream_idx_cam]->codecpar;
	
	const AVCodec* pCodecInCam = avcodec_find_decoder(pCodecParameters->codec_id);
	if (pCodecInCam == NULL) {
		std::cout<<"Codec not found."<<std::endl;
		return -1;
	}

	////encoder
	//find the format based on the filename
	pOutputFormat = av_guess_format(NULL, pOutFileName, NULL);
	//pOutputFormat->video_codec = AV_CODEC_ID_VP8;
	std::cout<<"Output format: "<<pOutputFormat->name<<std::endl;

	//AVOutputFormat* pOutputFormat_guess = av_guess_format("webm", NULL, NULL);
	pFormatCtxOut = avformat_alloc_context();
	//pOutputFormat->video_codec = AV_CODEC_ID_VP8;
	pFormatCtxOut->oformat = pOutputFormat;

	//add a new video stream
	pOutStream = avformat_new_stream(pFormatCtxOut, NULL);
	if (!pOutStream) {
		fprintf(stderr, "Could not alloc stream\n");
		exit(1);
	}

	AVCodecParameters* pCodecParametersOut = pOutStream->codecpar;
	pCodecParametersOut->codec_id = AV_CODEC_ID_VP8;
	pCodecParametersOut->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecOut = avcodec_find_encoder(pCodecParametersOut->codec_id);
	pCodecCtxOut = avcodec_alloc_context3(pCodecOut);
	if (!pCodecCtxOut) {
		fprintf(stderr, "Could not alloc an encoding context\n");
		exit(1);
	}
	pCodecCtxOut->bit_rate = 400000;
	pCodecCtxOut->width = 640;
	pCodecCtxOut->height = 480;
	pCodecCtxOut->time_base = {1, 30};
	pCodecCtxOut->framerate = {30, 1};
	pCodecCtxOut->gop_size = 10;	//Todo
	pCodecCtxOut->max_b_frames = 1;
	pCodecCtxOut->pix_fmt = AV_PIX_FMT_YUV420P;

	if (pFormatCtxOut->oformat->flags & AVFMT_GLOBALHEADER) {
		pCodecCtxOut->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	//find the encoder
	pCodecOut = avcodec_find_encoder(pCodecCtxOut->codec_id);
	//std::cout<<"Codec found: "<<pCodecOut->name<<std::endl;
	if (!pCodecOut) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}
	

}