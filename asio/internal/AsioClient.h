#pragma once

#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <deque>
#include <mutex>
#include "Package.h"
#include "Buffer.h"
#include "ThreadPool.h"

using boost::asio::ip::tcp;

namespace ningto
{
    class RequestMessage {
    public:
        class Data {
        public:
            Data(boost::asio::io_service &io, const PackagePtr &p, const Response &r, int msTimeout, const std::function<void(int)> &cb);
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;
            PackagePtr pac;
            Response res;
            time_t reqTime{ 0 };
            time_t writeFinishedTime{ 0 };
            time_t rspTime{ 0 };
            std::function<void(int)> onTimeout;
            boost::asio::deadline_timer timer;
        };
        typedef std::shared_ptr<Data> DataPtr;

        explicit RequestMessage(boost::asio::io_service &io);
        void put(const PackagePtr &p, const Response &r, int msTimeout);
        void startTimer(int msgid);
        DataPtr pick(int msgid);

    protected:
        void onTimeout(int msgId);

    private:
        boost::asio::io_service &io_;
        std::mutex mutex_;
        std::unordered_map<int, DataPtr> cache_;
    };

    class Handler {
    public:
        virtual ~Handler() {}
        virtual int type() const = 0;
        virtual std::vector<std::string> typeNames() const = 0;
        virtual void onPackage(const PackagePtr &package) = 0;
    };

    using HandlerPtr = std::shared_ptr<Handler>;
    using PostPackage = std::function<void(const PackagePtr&)>;

    class DeliverHandler : public Handler {
    public:
        DeliverHandler(int threadSize, const PostPackage &post) : pool_(threadSize), post_(post)
        {
            pool_.start();
        }

        void addHandleDeliver(const std::string &typeName, const DeliverFunc &func)
        {
            map_[typeName] = func;
        }

        int type() const override { return PacHeader::DELIVER; }
        std::vector<std::string> typeNames() const override
        {
            std::vector<std::string> result;
            for (auto it = map_.begin(); it != map_.end(); ++it) {
                result.push_back(it->first);
            }
            return result;
        }

        void onPackage(const PackagePtr &package) override
        {
            if (package->header.msgType == PacHeader::DELIVER) {
                auto it = map_.find(package->typeName);
                if (it != map_.end()) {
                    DeliverFunc func = it->second;
                    pool_.run([this, func, package]() {
                        MessagePtr rsp = func(package);
                        replyDeliver(package, rsp);
                    });
                }
            }
        }

        void replyDeliver(const PackagePtr &req, const MessagePtr &rspMsg) const
        {
            auto pac = std::make_shared<Package>();
            pac->header = req->header;
            pac->typeName = rspMsg->GetTypeName();
            pac->msgPtr = rspMsg;
            post_(pac);
        }

    private:
        PostPackage post_;
        std::map<std::string, DeliverFunc> map_;
        ThreadPool pool_;
    };

    class PublishHandler : public Handler {
    public:
        void addHandlePublish(const std::string &typeName, const PublishFunc &func)
        {
            map_[typeName] = func;
        }

        int type() const override { return PacHeader::PUBSUB; }
        std::vector<std::string> typeNames() const override
        {
            std::vector<std::string> result;
            for (auto it = map_.begin(); it != map_.end(); ++it) {
                result.push_back(it->first);
            }
            return result;
        }

        void onPackage(const PackagePtr &package) override
        {
            if (package->header.msgType == PacHeader::PUBSUB) {
                auto it = map_.find(package->typeName);
                if (it != map_.end()) {
                    PublishFunc func = it->second;
                    func(0, package->msgPtr);
                }
            }
        }

    private:
        std::map<std::string, PublishFunc> map_;
    };

    class AsioClient {
    public:
        explicit AsioClient(const std::vector<std::string> &addressList, int reconnectSeconds = 1, int heartbeatSeconds = 10);
        AsioClient(const AsioClient&) = delete;
        AsioClient& operator=(const AsioClient&) = delete;
        virtual ~AsioClient();
        void addHandler(const HandlerPtr &handler);
        void setConnectSuccess(const std::function<void()> &cb);
        void start();
        void stop();
        bool stopped() const;
        bool waitConnected(int timeoutSeconds = 3) const;

        int sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout = kMsTimeout);
        int postMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout = kMsTimeout);
        int postOrderMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout = kMsTimeout);
        void publishMessage(const MessagePtr &msgPtr);
        int postPackage(const PackagePtr &package);

    protected:
        void connect();
        void doConnect(const boost::system::error_code &ec);
        int postMessage(bool isOrder, const MessagePtr &msgPtr, const Response &res, int msTimeout = kMsTimeout);
        void close();
        void startRead();
        void doRead(boost::system::error_code ec, std::size_t length);
        void startWrite(const PackagePtr &pacPtr);
        void startWrite();
        void doClose();
        void startSubscribe();
        void startHeartbeatTimer();
        void doHeartbeat(const boost::system::error_code &e);
        void doResponse(const PackagePtr &pack);

    private:
        std::size_t addressIndex_{ 0 };
        std::vector<std::string> addressList_;
        boost::asio::io_service io_;
        boost::asio::io_service ioDoResponse_;
        boost::asio::io_service::work workDoResponse_;
        tcp::socket socket_;
        std::deque<PackagePtr> pendingList_;
        std::atomic<int> id_{ 0 };
        std::thread runthread_;
        std::thread threadDoResponse_;
        std::unique_ptr<boost::asio::deadline_timer> heartbeatTimer_;
        int reconnectSeconds_;
        int heartbeatSeconds_;
        std::atomic<bool> isOnline_{ false };
        std::function<void()> connectSuccess_;

        static const int kTempBufSize = boost::asio::detail::default_max_transfer_size;
        char tempBuf_[kTempBufSize];
        Buffer readBuffer_;

        std::mutex mutex_;
        std::condition_variable cond_;

        std::vector<HandlerPtr> handlerList_;
        RequestMessage requestMessage_;
    };
}