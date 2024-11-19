#include <frame_codec.hpp>


int main() {
	std::cout<<"Hello opencv transcodec\n";

	int ret;
	unsigned int stream_index;
	AVPacket* packet = NULL;

	FrameCodec& frameCodec = FrameCodec::getInstance();

	frameCodec.open_input_cam_smp("video=Integrated Camera");
	frameCodec.prepare420PFrame();
	frameCodec.set_264_encode_smp();
	frameCodec.set_264_decode_smp();
	frameCodec.display_through_opencv();





	return 0;
}