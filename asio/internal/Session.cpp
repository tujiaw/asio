#include "Session.h"
#include <deque>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include "ProtoHelp.h"
#include "Buffer.h"
#include "TaskManager.h"
#include "tool/util.h"

using boost::asio::ip::tcp;

namespace ningto
{
    static const int kTempBufSize = boost::asio::detail::default_max_transfer_size;
    std::string endpoint2str(const tcp::endpoint &endpoint)
    {
        std::string host = endpoint.address().to_string();
        host += (":" + boost::lexical_cast<std::string>(endpoint.port()));
        return host;
    }

    struct SessionData {
        explicit SessionData(tcp::socket socket)
            : socket_(std::move(socket))
            , io_(socket_.get_io_service())
            , sessionId_(s_sessionIdInc++)
        {
        }

        tcp::socket socket_;
        boost::asio::io_service &io_;
        char tempBuf_[kTempBufSize];
        Buffer readBuffer_;
        std::deque<BufferPtr> writeDeque_;
        boost::atomic<int> id_{ 0 };
        int sessionId_{ 0 };
        static boost::atomic<int> s_sessionIdInc;
    };
    boost::atomic<int> SessionData::s_sessionIdInc(1);

    Session::Session(tcp::socket socket, SessionRoom &room, TaskManager &taskManager)
        : d(util::make_unique<SessionData>(std::move(socket))), room_(room), taskManager_(taskManager)
    {
    }

    Session::~Session()
    {
        LOG(INFO) << "session destory, address:" << endpoint_;
    }

    void Session::start()
    {
        endpoint_ = remoteEndpoint();
        LOG(INFO) << "session start, remoteEndpoint:" << endpoint_;
        room_.join(shared_from_this());
        doRead();
    }

    void Session::replyMessage(const PackagePtr &req, const MessagePtr &rspMsg)
    {
        if (!req || !rspMsg) {
            return;
        }
        auto rspPtr = std::make_shared<Package>();
        rspPtr->header = req->header;
        rspPtr->typeName = rspMsg->GetTypeName();
        rspPtr->msgPtr = rspMsg;
        postPackage(rspPtr);
    }

    void Session::publishMessage(const MessagePtr &msg)
    {
        if (!msg) {
            return;
        }

        auto pack = std::make_shared<Package>();
        pack->header.msgType = PacHeader::PUBSUB;
        pack->header.msgId = ++d->id_ % INT_MAX;
        pack->header.typeNameLen = (short)msg->GetTypeName().length();
        pack->typeName = msg->GetTypeName();
        pack->msgPtr = msg;
        postPackage(pack);
    }

    void Session::deliverPackage(const PackagePtr &pac)
    {
        postPackage(pac);
    }

    std::string Session::remoteEndpoint() const
    {
        if (d->socket_.is_open()) {
            try {
                return endpoint2str(d->socket_.remote_endpoint());
            } catch (const boost::system::system_error &err) {
                LOG(WARNING) << "remoteEndpoint system error:" << err.what();
            }
        }
        return "";
    }

    int Session::id() const
    {
        return d->sessionId_;
    }

