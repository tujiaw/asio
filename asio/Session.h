#pragma once

#include <boost/asio.hpp>
#include <thread>
#include "Buffer.h"
#include "TaskManager.h"

using boost::asio::ip::tcp;
class session : public std::enable_shared_from_this<session>
{
public:
	session(tcp::socket socket);
	~session();

	void start();
	void replyMessage(const PackagePtr &rspPtr);

private:
	void onRead();
	void onWrite(BufferPtr writeBuffer);

	tcp::socket socket_;
	static const int kTempBufSize = 2048;
	char tempBuf_[kTempBufSize];
	Buffer readBuffer_;
};