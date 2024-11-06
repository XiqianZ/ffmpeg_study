#include <frame_server.hpp>


#include <thread>
#include <chrono>
#include <mutex>

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

using namespace std;

std::mutex send_mutex;

//class specified variables



FrameServer::FrameServer() {
	m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
	m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
	m_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
	m_endpoint.set_access_channels(websocketpp::log::alevel::app);

	m_endpoint.init_asio();

	m_endpoint.set_open_handler(bind(&FrameServer::on_open, this, _1));
	//m_endpoint.set_http_handler(bind(&FrameServer::on_http, this, _1));
	m_endpoint.set_close_handler(bind(&FrameServer::on_close, this, _1));
	m_endpoint.set_message_handler(bind(&FrameServer::on_message, this, _1, _2));

	//pFrameHandler = &FrameHandler::getInstance();
	//pFrameHandler->init();
}


void FrameServer::run() {
	m_endpoint.listen(9002);
	m_endpoint.start_accept();

	try {
		m_endpoint.run();
	}
	catch (websocketpp::exception const& e) {
		std::cout << "==== Dev ==== run "<< e.what() << std::endl;
	}
	catch (...) {
		std::cout << "other exception" << std::endl;
	}
}




void FrameServer::run_send_frame() {
	while (1) {

		wait_frame_ready();

		//if (can_send && pFrameHandler->isFrameReady() && (pFrameHandler->getFramePts()> m_last_frame_pts)) {
		if (can_send) {
			m_last_frame_pts = pFrameHandler->getFramePts();
			can_send = false;
			//frame_data.clear();
			//frame_data.resize(pFrameHandler->getFrame()->pkt_size);
			//memcpy(frame_data.data(), pFrameHandler->getFrame()->data, pFrameHandler->getFrame()->pkt_size);
			//sendFrame(frame_data);

			sendFrame(pFrameHandler->getEncodedFrame());
			pFrameHandler->resetFrameready();

			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			start = end;
			std::cout<<" ==== Dev ==== send frame duration: " << duration.count() << "ms" << std::endl;
		}
	}
}


void FrameServer::wait_frame_ready() {
	std::unique_lock<std::mutex> lock(pFrameHandler->getFrameLock());
	pFrameHandler->getFrameReadyCond().wait(lock, [this] {
		std::cout << "==== Dev ==== wait_frame_ready " << std::endl;
		return pFrameHandler->isFrameReady();
	});
	can_send = true;
}


void FrameServer::sendFrame(std::vector<uint8_t> data) {
	//std::lock_guard<std::mutex> lock(send_mutex);

	con_list::iterator it;
	for (it = m_connections.begin(); it != m_connections.end(); ++it) {
		m_endpoint.send(*it,
		data.data(),
		data.size(),
		websocketpp::frame::opcode::binary);
	}
}


void FrameServer::on_open(connection_hdl hdl) {
	{
		std::cout << "==== Dev ==== on_open" << std::endl;
		lock_guard<mutex> guard(m_action_lock);
		m_actions.push(action(SUBSCRIBE, hdl));
	}
	m_action_cond.notify_one();
}


void FrameServer::on_close(connection_hdl hdl) {
	{
		std::cout << "==== Dev ==== Connection closed" << std::endl;
		lock_guard<mutex> guard(m_action_lock);
		m_actions.push(action(UNSUBSCRIBE, hdl));
	}
	m_action_cond.notify_one();
}


void FrameServer::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
	{
		std::cout <<msg_count<<" "<< msg->get_payload() << std::endl;
		lock_guard<mutex> guard(m_action_lock);
		m_actions.push(action(MESSAGE, hdl, msg));
	}
	m_action_cond.notify_one();
	//if (msg->get_payload() == "rec" && msg_count < 10) {
	//if (msg->get_payload() == "rec") {
	//	msg_count++;
	//	m_endpoint.send(hdl, "cmd", websocketpp::frame::opcode::text);
	//	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//}
}


void FrameServer::proceed_messages() {
	while (true) {
		unique_lock<mutex> lock(m_action_lock);

		while (m_actions.empty()) {
			m_action_cond.wait(lock);
		}	

		action a = m_actions.front();
		m_actions.pop();

		lock.unlock();
		if (a.type == SUBSCRIBE) {
			lock_guard<mutex> guard(m_connection_lock);
			m_connections.insert(a.hdl);
		}
		else if (a.type == UNSUBSCRIBE) {
			lock_guard<mutex> guard(m_connection_lock);
			m_connections.erase(a.hdl);
		}
		else if (a.type == MESSAGE) {
			lock_guard<mutex> guard(m_connection_lock);
			con_list::iterator it;
			for (it = m_connections.begin(); it != m_connections.end(); ++it) {
				//m_endpoint.send(*it, a.msg->get_payload(), websocketpp::frame::opcode::text);
			}
		}
		else {
			//Todo
		}
	}
}