#pragma once

#include <mutex>
#include <memory>
#include <set>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include "Package.h"

using boost::asio::ip::tcp;

namespace ningto
{
    class SessionRoom;
    class TaskManager;
    struct SessionData;

    class Publisher {
    public:
        virtual ~Publisher() {}
        virtual void publishMessage(const MessagePtr &msg) = 0;
        virtual void deliverPackage(const PackagePtr &pac) = 0;
        virtual int id() const = 0;
    };

    typedef std::shared_ptr<Publisher> PublisherPtr;

    class Session : public Publisher, public std::enable_shared_from_this<Session>
    {
    public:
        explicit Session(tcp::socket socket, SessionRoom &room, TaskManager &taskManager);
        Session(const Session&) = delete;
        Session& operator=(const Session&) = delete;
        ~Session();

        void start();
        void replyMessage(const PackagePtr &req, const MessagePtr &rspMsg);
        std::string remoteEndpoint() const;

        void publishMessage(const MessagePtr &msg) override;
        void deliverPackage(const PackagePtr &pac) override;
        int id() const override;

        void doRead();
        void doWrite(boost::system::error_code ec, std::size_t length);
        void writeBuffer(BufferPtr buffer);
        void postPackage(const PackagePtr &pack);

    private:
        std::string endpoint_;
        std::unique_ptr<SessionData> d;
        SessionRoom &room_;
        TaskManager &taskManager_;
    };

    typedef std::shared_ptr<Session> SessionPtr;

    class SessionRoom
    {
    public:
        void join(PublisherPtr publisher);
        void leave(PublisherPtr publisher);
        void addSubscribe(int sessionId, int msgtype, const std::string &protoName);
        void delSubscribe(int sessionId, int msgtype, const std::string &protoName);
        void publishMessage(const MessagePtr &msg);
        void deliverPackage(const PackagePtr &pac);
        bool deliverPackage(int id, const PackagePtr &pac);

    private:
        std::mutex sessionMutex_;
        std::mutex protoMutex_;
        std::unordered_map<std::string, std::set<int>> subscribeCache_;
        std::unordered_map<std::string, std::set<int>> deliverCache_;
        std::unordered_map<int, PublisherPtr> sessionCache_;
    };
}