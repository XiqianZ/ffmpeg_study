#ifndef FRAME_HANDLER_HPP
#define FRAME_HANDLER_HPP

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



class FrameHandler {

public:
	//singletone implementation
	static FrameHandler& getInstance() {
		static FrameHandler instance;
		return instance;
	}

	void init();

	void retriveFrameFromCamera();
	bool readOneFrameFromCamera();
	void displayFrameByOpenCV();

	void prepareH264Decoder();


	void prepareDecoder();
	void decodeFrame(const uint8_t* data, size_t data_size);


	AVFrame* getFrame() {return pFrame;}
	std::vector<uint8_t> getEncodedFrame() { return encoded_frame; }
	int64_t getFramePts() { return pFrame->pts; }

	bool isFrameReady() { return pFrameReady; }
	void resetFrameready() { pFrameReady = false; }
	std::condition_variable& getFrameReadyCond() { return pFrameReadyCond; }
	std::mutex& getFrameLock() { return pFrameLock; }

private:
	FrameHandler();
	FrameHandler(const FrameHandler&) = delete;
	FrameHandler& operator=(const FrameHandler&) = delete;
	~FrameHandler();

	void convertAVFrameToMat(AVCodecContext* codec_ctx, AVFrame* frame, cv::Mat& img);

	bool isLastFrame();
	bool isLastPacket();


	AVFormatContext* pFormatContext = NULL;
	AVCodecContext* pCodecContext = NULL;
	AVFrame* pFrame = NULL;
	AVPacket* pPacket = NULL;
	AVCodecParameters* pCodecParameters = NULL;


	//AVCodec* pH264Codec = NULL;
	AVCodecContext* pH264CodecContext = NULL;
	AVPacket* pH264Packet = NULL;





	int videoStreamIndex = -1;

	cv::Mat openCVframe;

	std::vector<uint8_t> encoded_frame;

	int RAW_FRAME_WIDTH = 960;
	int RAW_FRAME_HEIGHT = 540;
	int64_t last_frame_pts = -1;
	int64_t last_packet_pts = -1;

	std::mutex pFrameLock;
	std::condition_variable pFrameReadyCond;
	std::mutex eFrameLock;
	bool pFrameReady = false;


	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();


};

#endif // !FRAME_HANDLER_HPP