#include <frame_codec.hpp>

int main() {
	std::cout<<"Hello live transcodec\n";

	int ret;
	unsigned int stream_index;
	AVPacket* packet = NULL;

	FrameCodec& frameCodec = FrameCodec::getInstance();

	char* input_file_name = "test4.mp4";
	char* output_file_name = "out4_test.mp4";

	//if (ret = frameCodec.open_input_file(input_file_name)<0) {
	//	std::cout<<"open_input_file failed\n";
	//	return ret;
	//}
	if(ret = frameCodec.open_intput_cam(frameCodec.getDeviceName())) {
		std::cout<<"open_intput_cam failed\n";
		return ret;
	}
	if (ret = frameCodec.open_output_file(output_file_name)<0) {
		std::cout<<"open_output_file failed\n";
		return ret;
	}
	if (ret = frameCodec.init_filters()<0) {
		std::cout<<"init_filters failed\n";
		return ret;
	}
	if (!(packet = av_packet_alloc())) {
		std::cout<<"av_packet_alloc failed\n";
		return AVERROR(ENOMEM);
	}


	while (1) {
		if (ret = av_read_frame(frameCodec.getIfmtCtx(), packet) < 0) {
			std::cout<<"av_read_frame() failed\n";
			break;
		}
		stream_index = packet->stream_index;
		std::cout<<"stream_index: "<<stream_index<<std::endl;

		if (frameCodec.getFilterCtx()[stream_index].filter_graph) {
			FrameCodec::StreamContext* stream = &frameCodec.getStreamCtx()[stream_index];
			
			ret = avcodec_send_packet(stream->dec_ctx, packet);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
				break;
			}

			while (ret >= 0) {
				ret = avcodec_receive_frame(stream->dec_ctx, stream->dec_frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				} else if (ret < 0) {
					std::cout<<"avcodec_receive_frame failed\n";
					goto end;
				}
				stream->dec_frame->pts = stream->dec_frame->best_effort_timestamp;
				//======================== May not need filter =============================
				ret = frameCodec.filter_encode_write_frame(stream->dec_frame, stream_index);
				if (ret < 0) {
					std::cout<<"filter_encode_write_frame failed\n";
					goto end;
				}
			}
		}
		else {
			av_packet_rescale_ts(packet, 
				frameCodec.getIfmtCtx()->streams[stream_index]->time_base, 
				frameCodec.getOfmtCtx()->streams[stream_index]->time_base);
			ret = av_interleaved_write_frame(frameCodec.getOfmtCtx(), packet);
			if (ret < 0) {
				std::cout<<"av_interleaved_write_frame failed\n";
				goto end;
			}
		}
		av_packet_unref(packet);

		//==== flash encoder ====

		//=======================================================================

	}

	av_write_trailer(frameCodec.getOfmtCtx());

end:
	av_packet_free(&packet);
	frameCodec.free_resource();
	if (ret < 0)
		std::cout << "Error occurred error code: " << ret << std::endl;

	return 0;
}