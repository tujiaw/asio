#include "AsioClient.h"
#include "ThreadPool.h"
#include "ProtoHelp.h"
#include "proto/pb_base.pb.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>

AsioClient::AsioClient(const std::vector<std::string> &addressList, int reconnectSeconds, int heartbeatSeconds)
    : addressIndex_(0),
    addressList_(addressList),
    socket_(io_), 
    id_(0), 
    reconnectSeconds_(reconnectSeconds),
    heartbeatSeconds_(heartbeatSeconds), 
    isOnline_(false), 
    workDoResponse_(ioDoResponse_)
{
}

AsioClient::~AsioClient()
{
	stop();
}

void AsioClient::start()
{
    connect();
	if (!runthread_.joinable()) {
        runthread_ = boost::move(boost::thread(boost::bind(&boost::asio::io_service::run, &io_)));
	}
    if (!threadDoResponse_.joinable()) {
        threadDoResponse_ = boost::move(boost::thread(boost::bind(&boost::asio::io_service::run, &ioDoResponse_)));
    }
}

void AsioClient::addHandlePublish(const std::string &typeName, const PublishFunc &func)
{
	publishMap_[typeName] = func;
}

void AsioClient::setConnectSuccess(const boost::function<void()> &cb)
{
    connectSuccess_ = cb;
}

void AsioClient::stop()
{
    io_.stop();
    ioDoResponse_.stop();
    close();
	if (runthread_.joinable()) {
		runthread_.join();
	}
    if (threadDoResponse_.joinable()) {
        threadDoResponse_.join();
    }
}

bool AsioClient::stopped() const
{
	return io_.stopped();
}

void AsioClient::close()
{
	io_.post(boost::bind(&AsioClient::doClose, this));
}

int AsioClient::sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout)
{
#if 0 // c++11
	bool isWait = true;
	int err = postMessage(msgPtr, [&](int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr) {
        if (!error) {
            rspPtr = rspMsgPtr->msgPtr;
        }
		err = error;
		boost::unique_lock<boost::mutex> lock(mutex_);
		isWait = false;
		this->cond_.notify_all();
	}, msTimeout);

	if (err != eSuccess) {
		return (int)err;
	}

    boost::unique_lock<boost::mutex> lock(mutex_);
	while (isWait) {
        cond_.wait(lock);
	}
	return err;
#else
    return -1;
#endif
}

void AsioClient::connect()
{
    if (addressIndex_ >= addressList_.size()) {
        addressIndex_ = 0;
    }

    DLOG(INFO) << "connect to:" << addressList_[addressIndex_];
    int pos = addressList_[addressIndex_].find(":");
    assert(pos > 0);
    tcp::resolver resolver(io_);
    tcp::resolver::query query(addressList_[addressIndex_].substr(0, pos), addressList_[addressIndex_].substr(pos + 1));
    tcp::resolver::iterator endpoint = resolver.resolve(query);
    boost::asio::async_connect(socket_, endpoint, boost::bind(&AsioClient::doConnect, this, boost::asio::placeholders::error));
}

void AsioClient::doConnect(const boost::system::error_code &ec)
{
    DLOG(INFO) << "connect " << ec.message();
    isOnline_ = !ec;
    if (!ec) {
        startRead();
        startSubscribe();
        startHeartbeatTimer();
        if (connectSuccess_) {
            connectSuccess_();
        }
    } else {
        DLOG(INFO) << "connect failed:" << ec.message();
        if (reconnectSeconds_ > 0) {
            boost::this_thread::sleep_for(boost::chrono::seconds(reconnectSeconds_));
            ++addressIndex_;
            connect();
        }
    }
}

int AsioClient::postMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout)
{
    return postMessage(false, msgPtr, res, msTimeout);
}

int AsioClient::postOrderMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout)
{
	return postMessage(true, msgPtr, res, msTimeout);
}

int AsioClient::postMessage(bool isOrder, const MessagePtr &msgPtr, const Response &res, int msTimeout)
{
    if (io_.stopped()) {
        return eServiceStopped;
    }
    if (!isOnline_) {
        return eDisconnect;
    }

    PackagePtr pacPtr(new Package);
    pacPtr->header.msgType = PacHeader::REQREP;
    pacPtr->header.msgId = ++id_ % INT_MAX;
    pacPtr->header.typeNameLen = msgPtr->GetTypeName().length();
    pacPtr->header.isorder = isOrder ? 1 : 0;
    pacPtr->typeName = msgPtr->GetTypeName();
    pacPtr->msgPtr = msgPtr;

    {
        boost::mutex::scoped_lock lock(responseMutex_);
        responseMap_[pacPtr->header.msgId] = MsgCachePtr(new MsgCache(this, pacPtr, res, msTimeout));
    }

    io_.post(boost::bind(&AsioClient::startWrite, this, pacPtr));
    return eSuccess;
}

void AsioClient::startRead()
{
    socket_.async_read_some(boost::asio::buffer(tempBuf_, kTempBufSize),
        boost::bind(&AsioClient::doRead, this, _1, _2));
}

void AsioClient::doRead(boost::system::error_code ec, std::size_t length)
{
    if (ec) {
        DLOG(ERROR) << "onRead error:" << ec.message();
        this->close();
        return;
    }

    readBuffer_.append(tempBuf_, length);
    do {
        PackagePtr pack = ProtoHelp::decode(readBuffer_);
        if (pack) {
            ioDoResponse_.post(boost::bind(&AsioClient::doResponse, this, pack));
        } else {
            break;
        }
    } while (readBuffer_.readableBytes() > 0);

    startRead();
}

