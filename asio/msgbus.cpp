#include "msgbus.h"
#include "proto/pb_base.pb.h"
#include "internal/AsioServer.h"
#include "internal/AsioClient.h"

namespace ningto
{
    MsgBus::MsgBus(unsigned short port)
        : d_ptr(new AsioServer(port, 2, 2))
    {
#ifdef MSGBUS
        LOG(INFO) << "msgbus init success";
        d_ptr->setPreHandleTask(std::bind(&MsgBus::preHandleMessage, this, std::placeholders::_1, std::placeholders::_2));
#endif
    }

    MsgBus::~MsgBus()
    {
    }

    void MsgBus::run()
    {
        d_ptr->run();
    }

    void MsgBus::stop()
    {
        d_ptr->stop();
    }

    // TODO 包不解析具体的msg
    void MsgBus::preHandleMessage(const SessionPtr &session, const PackagePtr &pac)
    {
        if (pac->msgPtr) {
            return;
        }

        const std::string &name = pac->typeName;
        const int &messageId = pac->header.msgId;
        int sessionId = session->id();
        LOG(INFO) << "preHandleMessage typeName:" << name << ", msgId:" << messageId << ", sessionId:" << sessionId;
        if (pac->header.msgType == PacHeader::REQREP) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                msgSessionCache_[messageId] = sessionId;
            }
            PackagePtr newPac = pac;
            newPac->header.msgType = PacHeader::DELIVER;
            d_func()->deliverPackage(newPac);
        } else if (pac->header.msgType == PacHeader::DELIVER) {
            int id = 0;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = msgSessionCache_.find(messageId);
                if (it != msgSessionCache_.end()) {
                    id = it->second;
                    msgSessionCache_.erase(it);
                }
            }
            if (id > 0) {
                PackagePtr newPac = pac;
                newPac->header.msgType = PacHeader::REQREP;
                d_func()->deliverPackage(id, newPac);
            } else {
                LOG(ERROR) << "msg discard, sessionId:" << sessionId << ", messageId:" << messageId;
            }
        } else if (pac->header.msgType == PacHeader::PUBSUB) {
            d_func()->deliverPackage(pac);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    BusStub::BusStub(int workSize, const std::vector<std::string> &addressList, int heartbeatSeconds)
        : d_ptr(new AsioClient(addressList, 1, heartbeatSeconds))
    {
        publishHandler_ = std::make_shared<PublishHandler>();
        deliverHandler_ = std::make_shared<DeliverHandler>(workSize, std::bind(&AsioClient::postPackage, d_func(), std::placeholders::_1));
        d_func()->addHandler(publishHandler_);
        d_func()->addHandler(deliverHandler_);
    }

    void BusStub::addHandleDeliver(const std::string &typeName, const DeliverFunc &func)
    {
        deliverHandler_->addHandleDeliver(typeName, func);
    }

    void BusStub::addHandlePublish(const std::string &typeName, const PublishFunc &func)
    {
        publishHandler_->addHandlePublish(typeName, func);
    }

    void BusStub::publishMessage(const MessagePtr &msg)
    {
        d_func()->publishMessage(msg);
    }

    void BusStub::start() { d_func()->start(); }
    void BusStub::stop() { d_func()->stop(); }
}