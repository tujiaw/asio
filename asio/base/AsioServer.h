#pragma once

#include <boost/asio.hpp>
#include <thread>
#include "Buffer.h"
#include "TaskManager.h"

using boost::asio::ip::tcp;
class AsioServer
{
public:
	explicit AsioServer(unsigned short port);
	~AsioServer();
	AsioServer(const AsioServer&) = delete;
	AsioServer& operator=(const AsioServer&) = delete;

	void run();
	void stop();

	void addHandleMessage(const std::string &protoName, const Task &task);
	void publishMessage(const MessagePtr &msg);

private:
	void init();
	void onAccept();

private:
	boost::asio::io_service io_;
	tcp::acceptor acceptor_;
	tcp::socket socket_;
	std::thread runthread_;
};
