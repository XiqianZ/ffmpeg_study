#include <frame_handler.hpp>




FrameHandler::FrameHandler() {
    auto start = std::chrono::high_resolution_clock::now();
}


FrameHandler::~FrameHandler() {
	av_frame_free(&pFrame);
	av_packet_free(&pPacket);
	avcodec_free_context(&pCodecContext);
	avformat_close_input(&pFormatContext);
}


void FrameHandler::init() {
    //ffmpeg
    avdevice_register_all();
    const char* deviceName = "video=Integrated Camera";
    const AVInputFormat* pInputFormat = av_find_input_format("dshow");
    AVDictionary* pDictionary = NULL;
    if (av_dict_set(&pDictionary, "video_size", "960x540", 0) != 0) {
        std::cout << "Could not set video size" << std::endl;
        std::abort();
    }
    if (av_dict_set(&pDictionary, "framerate", "30", 0) != 0) {
        std::cout << "Could not set framerate" << std::endl;
        std::abort();
    }
    if (av_dict_set(&pDictionary, "fflags", "nobuffer", 0) != 0) {
		std::cout << "Could not set fflags" << std::endl;
		std::abort();
	}
    if (av_dict_set(&pDictionary, "framedrop", "1", 0) != 0) {
        std::cout<< "Could not set framedrop" << std::endl;
        std::abort();
    }
    if (av_dict_set(&pDictionary, "flags", "low_delay", 0) != 0) {
		std::cout << "Could not set flags" << std::endl;
		std::abort();
	}
    


    if (avformat_open_input(&pFormatContext, deviceName, pInputFormat, &pDictionary) != 0) {
        std::cout << "Could not open camera" << std::endl;
    }
    else {
        std::cout << "Camera opened" << std::endl;
    }

    if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
        std::cout << "Could not find stream info" << std::endl;
    }
    else {
        std::cout << "Stream info found" << std::endl;
    }
    std::cout << "Number of streams: " << pFormatContext->nb_streams << std::endl;
    for (unsigned int i = 0; i < pFormatContext->nb_streams; i++) {
        if (pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {   //So here video stream index indeed is 0
            videoStreamIndex = i;
            break;
        }
    }
    std::cout << "videoStreamIndex: " << videoStreamIndex << std::endl;

    if (videoStreamIndex == -1) {   //If no video stream found
        std::cout << "Could not find video stream" << std::endl;
        return;
    }

    pCodecParameters = pFormatContext->streams[videoStreamIndex]->codecpar;
    std::cout << "Codec ID: " << pCodecParameters->codec_id << std::endl;
    std::cout << "Codec Name: " << avcodec_get_name(pCodecParameters->codec_id) << std::endl;


    const AVCodec* pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if (!pCodec) {
        std::cout << "Unsupported codec" << std::endl;
        return;
    }
    pCodecContext = avcodec_alloc_context3(pCodec);
    pCodecContext->skip_frame = AVDISCARD_NONREF;
    if (!pCodecContext) {
        std::cout << "Could not allocate codec context" << std::endl;
        return;
    }
    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0) {
        std::cout << "Could not copy codec parameters to context" << std::endl;
        return;
    }
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
        std::cout << "Could not open codec" << std::endl;
        return;
    }

    //Allocate frame and packet
    pFrame = av_frame_alloc();
    pPacket = av_packet_alloc();


    prepareH264Decoder();   //Todo Test
}



void FrameHandler::convertAVFrameToMat(AVCodecContext* codec_ctx, AVFrame* frame, cv::Mat& img) {
    // Initialize the SwsContext for converting the pixel format
    struct SwsContext* sws_ctx = sws_getContext(
        codec_ctx->width,
        codec_ctx->height,
        codec_ctx->pix_fmt,
        codec_ctx->width,
        codec_ctx->height,
        AV_PIX_FMT_BGR24,
        SWS_BILINEAR,
        nullptr, nullptr, nullptr
    );

    // Create a cv::Mat to hold the BGR image
    img.create(codec_ctx->height, codec_ctx->width, CV_8UC3);

    // Allocate a buffer to store the converted data
    uint8_t* dest[4] = { img.data, nullptr, nullptr, nullptr };
    int dest_linesize[4] = { img.step[0], 0, 0, 0 };

    // Convert the frame to BGR and store it in the img Mat
    sws_scale(sws_ctx, frame->data, frame->linesize, 0, codec_ctx->height, dest, dest_linesize);


    int frameSizeKB = (img.cols * img.rows * img.channels()) / 1024;
    //std::cout << "CV2 Frame size: " << frameSizeKB << " KB" << " Width: " << img.cols << " Height: " << img.rows << std::endl;

    // Free the SwsContext after use
    sws_freeContext(sws_ctx);
}



