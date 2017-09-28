#include "AsioClient.h"


AsioClient::AsioClient(const std::string &address)
	: socket_(io_)
{
	tcp::resolver resolver(io_);
	int pos = address.find(":");
	if (pos > 0) {
		auto iter = resolver.resolve({ address.substr(0, pos), address.substr(pos + 1) });
		boost::asio::async_connect(socket_, iter, [this](std::error_code ec, tcp::resolver::iterator) {
			if (!ec) {
			}
			std::cout << "connect " << ec.message() << std::endl;
		});
	}
}
AsioClient::~AsioClient()
{
}


void AsioClient::run()
{
	runthread_ = std::move(std::thread([this]() {
		io_.run();
	}));
}

void AsioClient::stop()
{
	io_.stop();
	runthread_.join();
}

void AsioClient::sendMessage(const MessagePtr &msgPtr, int msTimeout)
{

}

void AsioClient::postMessage(const MessagePtr &msgPtr, const Response &res)
{
	io_.post([this, msgPtr] {
		bool isProgress = !pendingList_.empty();
		pendingList_.push_back(msgPtr);
		if (!isProgress) {
			onWrite();
		}
	});
}

void AsioClient::onWrite()
{
	std::string buf = std::move(pendingList_.front()->SerializeAsString());
	boost::asio::async_write(socket_, boost::asio::buffer(buf), [this](std::error_code ec, std::size_t length) {
		if (!ec) {
			sentList_.push_back(pendingList_.front());
			pendingList_.pop_front();
			if (!pendingList_.empty()) {
				onWrite();
			}
		} else {
			socket_.close();
		}
	});
}