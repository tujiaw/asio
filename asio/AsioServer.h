#pragma once

#include <boost/enable_shared_from_this.hpp>
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
	void run();
	void stop();

	void addHandleMessage(const std::string &protoName, const Task &task);

private:
	AsioServer(AsioServer &);
	void operator=(AsioServer &);
	void onAccept();

private:
	boost::asio::io_service io_;
	tcp::acceptor acceptor_;
	tcp::socket socket_;
	std::thread runthread_;
};