void FrameHandler::retriveFrameFromCamera() {
    while (av_read_frame(pFormatContext, pPacket) >= 0) {
        //std::cout<<"*********** read frame ***********"<<std::endl;
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        start = end;


        pFrameReady = false;
        if (pPacket->stream_index == videoStreamIndex) {
            std::lock_guard<std::mutex> lock(pFrameLock);


            std::cout << " ==== Dev ==== retrive frame duration: " << duration.count() << "ms" << std::endl;


            int response = avcodec_send_packet(pCodecContext, pPacket);
            if (response < 0) {
                std::cout << "Error sending packet to codec context" << std::endl;
                break;
            }
            response = avcodec_receive_frame(pCodecContext, pFrame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                continue;
            }
            else if (response < 0) {
                std::cout << "Error receiving frame from codec context" << std::endl;
                break;
            }



            // Convert the frame to H.264




            if (pFrame == nullptr) {
                std::cout << "Frame is null" << std::endl;
                break;
            }



            /*response = avcodec_send_frame(pH264CodecContext, pFrame);
            if (response < 0) {
                std::cout << "Error sending frame to H.264 codec context" << std::endl;
                avcodec_free_context(&pH264CodecContext);
                av_packet_free(&pH264Packet);
                break;
            }

            response = avcodec_receive_packet(pH264CodecContext, pH264Packet);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                avcodec_free_context(&pH264CodecContext);
                av_packet_free(&pH264Packet);
                continue;
            }
            else if (response < 0) {
                std::cout << "Error receiving packet from H.264 codec context" << std::endl;
                avcodec_free_context(&pH264CodecContext);
                av_packet_free(&pH264Packet);
                break;
            }

            int H264PacketSize = pH264Packet->size / 1024;
            std::cout << "pH264Packet size: " << H264PacketSize << " KB" << std::endl;

            std::cout << "Frame " << pCodecContext->frame_num << " (type=" << av_get_picture_type_char(pFrame->pict_type)
                << ", size=" << pFrame->pkt_size / 1024
                << "Kb) key_frame " << pFrame->key_frame
                << " Width and Height: " << pFrame->width << "x" << pFrame->height
                << " Codec Name: " << avcodec_get_name(pH264CodecContext->codec_id) << " Codec ID: " << pH264CodecContext->codec_id
                << std::endl;

            av_packet_unref(pH264Packet);
            avcodec_free_context(&pH264CodecContext);*/

            // Use pH264Packet as needed
            // ...

            //eFrameReady = false;
            {
                std::lock_guard<std::mutex> lock(eFrameLock);
                encoded_frame.clear();
                encoded_frame.resize(pPacket->size);
                memcpy(encoded_frame.data(), pPacket->data, pPacket->size);
            }
            //eFrameReady = true;
            //std::cout << "Frame " << pCodecContext->frame_num << " (type=" << av_get_picture_type_char(pFrame->pict_type)
            //    << ", size=" << pFrame->pkt_size / 1024
            //    << "Kb) key_frame " << pFrame->key_frame
            //    << " Width and Height: " << pFrame->width << "x" << pFrame->height
            //    << " Codec Name: " << avcodec_get_name(pCodecParameters->codec_id) << " Codec ID: " << pCodecParameters->codec_id
            //    << std::endl;

            int frameSizeKB2 = pPacket->size / 1024;
            //std::cout << "FFMPEG Frame size: " << frameSizeKB2 << " KB" << " Width: " << pFrame->width << " Height: " << pFrame->height << std::endl;

        }
        pFrameReady = true;
        pFrameReadyCond.notify_one();
        displayFrameByOpenCV();
        av_packet_unref(pPacket);

    }
}


