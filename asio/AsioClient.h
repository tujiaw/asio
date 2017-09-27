#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <deque>
#include "Constant.h"
#include "Buffer.h"

using boost::asio::ip::tcp;
class AsioClient
{
public:
	explicit AsioClient(const std::string &address);
	~AsioClient();
	void run();
	void stop();

	void sendMessage(const MessagePtr &msgPtr, int msTimeout);
	void postMessage(const MessagePtr &msgPtr, const Response &res);

private:
	AsioClient(AsioClient &);
	void operator=(AsioClient &);
	void onWrite();

private:
	boost::asio::io_service io_;
	tcp::socket socket_;
	std::thread runthread_;
	std::deque<MessagePtr> pendingList_;
	std::deque<MessagePtr> sentList_;
};

