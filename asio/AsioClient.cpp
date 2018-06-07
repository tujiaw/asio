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
	PackagePtr pacPtr(new Package);
	pacPtr->id = ++id_;
	pacPtr->typeName = msgPtr->GetTypeName();
	pacPtr->typeNameLen = msgPtr->GetTypeName().length();
	pacPtr->msgPtr = msgPtr;
	responseMap_[pacPtr] = res;
	io_.post([this, pacPtr] {
		bool isProgress = !pendingList_.empty();
		pendingList_.push_back(pacPtr);
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
		if (ec) {
			return;
		}
		
		readBuffer_.append(tempBuf_, length);
		PackagePtr pack = ProtoHelp::decode(readBuffer_);
		if (pack) {
			for (auto iter = responseMap_.begin(); iter != responseMap_.end(); ++iter) {
				if (iter->first->id == pack->id) {
					Response resFunc = iter->second;
					resFunc(0, iter->first, pack);
					responseMap_.erase(iter);
					break;
				}
			}
		}

		onRead();
	});
}

void AsioClient::onWrite()
{
	if (pendingList_.empty()) {
		return;
	}

	PackagePtr pacPtr = pendingList_.front();
	std::string buf = ProtoHelp::encode(pacPtr);
	if (buf.size() > 0) {
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
}