    void Session::doRead()
    {
        auto self(shared_from_this());
        d->socket_.async_read_some(boost::asio::buffer(d->tempBuf_, kTempBufSize),
            [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                d->readBuffer_.append(d->tempBuf_, length);
                do {
                    PackagePtr pack = ProtoHelp::decode(d->readBuffer_);
                    if (pack) {
                        taskManager_.handleMessage(pack, shared_from_this());
                    } else {
                        break;
                    }
                } while (d->readBuffer_.readableBytes() > 0);

                doRead();
            } else {
                room_.leave(shared_from_this());
                LOG(INFO) << "Session::onRead error:" << ec.message() << "(" << ec << ")";
            }
        });
    }

    void Session::doWrite(boost::system::error_code ec, std::size_t length)
    {
        if (!ec) {
            d->writeDeque_.pop_front();
            if (!d->writeDeque_.empty()) {
                const BufferPtr &buf = d->writeDeque_.front();
                boost::asio::async_write(d->socket_, boost::asio::buffer(buf->peek(), buf->readableBytes()),
                    boost::bind(&Session::doWrite, shared_from_this(), _1, _2));
            }
        } else {
            LOG(INFO) << "doWrite error:" << ec.message() << "(" << ec << ") length:" << length;
            room_.leave(shared_from_this());
        }
    }

    void Session::writeBuffer(BufferPtr buffer)
    {
        bool writeInProgress = !d->writeDeque_.empty();
        d->writeDeque_.push_back(buffer);
        if (!writeInProgress) {
            const BufferPtr &buf = d->writeDeque_.front();
            boost::asio::async_write(d->socket_, boost::asio::buffer(buf->peek(), buf->readableBytes()),
                boost::bind(&Session::doWrite, shared_from_this(), _1, _2));
        }
    }

    void Session::postPackage(const PackagePtr &pack)
    {
        auto buffer = std::make_shared<Buffer>();
        if (ProtoHelp::encode(pack, buffer)) {
            d->io_.post(boost::bind(&Session::writeBuffer, shared_from_this(), buffer));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void SessionRoom::join(PublisherPtr publisher)
    {
        if (!publisher) {
            return;
        }
        std::lock_guard<std::mutex> lock(sessionMutex_);
        sessionCache_[publisher->id()] = publisher;
    }

    void SessionRoom::leave(PublisherPtr publisher)
    {
        if (!publisher) {
            return;
        }

        int sessionId = publisher->id();
        {
            std::lock_guard<std::mutex> lock(sessionMutex_);
            auto it = sessionCache_.find(sessionId);
            if (it != sessionCache_.end()) {
                sessionCache_.erase(it);
            }
        }

        auto removeId = [](std::unordered_map<std::string, std::set<int>> &mp, int id) {
            for (auto it = mp.begin(); it != mp.end(); ++it) {
                auto it2 = it->second.find(id);
                if (it2 != it->second.end()) {
                    it->second.erase(it2);
                }
            }
        };

        std::lock_guard<std::mutex> lock(protoMutex_);
        removeId(subscribeCache_, sessionId);
        removeId(deliverCache_, sessionId);
    }

    void SessionRoom::addSubscribe(int sessionId, int msgtype, const std::string &protoName)
    {
        std::lock_guard<std::mutex> lock(protoMutex_);
        if (msgtype == PacHeader::PUBSUB) {
            subscribeCache_[protoName].insert(sessionId);
        } else if (msgtype == PacHeader::DELIVER) {
            deliverCache_[protoName].insert(sessionId);
        }
    }

    void SessionRoom::delSubscribe(int sessionId, int msgtype, const std::string &protoName)
    {
        auto removeIdName = [](std::unordered_map<std::string, std::set<int>> &mp, int id, const std::string &name) {
            auto it = mp.find(name);
            if (it == mp.end()) {
                return;
            }
            auto it2 = it->second.find(id);
            if (it2 == it->second.end()) {
                return;
            }
            it->second.erase(it2);
        };

        std::lock_guard<std::mutex> lock(protoMutex_);
        if (msgtype == PacHeader::PUBSUB) {
            removeIdName(subscribeCache_, sessionId, protoName);
        } else if (msgtype == PacHeader::DELIVER) {
            removeIdName(deliverCache_, sessionId, protoName);
        }
    }

    // TODO优化，只组一个包
    void SessionRoom::publishMessage(const MessagePtr &msg)
    {
        std::set<int> ids;
        {
            std::lock_guard<std::mutex> lock(protoMutex_);
            auto it = subscribeCache_.find(msg->GetTypeName());
            if (it != subscribeCache_.end()) {
                ids = it->second;
            }
        }

        // 推送给所有session
        std::lock_guard<std::mutex> lock(sessionMutex_);
        for (auto it = ids.begin(); it != ids.end(); ++it) {
            auto it2 = sessionCache_.find(*it);
            if (it2 != sessionCache_.end()) {
                const PublisherPtr &session = it2->second;
                session->publishMessage(msg);
            }
        }
    }

    void SessionRoom::deliverPackage(const PackagePtr &pac)
    {
        std::set<int> ids;
        {
            std::lock_guard<std::mutex> lock(protoMutex_);
            auto it = deliverCache_.find(pac->typeName);
            if (it != deliverCache_.end()) {
                ids = it->second;
            }
        }

        std::lock_guard<std::mutex> lock(sessionMutex_);
        for (auto it = ids.begin(); it != ids.end(); ++it) {
            auto it2 = sessionCache_.find(*it);
            if (it2 != sessionCache_.end()) {
                const PublisherPtr &session = it2->second;
                session->deliverPackage(pac);
            }
        }
    }

    bool SessionRoom::deliverPackage(int id, const PackagePtr &pac)
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        auto it = sessionCache_.find(id);
        if (it != sessionCache_.end()) {
            const PublisherPtr &session = it->second;
            session->deliverPackage(pac);
            return true;
        }
        return false;
    }
}