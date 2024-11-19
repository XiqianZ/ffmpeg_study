#ifndef FRAME_CODEC_HPP
#define FRAME_CODEC_HPP

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/timestamp.h>

#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}


//std
#include<vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>

#include <opencv2/opencv.hpp>

#include<dshow.h>

class FrameCodec {

public:

	typedef struct StreamContext {
		AVCodecContext* dec_ctx;
		AVCodecContext* enc_ctx;

		AVFrame* dec_frame;
	} StreamContext;

	typedef struct FilteringContext {
		AVFilterContext* buffersink_ctx;
		AVFilterContext* buffersrc_ctx;
		AVFilterGraph* filter_graph;

		AVPacket* enc_pkt;
		AVFrame* filtered_frame;
	} FilteringContext;


	//singletone implementation
	static FrameCodec& getInstance() {
		static FrameCodec instance;
		return instance;
	}

	int encode_write_frame(unsigned int stream_index, int flush);
	int open_input_file(const char* filename);
	int open_output_file(const char* filename);
	int open_intput_cam(const char* cam_name);


	int open_input_cam_smp(const char* cam_name);
	int display_through_opencv();
	int set_264_encode_smp();
	int set_264_decode_smp();


	int init_filter(FilteringContext* fctx, AVCodecContext* dec_ctx, AVCodecContext* enc_ctx, const char* filter_spec);
	int init_filters(void);
	int filter_encode_write_frame(AVFrame* frame, unsigned int stream_index);


	void prepare420PFrame();

	int flush_encoder(unsigned int stream_index);

	void free_resource();

	const char* getDeviceName() {
		return deviceName;
	}

	AVFormatContext* getIfmtCtx() {return ifmt_ctx;}
	AVFormatContext* getOfmtCtx() {return ofmt_ctx;}
	StreamContext* getStreamCtx() {return stream_ctx;}
	FilteringContext* getFilterCtx() {return filter_ctx;}


private:
	FrameCodec() {
		//init();
	}
	~FrameCodec() {
		//release();
	}

	AVFormatContext* ifmt_ctx = NULL;
	AVFormatContext* ofmt_ctx = NULL;

	StreamContext* stream_ctx = NULL;
	FilteringContext* filter_ctx = NULL;

	AVCodecContext* pEncCodecContext = NULL;
	AVCodecContext* pDecCodecContext = NULL;

	const char* deviceName = "video=Integrated Camera";

	int64_t last_pts = AV_NOPTS_VALUE;

	cv::Mat openCVMat;
	cv::Mat decodedMat;

	AVFrame* pframe420P = NULL;


	void convertAVFrameToMat(AVCodecContext* codec_ctx, AVFrame* frame, cv::Mat& img); //Todo test
	void convert_frame(AVFrame* src_frame, AVFrame** dst_frame, AVPixelFormat dst_pix_fmt);
	void displayH264FrameOpenCV(AVCodecContext dec_ctx, AVPacket *pkt);




};






#endif //FRAME_CODEC_HPP