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
				onRead();
			}
			std::cout << "connect " << ec.message() << std::endl;
		});
		runthread_ = std::move(std::thread([this](){ io_.run(); }));
	}
}
AsioClient::~AsioClient()
{
}


void AsioClient::run()
{
}

void AsioClient::stop()
{
	if (!io_.stopped()) {
		io_.stop();
		runthread_.join();
	}
}

void AsioClient::close()
{
	io_.post(std::bind(&AsioClient::doClose, this));
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
	responseMap_[pacPtr->id] = std::make_pair(pacPtr, res);
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
			std::cout << "onRead error: " << ec;
			return;
		}
		
		readBuffer_.append(tempBuf_, length);

		do {
			PackagePtr pack = ProtoHelp::decode(readBuffer_);
			if (pack) {
				auto it = responseMap_.find(pack->id);
				if (it != responseMap_.end()) {
					Response resFunc = it->second.second;
					resFunc(0, it->second.first, pack);
					responseMap_.erase(it);
				} else {
					std::cout << "onRead id not find:" << pack->id << std::endl;
				}
			} else {
				break;
			}
		} while (1);

		onRead();
	});
}

void AsioClient::onWrite()
{
	if (pendingList_.empty()) {
		return;
	}

	PackagePtr pacPtr = pendingList_.front();
	pendingList_.pop_front();
	BufferPtr writeBuffer = ProtoHelp::encode(pacPtr);
	if (writeBuffer) {
		boost::asio::async_write(socket_, boost::asio::buffer(writeBuffer->peek(), writeBuffer->readableBytes()), 
			[this](std::error_code ec, std::size_t length) {
			if (!ec) {
				std::cout << "write length:" << length << std::endl;
				onWrite();
			} else {
				this->close();
			}
		});
	}
}

void AsioClient::doClose()
{
	socket_.close();
}