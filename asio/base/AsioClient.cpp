#include "AsioClient.h"
#include "threadpool.h"
#include "ProtoHelp.h"
#include "util.h"
#include "asio/proto/pb_base.pb.h"

AsioClient::AsioClient(const std::string &address, int heartbeatSeconds)
    : address_(address), socket_(io_), id_(0), heartbeatSeconds_(heartbeatSeconds), isOnline_(false), writeBuffer_(new Buffer())
{
}

AsioClient::~AsioClient()
{
	stop();
}

void AsioClient::start()
{
	LOG(INFO) << "connect to:" << address_;
	tcp::resolver resolver(io_);
	int pos = address_.find(":");
	assert(pos > 0);
	auto endpoints = resolver.resolve({ address_.substr(0, pos), address_.substr(pos + 1) });
	boost::asio::async_connect(socket_, endpoints, [this](std::error_code ec, tcp::resolver::iterator) {
		LOG(INFO) << "connect " << ec.message();
		isOnline_ = !ec;
		if (!ec) {
			onRead();
			doSubscribe();
			onHeartbeat();
		}
	});

	if (!runthread_.joinable()) {
		runthread_ = std::move(std::thread([this](){ io_.run(); }));
	}
}

void AsioClient::addHandlePublish(const std::string &typeName, const PublishFunc &func)
{
	publishMap_[typeName] = func;
}

void AsioClient::stop()
{
    io_.stop();
    close();
	if (runthread_.joinable()) {
		runthread_.join();
	}
}

bool AsioClient::stopped() const
{
	return io_.stopped();
}

void AsioClient::close()
{
	io_.post(std::bind(&AsioClient::doClose, this));
}

int AsioClient::sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout)
{
	bool isWait = true;
	int err = postMessage(msgPtr, [&](int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr) {
		err = error;
		if (error == 0) {
			rspPtr = rspMsgPtr->msgPtr;
		} 
		std::unique_lock<std::mutex> lock(mutex_);
		isWait = false;
		this->cond_.notify_all();
	});
	if (err != eSuccess) {
		return (int)err;
	}

	std::unique_lock<std::mutex> lock(mutex_);
	while (isWait) {
		std::_Cv_status status = cond_.wait_for(lock, std::chrono::milliseconds(msTimeout));
		if (status == std::_Cv_status::timeout) {
			err = eTimeout;
			break;
		}
	}
	return err;
}

int AsioClient::postMessage(const MessagePtr &msgPtr, const Response &res)
{
	if (io_.stopped()) {
		return eServiceStopped;
	}
	if (!isOnline_) {
		return eDisconnect;
	}

	PackagePtr pacPtr(new Package);
	pacPtr->id = ++id_;
	pacPtr->typeName = msgPtr->GetTypeName();
	pacPtr->typeNameLen = msgPtr->GetTypeName().length();
	pacPtr->msgPtr = msgPtr;

	{
		std::unique_lock<std::mutex> lock(responseMutex_);
		responseMap_[pacPtr->id] = std::make_pair(pacPtr, res);
	}

	io_.post([this, pacPtr] {
		bool isProgress = !pendingList_.empty();
		pendingList_.push_back(pacPtr);
		if (!isProgress) {
			onWrite();
		}
	});
	return eSuccess;
}

void AsioClient::onRead()
{
	socket_.async_read_some(boost::asio::buffer(tempBuf_, kTempBufSize),
		[this](boost::system::error_code ec, std::size_t length)
	{
		if (ec) {
			LOG(ERROR) << "onRead error:" << ec.message();
			close();
			return;
		}
		
        if (length != 40) {
            LOG(INFO) << "onRead:" << length;
        }
        
		readBuffer_.append(tempBuf_, length);

		do {
			PackagePtr pack = ProtoHelp::decode(readBuffer_);
			if (pack) {
				if (pack->id == -1) {
					// ������Ϣ
					auto it = publishMap_.find(pack->typeName);
					if (it != publishMap_.end()) {
						it->second(0, pack->msgPtr);
					} else {
						LOG(WARNING) << "onRead publish not found:" << pack->typeName;
					}
				} else {
					// Ӧ����Ϣ
					std::pair<PackagePtr, Response> rsp;
					{
						std::unique_lock<std::mutex> lock(responseMutex_);
						auto it = responseMap_.find(pack->id);
						if (it != responseMap_.end()) {
							rsp = it->second;
							responseMap_.erase(it);
						} else {
							LOG(ERROR) << "onRead id not find:" << pack->id;
						}
					}
					if (rsp.first && rsp.second) {
						rsp.second(0, rsp.first, pack);
					}
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
    if (ProtoHelp::encode(pacPtr, writeBuffer_)) {
        boost::asio::async_write(socket_, boost::asio::buffer(writeBuffer_->peek(), writeBuffer_->readableBytes()),
            [this](std::error_code ec, std::size_t length) {
            if (!ec) {
                LOG(INFO) << "onWrite:" << length;
                writeBuffer_->retrieve(length);
                onWrite();
            } else {
                LOG(ERROR) << "async write error: " << ec.message();
                this->close();
            }
        });
    }
}

void AsioClient::doClose()
{
	try {
		if (socket_.is_open()) {
			socket_.shutdown(boost::asio::socket_base::shutdown_both);
			socket_.close();
		}
	}
	catch (std::exception &e) {
		LOG(ERROR) << "do close exception:" << e.what();
	}
	isOnline_ = false;
}

void AsioClient::doSubscribe()
{
	PbBase::SubscribeReq *msg(new PbBase::SubscribeReq());
	for (auto it = publishMap_.begin(); it != publishMap_.end(); ++it) {
		msg->add_typenamelist(it->first);
	}
	msg->set_type(1);
	postMessage(MessagePtr(msg), [](int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr) {
		if (error == 0) {
			PbBase::SubscribeRsp *p = static_cast<PbBase::SubscribeRsp*>(rspMsgPtr->msgPtr.get());
			if (p->errorcode() == 0) {
				LOG(INFO) << "subscribe success...";
			} else {
				LOG(ERROR) << "subscribe failed, response error:" << p->errorcode();
			}
		} else {
			LOG(ERROR) << "subscribe failed, error:" << error;
		}
	});
}

void AsioClient::onHeartbeat()
{
	if (heartbeatSeconds_ > 0) {
		heartbeatTimer_.reset(new boost::asio::deadline_timer(io_, boost::posix_time::seconds(heartbeatSeconds_)));
		heartbeatTimer_->async_wait(std::bind(&AsioClient::doHeartbeat, this, std::placeholders::_1));
	}
}

void AsioClient::doHeartbeat(const boost::system::error_code &e)
{
	if (0 != e) {
		return;
	}

	PbBase::HeartbeatReq *msg(new PbBase::HeartbeatReq());
	msg->set_cpu(0);
	msg->set_memory(0);
	postMessage(MessagePtr(msg), [&](int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr) {
		if (error == 0) {
			PbBase::HeartbeatRsp *rsp = static_cast<PbBase::HeartbeatRsp*>(rspMsgPtr->msgPtr.get());
			LOG(INFO) << "heartbeat " << util::getFormatTime(rsp->servertime());

			heartbeatTimer_->expires_at(heartbeatTimer_->expires_at() + boost::posix_time::seconds(heartbeatSeconds_));
			heartbeatTimer_->async_wait(std::bind(&AsioClient::doHeartbeat, this, std::placeholders::_1));
		}
	});
}