void AsioClient::startWrite(const PackagePtr &pacPtr)
{
    bool isProgress = !pendingList_.empty();
    pendingList_.push_back(pacPtr);
    if (!isProgress) {
        startWrite();
    }
}

void AsioClient::startWrite()
{
	if (pendingList_.empty()) {
		return;
	}

	PackagePtr pacPtr = pendingList_.front();
	pendingList_.pop_front();
    BufferPtr writeBuffer(new Buffer());
    if (ProtoHelp::encode(pacPtr, writeBuffer)) {
        boost::asio::async_write(socket_, 
            boost::asio::buffer(writeBuffer->peek(), 
            writeBuffer->readableBytes()),
            boost::bind(&AsioClient::doWrite, this, _1, _2));
    }
}

void AsioClient::doWrite(boost::system::error_code ec, std::size_t length)
{
    if (!ec) {
        startWrite();
    } else {
        DLOG(ERROR) << "async write error: " << ec.message();
        this->close();
    }
}

void AsioClient::onTimeout(boost::system::error_code err, int msgId)
{
    if (err == boost::asio::error::operation_aborted) {
        return;
    }

    MsgCachePtr rsp;
    {
        boost::mutex::scoped_lock lock(responseMutex_);
        auto it = responseMap_.find(msgId);
        if (it != responseMap_.end()) {
			DLOG(ERROR) << "on timeout, msgid:" << msgId;
            rsp = it->second;
            responseMap_.erase(it);
        } else {
            DLOG(ERROR) << "onTimeout id not find:" << msgId;
        }
    }
    if (rsp) {
        rsp->res(eTimeout, rsp->pac, PackagePtr());
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
		DLOG(ERROR) << "do close exception:" << e.what();
	}
	isOnline_ = false;

    if (!io_.stopped()) {
        if (reconnectSeconds_ > 0) {
            boost::this_thread::sleep_for(boost::chrono::seconds(reconnectSeconds_));
            connect();
        }
    }
}

void AsioClient::startSubscribe()
{
	PbBase::SubscribeReq *msg(new PbBase::SubscribeReq());
	for (auto it = publishMap_.begin(); it != publishMap_.end(); ++it) {
		msg->add_typenamelist(it->first);
	}
	msg->set_type(1);
	postMessage(MessagePtr(msg), boost::bind(&AsioClient::doSubscribe, this, _1, _2, _3));
}

void AsioClient::doSubscribe(int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr)
{
    if (error == 0) {
        PbBase::SubscribeRsp *p = static_cast<PbBase::SubscribeRsp*>(rspMsgPtr->msgPtr.get());
        if (p->errorcode() == 0) {
            DLOG(INFO) << "subscribe success...";
        } else {
            DLOG(ERROR) << "subscribe failed, response error:" << p->errorcode();
        }
    } else {
        DLOG(ERROR) << "subscribe failed, error:" << error;
    }
}

void AsioClient::startHeartbeatTimer()
{
	if (heartbeatSeconds_ > 0) {
		heartbeatTimer_.reset(new boost::asio::deadline_timer(io_, boost::posix_time::seconds(heartbeatSeconds_)));
		heartbeatTimer_->async_wait(std::bind(&AsioClient::startHeartbeat, this, std::placeholders::_1));
	}
}

void AsioClient::startHeartbeat(const boost::system::error_code &e)
{
	if (e) {
        DLOG(ERROR) << "doHeartbeat error:" << e.message();
		return;
	}

	PbBase::HeartbeatReq *msg(new PbBase::HeartbeatReq());
	msg->set_cpu(0);
	msg->set_memory(0);
	postMessage(MessagePtr(msg), boost::bind(&AsioClient::doHeartbeat, this, _1, _2, _3));
}

void AsioClient::doHeartbeat(int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr)
{
    if (error == 0) {
        PbBase::HeartbeatRsp *rsp = static_cast<PbBase::HeartbeatRsp*>(rspMsgPtr->msgPtr.get());
        DLOG(INFO) << "heartbeat " << rsp->servertime();
    }
    heartbeatTimer_->expires_at(heartbeatTimer_->expires_at() + boost::posix_time::seconds(heartbeatSeconds_));
    heartbeatTimer_->async_wait(std::bind(&AsioClient::startHeartbeat, this, std::placeholders::_1));
}

void AsioClient::doResponse(const PackagePtr &pack)
{
    if (pack->header.msgType == PacHeader::PUBSUB) {
        // 推送消息
        std::map<std::string, PublishFunc>::iterator it = publishMap_.find(pack->typeName);
        if (it != publishMap_.end()) {
            it->second(0, pack->msgPtr);
        } else {
            DLOG(WARNING) << "onRead publish not found:" << pack->typeName;
        }
    } else {
        // 应答消息
        MsgCachePtr rsp;
        {
            boost::mutex::scoped_lock lock(responseMutex_);
            std::map<int, MsgCachePtr>::iterator it = responseMap_.find(pack->header.msgId);
            if (it != responseMap_.end()) {
                rsp = it->second;
                responseMap_.erase(it);
            } else {
                DLOG(ERROR) << "onRead id not find:" << pack->header.msgId;
            }
        }
        if (rsp) {
            rsp->res(eSuccess, rsp->pac, pack);
        }
    }
}
