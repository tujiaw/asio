#pragma once

#include <boost/asio.hpp>
#include <thread>
#include <deque>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "Constant.h"
#include "Buffer.h"
#include "threadpool.h"

using boost::asio::ip::tcp;
class AsioClient : boost::noncopyable
{
public:
	explicit AsioClient(const std::string &address);
	~AsioClient();
	void stop();
	void close();

	int sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout);
	void postMessage(const MessagePtr &msgPtr, const Response &res);

private:
	void onRead();
	void onWrite();
	void doClose();

private:
	boost::asio::io_service io_;
	tcp::socket socket_;
	std::thread runthread_;
	std::deque<PackagePtr> pendingList_;
	std::atomic<int> id_;

	static const int kTempBufSize = 1024 * 10;
	char tempBuf_[kTempBufSize];
	Buffer readBuffer_;

	std::mutex mutex_;
	std::condition_variable cond_;

	std::map<int, std::pair<PackagePtr, Response>> responseMap_;
	ThreadPool pool_;
};

