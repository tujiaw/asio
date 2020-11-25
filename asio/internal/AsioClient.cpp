#include "AsioClient.h"
#include "internal/ProtoHelp.h"
#include "ThreadPool.h"
#include "tool/util.h"
#include "tool/strutil.h"
#include "proto/pb_base.pb.h"
#include <boost/bind.hpp>

namespace ningto
{
    RequestMessage::Data::Data(boost::asio::io_service &io, const PackagePtr &p, const Response &r, int msTimeout, const std::function<void(int)> &cb)
        : pac(p), res(r), reqTime(util::currentMillisecond()), onTimeout(cb), timer(io, boost::posix_time::milliseconds(msTimeout))
    {
    }

    RequestMessage::RequestMessage(boost::asio::io_service &io)
        : io_(io)
    {
    }

    void RequestMessage::put(const PackagePtr &p, const Response &r, int msTimeout)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[p->header.msgId] = std::make_shared<RequestMessage::Data>(io_, p, r, msTimeout,
            std::bind(&RequestMessage::onTimeout, this, std::placeholders::_1));
    }

    void RequestMessage::startTimer(int msgid)
    {
        DataPtr data = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = cache_.find(msgid);
            if (it != cache_.end()) {
                data = it->second;
            }
        }
        if (data) {
            data->writeFinishedTime = util::currentMillisecond();
            data->timer.async_wait([msgid, this](boost::system::error_code err) {
                if (err == boost::asio::error::operation_aborted) {
                    return;
                }
                onTimeout(msgid);
            });
        }
    }

    RequestMessage::DataPtr RequestMessage::pick(int msgid)
    {
        // 应答消息
        DataPtr rsp = nullptr;
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(msgid);
        if (it != cache_.end()) {
            rsp = it->second;
            rsp->rspTime = util::currentMillisecond();
            cache_.erase(it);
        } else {
            LOG(ERROR) << "onRead id not find:" << msgid;
        }
        return rsp;
    }

    void RequestMessage::onTimeout(int msgId)
    {
        DataPtr p = pick(msgId);
        if (p) {
            p->res(eTimeout, p->pac, PackagePtr());
        }
    }

    //////////////////////////////////////////////////////////////////////////
    AsioClient::AsioClient(const std::vector<std::string> &addressList, int reconnectSeconds, int heartbeatSeconds)
        : addressList_(addressList),
        workDoResponse_(ioDoResponse_),
        socket_(io_),
        reconnectSeconds_(reconnectSeconds),
        heartbeatSeconds_(heartbeatSeconds),
        requestMessage_(io_)
    {
        LOG(INFO) << "client create, address list:" << strutil::FromVector(addressList, "; ") << ", reconnect seconds:" << reconnectSeconds
            << ", heartbeat seconds:" << heartbeatSeconds;
    }

    AsioClient::~AsioClient()
    {
        stop();
    }

    void AsioClient::start()
    {
        connect();
        if (!runthread_.joinable()) {
            runthread_ = std::thread(boost::bind(&boost::asio::io_service::run, &io_));
        }
        if (!threadDoResponse_.joinable()) {
            threadDoResponse_ = std::thread(boost::bind(&boost::asio::io_service::run, &ioDoResponse_));
        }
    }

    void AsioClient::addHandler(const HandlerPtr &handler)
    {
        handlerList_.push_back(handler);
    }

    void AsioClient::setConnectSuccess(const std::function<void()> &cb)
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

    bool AsioClient::waitConnected(int timeoutSeconds) const
    {
        LOG(INFO) << "wait connected start";
        const int MS_INTERVAL = 100;
        time_t start = util::currentMillisecond();
        int cost = 0;
        while (!isOnline_ && cost < timeoutSeconds * 1000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(MS_INTERVAL));
            cost += MS_INTERVAL;
        }

        LOG(INFO) << "wait connected end, status:" << (isOnline_.load() ? "online" : "offline") << ", cost:" << (util::currentMillisecond() - start) << "ms";
        return isOnline_;
    }

    void AsioClient::close()
    {
        io_.post(std::bind(&AsioClient::doClose, this));
    }

    int AsioClient::sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout)
    {
        bool isWait = true;
        int err = 0;
        int code = postMessage(msgPtr, [this, &rspPtr, &isWait, &err](int error, const PackagePtr &/*reqMsgPtr*/, const PackagePtr &rspMsgPtr) {
            if (!error) {
                rspPtr = rspMsgPtr->msgPtr;
            }
            err = error;
            std::unique_lock<std::mutex> lock(mutex_);
            isWait = false;
            this->cond_.notify_all();
        }, msTimeout);

        if (code != eSuccess) {
            return code;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        while (isWait) {
            cond_.wait(lock);
        }
        return err;
    }

    void AsioClient::connect()
    {
        if (addressIndex_ >= addressList_.size()) {
            addressIndex_ = 0;
        }

        LOG(INFO) << "connect to:" << addressList_[addressIndex_];
        int pos = addressList_[addressIndex_].find(":");
        assert(pos > 0);
        tcp::resolver resolver(io_);
        tcp::resolver::query query(addressList_[addressIndex_].substr(0, pos), addressList_[addressIndex_].substr(pos + 1));
        tcp::resolver::iterator endpoint = resolver.resolve(query);
        boost::asio::async_connect(socket_, endpoint, std::bind(&AsioClient::doConnect, this, std::placeholders::_1));
    }

    void AsioClient::doConnect(const boost::system::error_code &ec)
    {
        LOG(INFO) << "connect " << ec.message();
        isOnline_ = !ec;
        if (!ec) {
            startRead();
            startSubscribe();
            startHeartbeatTimer();
            if (connectSuccess_) {
                connectSuccess_();
            }
        } else {
            LOG(INFO) << "connect failed:" << ec.message();
            if (reconnectSeconds_ > 0) {
                std::this_thread::sleep_for(std::chrono::seconds(reconnectSeconds_));
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

    void AsioClient::publishMessage(const MessagePtr &msgPtr)
    {
        auto pacPtr = std::make_shared<Package>();
        pacPtr->header.msgType = PacHeader::PUBSUB;
        pacPtr->header.msgId = ++id_ % INT_MAX;
        pacPtr->header.typeNameLen = (short)msgPtr->GetTypeName().length();
        pacPtr->header.isorder = 0;
        pacPtr->typeName = msgPtr->GetTypeName();
        pacPtr->msgPtr = msgPtr;
        postPackage(pacPtr);
    }

    int AsioClient::postMessage(bool isOrder, const MessagePtr &msgPtr, const Response &res, int msTimeout)
    {
        auto pacPtr = std::make_shared<Package>();
        pacPtr->header.msgType = PacHeader::REQREP;
        pacPtr->header.msgId = ++id_ % INT_MAX;
        pacPtr->header.typeNameLen = (short)msgPtr->GetTypeName().length();
        pacPtr->header.isorder = isOrder ? 1 : 0;
        pacPtr->typeName = msgPtr->GetTypeName();
        pacPtr->msgPtr = msgPtr;

        requestMessage_.put(pacPtr, res, msTimeout);
        return postPackage(pacPtr);
    }

    int AsioClient::postPackage(const PackagePtr &package)
    {
        if (!package) {
            LOG(ERROR) << "params error";
            return eParams;
        }

        if (io_.stopped()) {
            LOG(WARNING) << "replyDeliver failed, io stopped";
            return eServiceStopped;
        }
        if (!isOnline_) {
            LOG(WARNING) << "replyDeliver failed, disconnect";
            return eDisconnect;
        }

        io_.post(boost::bind(&AsioClient::startWrite, this, package));
        return eSuccess;
    }

    void AsioClient::startRead()
    {
        socket_.async_read_some(boost::asio::buffer(tempBuf_, kTempBufSize),
            std::bind(&AsioClient::doRead, this, std::placeholders::_1, std::placeholders::_2));
    }

    void AsioClient::doRead(boost::system::error_code ec, std::size_t length)
    {
        if (ec) {
            LOG(ERROR) << "onRead error:" << ec.message();
            this->close();
            return;
        }

        readBuffer_.append(tempBuf_, length);
        do {
            PackagePtr pack = ProtoHelp::decode(readBuffer_);
            if (pack) {
                ioDoResponse_.post(std::bind(&AsioClient::doResponse, this, pack));
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
        PackagePtr pacPtr = pendingList_.front();
        auto writeBuffer = std::make_shared<Buffer>();
        if (ProtoHelp::encode(pacPtr, writeBuffer)) {
            int msgid = pacPtr->header.msgId;
            boost::asio::async_write(socket_,
                boost::asio::buffer(writeBuffer->peek(), writeBuffer->readableBytes()),
                [this, writeBuffer, msgid](boost::system::error_code ec, std::size_t /*length*/) {
                requestMessage_.startTimer(msgid);
                // 必须捕获writeBuffer确保在应答之前对象没有被销毁，否则async_write会失败
                if (!ec) {
                    pendingList_.pop_front();
                    if (!pendingList_.empty()) {
                        startWrite();
                    }
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
        } catch (const boost::system::system_error &err) {
            LOG(ERROR) << "do close exception:" << err.what();
        }

        isOnline_ = false;
        if (!io_.stopped() && reconnectSeconds_ > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(reconnectSeconds_));
            connect();
        }
    }

    void AsioClient::startSubscribe()
    {
        if (handlerList_.empty()) {
            return;
        }

        auto msg = new ProtoBase::SubscribeReq();
        msg->set_type(1);

        for (auto it = handlerList_.begin(); it != handlerList_.end(); ++it) {
            const auto &handler = *it;
            std::vector<std::string> typeNames = handler->typeNames();
            int type = handler->type();
            for (auto it2 = typeNames.begin(); it2 != typeNames.end(); ++it2) {
                msg->add_msgtypelist(type);
                msg->add_typenamelist(*it2);
            }
        }

        postMessage(MessagePtr(msg), [](int error, const PackagePtr &/*reqMsgPtr*/, const PackagePtr &rspMsgPtr) {
            if (error == 0) {
                const ProtoBase::SubscribeRsp *p = static_cast<ProtoBase::SubscribeRsp*>(rspMsgPtr->msgPtr.get());
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

    void AsioClient::startHeartbeatTimer()
    {
        if (heartbeatSeconds_ > 0) {
            heartbeatTimer_.reset(new boost::asio::deadline_timer(io_, boost::posix_time::seconds(heartbeatSeconds_)));
            heartbeatTimer_->async_wait(std::bind(&AsioClient::doHeartbeat, this, std::placeholders::_1));
        }
    }

    void AsioClient::doHeartbeat(const boost::system::error_code &e)
    {
        if (e) {
            LOG(ERROR) << "doHeartbeat error:" << e.message();
            return;
        }

        auto msg(new ProtoBase::HeartbeatReq());
        msg->set_cpu(0);
        msg->set_memory(0);
        postMessage(MessagePtr(msg), [this](int, const PackagePtr&, const PackagePtr&) {
            heartbeatTimer_->expires_at(heartbeatTimer_->expires_at() + boost::posix_time::seconds(heartbeatSeconds_));
            heartbeatTimer_->async_wait(std::bind(&AsioClient::doHeartbeat, this, std::placeholders::_1));
        }, 10 * 1000);
    }

    void AsioClient::doResponse(const PackagePtr &pack)
    {
        const short msgType = pack->header.msgType;
        const std::string &typeName = pack->typeName;
        const int msgId = pack->header.msgId;

        if (msgType == PacHeader::REQREP) {
            // 应答消息
            auto rsp = requestMessage_.pick(msgId);
            if (rsp) {
                time_t cost = rsp->rspTime - rsp->writeFinishedTime;
                LOG(INFO) << "doResponse msgType:" << msgType << ", typeName:" << typeName << ", msgId:" << msgId << ", cost:" << cost << "ms";
                rsp->res(eSuccess, rsp->pac, pack);
            }
        } else {
            LOG(INFO) << "doResponse msgType:" << msgType << ", typeName:" << typeName << ", msgId:" << msgId;
            for (auto it = handlerList_.cbegin(); it != handlerList_.cend(); ++it) {
                const HandlerPtr &handler = *it;
                handler->onPackage(pack);
            }
        }
    }
}