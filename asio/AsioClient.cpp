#include "AsioClient.h"
#include "threadpool.h"
#include "ProtoHelp.h"

AsioClient::AsioClient(const std::string &address)
	: socket_(io_), id_(0)
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
	//responseMap_[msgPtr] = res;
	io_.post([this, msgPtr] {
		bool isProgress = !pendingList_.empty();
		pendingList_.push_back(msgPtr);
		if (!isProgress) {
			onWrite();
		}
	});
}

void AsioClient::onRead()
{
	socket_.async_read_some(boost::asio::buffer(tempBuf_, kTempBufSize),
		[this](boost::system::error_code ec, std::size_t length)
	{
		if (!ec) {
			readBuffer_.append(tempBuf_, length);
			if (readBuffer_.readableBytes() > 4) {
				int packLen = readBuffer_.peekInt32();
				if (packLen <= 0) {
					readBuffer_.retrieve(1);
				} else if (packLen > 1024 * 1024 * 10) {
					readBuffer_.retrieve(1);
				} else if (readBuffer_.readableBytes() >= (size_t)packLen + 4) {
					std::string packBuf;
					packBuf.resize(packLen);
					memcpy(&packBuf[0], readBuffer_.peek() + 4, packLen);
					PackagePtr packPtr = ProtoHelp::decode(packBuf);
					if (packPtr) {
						readBuffer_.retrieve(packLen + 4);
						//std::shared_ptr<google::protobuf::Message> msgPtr(msg);
						//TaskManager::instance()->handleMessage(msgPtr, self);
						//responseMap_[0](0, )
					} else {
						readBuffer_.retrieve(1);
					}
				}
			}

			onRead();
		}
	});
}

void AsioClient::onWrite()
{
	std::string buf = std::move(pendingList_.front()->SerializeAsString());
	boost::asio::async_write(socket_, boost::asio::buffer(buf), [this](std::error_code ec, std::size_t length) {
		if (!ec) {
			pendingList_.pop_front();
			if (!pendingList_.empty()) {
				onWrite();
			}
		} else {
			socket_.close();
		}
	});
}