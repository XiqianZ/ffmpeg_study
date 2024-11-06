#ifndef FRAME_SERVER_HPP
#define FRAME_SERVER_HPP

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>

#include <frame_handler.hpp>

#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <string>
#include <chrono>




class FrameServer {
public:
	typedef websocketpp::connection_hdl connection_hdl;
	typedef websocketpp::server<websocketpp::config::asio> server;

	static FrameServer& getInstance() {
		static FrameServer instance;
		return instance;
	}

	void run();
	//void set_timer();
	//void on_timer(const websocketpp::lib::error_code & ec);
	void run_send_frame();
	void wait_frame_ready();

	//void on_http(connection_hdl hdl);
	void on_open(connection_hdl hdl);
	void on_close(connection_hdl hdl);
	void on_message(connection_hdl hdl, server::message_ptr msg);
	void proceed_messages();

	void setFrameHandler(FrameHandler* pFrameHandler) {this->pFrameHandler = pFrameHandler;}

private:
	typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;



	enum action_type {
		SUBSCRIBE,
		UNSUBSCRIBE,
		MESSAGE
	};

	struct action {
		action(action_type t, connection_hdl h) : type(t), hdl(h) {}
		action(action_type t, connection_hdl h, server::message_ptr m)
			: type(t), hdl(h), msg(m) {}

		action_type type;
		websocketpp::connection_hdl hdl;
		server::message_ptr msg;
	};

	FrameServer();
	FrameServer(const FrameServer&) = delete;
	FrameServer& operator=(const FrameServer&) = delete;
	~FrameServer() {};

	void sendFrame(std::vector<uint8_t> data);

	server m_endpoint;
	con_list m_connections;
	std::queue<action> m_actions;
	server::timer_ptr m_timer;

	websocketpp::lib::mutex m_action_lock;
	websocketpp::lib::mutex m_connection_lock;
	websocketpp::lib::condition_variable m_action_cond;



	//using websocketpp::lib::unique_lock;
	//using websocketpp::lib::condition_variable;

	FrameHandler* pFrameHandler = nullptr;

	int msg_count = 0;
	int old_msg_count = 0;

	bool can_send = false;

	std::vector<uint8_t> frame_data;

	int64_t m_last_frame_pts = -1;

	uint64_t m_data = 0;

	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
};






#endif	// !FRAME_SERVER_HPP