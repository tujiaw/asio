#pragma once

#include "internal/Package.h"
#include <unordered_map>

namespace ningto
{
    class AsioClient;
    class AsioServer;
    class PublishHandler;
    class DeliverHandler;
    class MsgBus
    {
    public:
        explicit MsgBus(unsigned short port);
        ~MsgBus();
        MsgBus(const MsgBus&) = delete;
        MsgBus& operator=(const MsgBus&) = delete;

        void run();
        void stop();

    private:
        void preHandleMessage(const SessionPtr &sessionPtr, const PackagePtr &pacPtr);

    private:
        std::mutex mutex_;
        std::unordered_map<int, int> msgSessionCache_;
        D_PRIVATE(AsioServer)
    };

    class BusStub {
    public:
        explicit BusStub(int workSize, const std::vector<std::string> &addressList, int heartbeatSeconds = 5);
        BusStub(const BusStub&) = delete;
        BusStub& operator=(const BusStub&) = delete;

        void addHandleDeliver(const std::string &typeName, const DeliverFunc &func);
        void addHandlePublish(const std::string &typeName, const PublishFunc &func);
        void publishMessage(const MessagePtr &msg);
        void start();
        void stop();

    private:
        std::shared_ptr<DeliverHandler> deliverHandler_;
        std::shared_ptr<PublishHandler> publishHandler_;
        D_PRIVATE(AsioClient);
    };
}