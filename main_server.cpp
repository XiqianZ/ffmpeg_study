// socket_study2.cpp : Defines the entry point for the application.
//

#include <frame_handler.hpp>
#include <frame_server.hpp>

//std
#include <iostream>
#include <string>
#include <vector>



using namespace std;



int main()
{
	cout << "Hello CMake socket server app" << endl;
    FrameHandler& frameHandler = FrameHandler::getInstance();

	try {
		FrameServer& frameServer = FrameServer::getInstance();
		frameServer.setFrameHandler(&frameHandler);
		frameHandler.init();

		thread frame_handler_thread(&FrameHandler::retriveFrameFromCamera, &frameHandler);
		thread send_frame_thread(&FrameServer::run_send_frame, &frameServer);
		thread server_porceed_thread(&FrameServer::proceed_messages, &frameServer);

		frameServer.run();

		server_porceed_thread.join();
	}
	catch (websocketpp::exception const & e){
		std::cout << "==== Dev ==== main " << e.what() << std::endl;
	}

	return 0;
}
