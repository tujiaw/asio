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
			LOG(INFO) << "connect " << ec.message();
		});
		runthread_ = std::move(std::thread([this](){ io_.run(); }));
	}
}
AsioClient::~AsioClient()
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

int AsioClient::sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout)
{
	int errorCode = ErrorCode::Unkown;
	bool isWait = true;
	postMessage(msgPtr, [&](int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr) {
		errorCode = error;
		if (error == 0) {
			rspPtr = rspMsgPtr->msgPtr;
		} 
		std::unique_lock<std::mutex> lock(mutex_);
		isWait = false;
		this->cond_.notify_all();
	});

	std::unique_lock<std::mutex> lock(mutex_);
	while (isWait) {
		std::_Cv_status status = cond_.wait_for(lock, std::chrono::milliseconds(msTimeout));
		if (status == std::_Cv_status::timeout) {
			errorCode = ErrorCode::Timeout;
			break;
		}
	}
	return errorCode;
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
			LOG(ERROR) << "onRead error:" << ec.message();
			return;
		}
		
		readBuffer_.append(tempBuf_, length);

		do {
			PackagePtr pack = ProtoHelp::decode(readBuffer_);
			if (pack) {
				LOG(INFO) << "response id:" << pack->id;
				auto it = responseMap_.find(pack->id);
				if (it != responseMap_.end()) {
					Response resFunc = it->second.second;
					resFunc(0, it->second.first, pack);
					responseMap_.erase(it);
				} else {
					LOG(ERROR) << "onRead id not find:" << pack->id;
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
				onWrite();
			} else {
				LOG(ERROR) << "close " << ec.message();
				this->close();
			}
		});
	}
}

void AsioClient::doClose()
{
	socket_.close();
}