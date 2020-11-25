#pragma once

#include "Buffer.h"
#include "TaskManager.h"
#include "Session.h"

using boost::asio::ip::tcp;

namespace ningto
{
    class AsioServer
    {
    public:
        explicit AsioServer(unsigned short port, unsigned int workSize = std::thread::hardware_concurrency(), unsigned int poolSize = std::thread::hardware_concurrency());
        AsioServer(const AsioServer&) = delete;
        AsioServer& operator=(const AsioServer&) = delete;
        virtual ~AsioServer();

        void run();
        void asyncRun();
        bool isRunning() const;
        void stop();

        void addHandleMessage(const std::string &protoName, const Task &task);
        void setPreHandleTask(const Task &task);
        void publishMessage(const MessagePtr &msg);
        void deliverPackage(const PackagePtr &pac);
        void deliverPackage(int id, const PackagePtr &pac);

    protected:
        void init();
        void startAccept();
        void doSubscribe(const SessionPtr &sessionPtr, const PackagePtr &reqPtr);
        void doHeartbeat(const SessionPtr &sessionPtr, const PackagePtr &reqPtr) const;

    private:
        boost::asio::io_service io_;
        tcp::acceptor acceptor_;
        std::thread runthread_;
        TaskManager taskManager_;
        SessionRoom room_;
    };
}