bool FrameHandler::readOneFrameFromCamera() {
    if (av_read_frame(pFormatContext, pPacket) >= 0) {
        if (pPacket->stream_index == videoStreamIndex) {
            if (isLastPacket()) {
                int response = avcodec_send_packet(pCodecContext, pPacket);
                if (response < 0) {
                    std::cout << "Error sending packet to codec context" << std::endl;
                    return false;
                }
                response = avcodec_receive_frame(pCodecContext, pFrame);
                if (response < 0) {
                    std::cout << "Error receiving frame from codec context" << std::endl;
                    return false;
                }
                //encoded_frame.clear();
                //encoded_frame.resize(pFrame->pkt_size);
                //memcpy(encoded_frame.data(), pPacket->data, pFrame->pkt_size);

                
                //std::cout << "Frame " << pCodecContext->frame_num << " (type=" << av_get_picture_type_char(pFrame->pict_type)
                //    << ", size=" << pFrame->pkt_size / 1024
                //    << "Kb) key_frame " << pFrame->key_frame
                //    << " Width and Height: " << pFrame->width << "x" << pFrame->height
                //    << " Codec Name: " << avcodec_get_name(pCodecParameters->codec_id) << " Codec ID: " << pCodecParameters->codec_id
                //    << std::endl;

                int frameSizeKB2 = pPacket->size / 1024;
                //std::cout << "FFMPEG Frame size: " << frameSizeKB2 << " KB" << " Width: " << pFrame->width << " Height: " << pFrame->height << std::endl;

                av_packet_unref(pPacket);
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
    return true;
}


void FrameHandler::displayFrameByOpenCV() {
    convertAVFrameToMat(pCodecContext, pFrame, openCVframe);
    cv::imshow("Camera", openCVframe);
    //cv::waitKey(1);
    cv::pollKey();
    //if (cv::waitKey(1) == 27) {
    //    return;
    //}
}


bool FrameHandler::isLastFrame() {
    if (pFrame->pts > last_frame_pts) {
        last_frame_pts = pFrame->pts;
        return true;
    }
    else {
		return false;
    }
}


bool FrameHandler::isLastPacket() {
    if (pPacket->pts > last_packet_pts) {
        last_packet_pts = pPacket->pts;
		return true;
	}
    else {
		return false;
	}
}



void FrameHandler::prepareH264Decoder() {

    const AVCodec* pH264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!pH264Codec) {
        std::cout << "H.264 codec not found" << std::endl;
        av_packet_free(&pH264Packet);
        return;
    }

    pH264CodecContext = avcodec_alloc_context3(pH264Codec);
    if (!pH264CodecContext) {
        std::cout << "Could not allocate H.264 codec context" << std::endl;
        av_packet_free(&pH264Packet);
        return;
    }

    pH264CodecContext->bit_rate = 400000;
    pH264CodecContext->width = pFrame->width;
    pH264CodecContext->height = pFrame->height;
    pH264CodecContext->time_base = { 1, 25 };
    pH264CodecContext->framerate = { 25, 1 };
    pH264CodecContext->gop_size = 10;
    pH264CodecContext->max_b_frames = 1;
    pH264CodecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    if (avcodec_open2(pH264CodecContext, pH264Codec, NULL) < 0) {
        std::cout << "Could not open H.264 codec" << std::endl;
        avcodec_free_context(&pH264CodecContext);
        av_packet_free(&pH264Packet);
        return;
    }

    pH264Packet = av_packet_alloc();    //Todo test
    if (!pH264Packet) {
        std::cout << "Could not allocate H.264 packet" << std::endl;
        return;
    }
}



void FrameHandler::prepareDecoder() {
    const AVCodec* pCodec = avcodec_find_decoder(AV_CODEC_ID_RAWVIDEO);
    if (!pCodec) {
		std::cout << "Unsupported codec" << std::endl;
		return;
    }
    pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext) {
        std::cout << "Could not allocate codec context" << std::endl;
        return;
    }
    pCodecContext->width = RAW_FRAME_WIDTH;
    pCodecContext->height = RAW_FRAME_HEIGHT;
    pCodecContext->pix_fmt = AV_PIX_FMT_YUYV422;

    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
		std::cout << "Could not open codec" << std::endl;
		return;
    }
}


void FrameHandler::decodeFrame(const uint8_t* data, size_t data_size) {
    pFrame = av_frame_alloc();
    pPacket = av_packet_alloc();

    av_new_packet(pPacket, data_size);
    memcpy(pPacket->data, data, data_size);

    int ret = avcodec_send_packet(pCodecContext, pPacket);
    if (ret < 0) {
		std::cout << "Error sending packet to codec context" << std::endl;
		return;
	}
    ret = avcodec_receive_frame(pCodecContext, pFrame);
    if (ret < 0) {
		std::cout << "Error receiving frame from codec context" << std::endl;
		return;
	}

	av_packet_free(&pPacket);
}