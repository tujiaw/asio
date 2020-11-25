#pragma once

#include "internal/Package.h"
#include "internal/AsioClient.h"

namespace ningto
{
    class PublishHandler;
    class MsgClient {
    public:
        explicit MsgClient(const std::vector<std::string> &addressList, int heartbeatSeconds = 5);
        ~MsgClient();
        MsgClient(const MsgClient&) = delete;
        MsgClient& operator=(const MsgClient&) = delete;

        void addHandlePublish(const std::string &typeName, const PublishFunc &func);
        void start();
        void stop();
        bool waitConnected(int timeoutSecond = 3);

        int sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout = kMsTimeout);
        int postMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout = kMsTimeout);
        int postOrderMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout = kMsTimeout);

    private:
        std::shared_ptr<PublishHandler> publishHandler_;
        D_PRIVATE(AsioClient);
    };
}