#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <thread>

#include <opencv2/opencv.hpp>

#include <frame_handler.hpp>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;



typedef websocketpp::client<websocketpp::config::asio_client> client;

FrameHandler& frameHandler = FrameHandler::getInstance();


void on_open(websocketpp::connection_hdl hdl, client* c) {
	std::cout << "WebSocket connection opened!" << std::endl;
	websocketpp::lib::error_code ec;
	client::connection_ptr con = c->get_con_from_hdl(hdl, ec);
	if (ec) {
		std::cout << "could not get connection from handle because: " << ec.message() << std::endl;
		return;
	}
	std::string payload = "Hello World!";
	//std::string payload = "{\"userKey\":\"API_KEY\", \"symbol\":\"EURUSD,GBPUSD\"}";
	
}


void on_fail(websocketpp::connection_hdl hdl) {
	std::cout << "Connection failed" << std::endl;
}

void on_close(websocketpp::connection_hdl hdl) {
	std::cout << "Connection closed" << std::endl;
}

void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg, client* c) {
	//std::cout << "Recevid message: " << msg->get_payload() << std::endl;
	if (msg->get_payload() == "cmd") {
		c->send(hdl, "rec", websocketpp::frame::opcode::text);
	}
	else if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
		std::cout << "Received binary message with size: "<<msg->get_payload().size()/1024<<" KB" << std::endl;
		frameHandler.decodeFrame(reinterpret_cast<const uint8_t*>(msg->get_payload().data()), msg->get_payload().size());
		frameHandler.displayFrameByOpenCV();
	}

}



int main(int argc, char* argv[]) {

	std::cout<<"Hello Client"<<std::endl;

	client c;
	int reattempts = 0;

	client::connection_ptr con = nullptr;

	std::string uri = "ws://localhost:9002";

	if (argc == 2) {
		uri = argv[1];
	}

	frameHandler.prepareDecoder();
	
	

	try {
		c.set_access_channels(websocketpp::log::alevel::all);
		c.clear_access_channels(websocketpp::log::alevel::frame_payload);
		c.set_error_channels(websocketpp::log::elevel::all);

		//Initialize ASIO
		c.init_asio();

		//Register message handler
		c.set_open_handler(bind(&on_open, _1, &c));
		c.set_fail_handler(bind(&on_fail, _1));
		c.set_close_handler(bind(&on_close, _1));
		c.set_message_handler(bind(&on_message, _1, _2, &c));
		//c.set_message_handler(&on_message);

		websocketpp::lib::error_code ec;
		con = c.get_connection(uri, ec);
		if (ec) {
			std::cout << "could not create connection because: " << ec.message() << std::endl;
			return 0;
		}
		c.connect(con);
		c.run();
	}
	catch (websocketpp::exception const& e) {
		std::cout << e.what() << std::endl;
		std::cout<<"====================="<<std::endl;
	}
	catch (...) {
		std::cout << "other exception" << std::endl;
	}



}