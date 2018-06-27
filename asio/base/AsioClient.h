#pragma once

#include <boost/asio.hpp>
#include <thread>
#include <deque>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "desc.h"
#include "Buffer.h"

using boost::asio::ip::tcp;
class AsioClient {
public:
	explicit AsioClient(const std::string &address, int heartbeatSeconds = 0);
	~AsioClient();
	AsioClient(const AsioClient&) = delete;
	AsioClient& operator=(const AsioClient&) = delete;

	void addHandlePublish(const std::string &typeName, const PublishFunc &func);
	void start();
	void stop();
	bool stopped() const;

	int sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout);
	int postMessage(const MessagePtr &msgPtr, const Response &res);

private:
	void close();
	void onRead();
	void onWrite();
	void doClose();
	void doSubscribe();
	void onHeartbeat();
	void doHeartbeat(const boost::system::error_code &e);
    void doResponse(const PackagePtr &pack);

private:
	std::string address_;
	boost::asio::io_service io_;
    boost::asio::io_service ioDoResponse_;
	tcp::socket socket_;
	std::deque<PackagePtr> pendingList_;
	std::atomic<int> id_;
	std::thread runthread_;
    std::thread threadDoResponse_;
	std::unique_ptr<boost::asio::deadline_timer> heartbeatTimer_;
	int heartbeatSeconds_;
	std::atomic<bool> isOnline_;

    static const int kTempBufSize = boost::asio::detail::default_max_transfer_size;
    char tempBuf_[kTempBufSize];
	Buffer readBuffer_;

	std::mutex mutex_;
	std::condition_variable cond_;

	std::mutex responseMutex_;
	std::map<int, std::pair<PackagePtr, Response>> responseMap_;
	std::map<std::string, PublishFunc> publishMap_;
};

