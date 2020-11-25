#include "msgclient.h"
#include "internal/AsioClient.h"
#include "tool/util.h"

namespace ningto
{
    MsgClient::MsgClient(const std::vector<std::string> &addressList, int heartbeatSeconds)
        : d_ptr(util::make_unique<AsioClient>(addressList, 1, heartbeatSeconds))
    {
        publishHandler_ = std::make_shared<PublishHandler>();
        d_func()->addHandler(publishHandler_);
    }

    MsgClient::~MsgClient() { }

    void MsgClient::addHandlePublish(const std::string &typeName, const PublishFunc &func)
    {
        publishHandler_->addHandlePublish(typeName, func);
    }

    void MsgClient::start() {
        d_func()->start();
    }

    void MsgClient::stop() {
        d_func()->stop();
    }

    bool MsgClient::waitConnected(int timeoutSecond)
    {
        return d_func()->waitConnected(timeoutSecond);
    }

    int MsgClient::sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout) {
        if (d_func()->stopped()) {
            LOG(WARNING) << "service stopped";
            return eDisconnect;
        }
        return d_func()->sendMessage(msgPtr, rspPtr, msTimeout);
    }

    int MsgClient::postMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout) {
        if (d_func()->stopped()) {
            LOG(WARNING) << "service stopped";
            return eDisconnect;
        }
        return d_func()->postMessage(msgPtr, res, msTimeout);
    }

    int MsgClient::postOrderMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout)
    {
        if (d_func()->stopped()) {
            LOG(WARNING) << "service stopped";
            return eDisconnect;
        }
        return d_func()->postOrderMessage(msgPtr, res, msTimeout);
    }
}