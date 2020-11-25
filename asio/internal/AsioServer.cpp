#include "AsioServer.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "ProtoHelp.h"
#include "proto/pb_base.pb.h"
#include "tool/util.h"

namespace ningto
{
    AsioServer::AsioServer(unsigned short port, unsigned int workSize, unsigned int poolSize)
        : acceptor_(io_, tcp::endpoint(tcp::v4(), port))
        , taskManager_(workSize, poolSize)
    {
        LOG(INFO) << "AsioServer create acceptor port:" << port;
        init();
        startAccept();
    }

    AsioServer::~AsioServer()
    {
    }

    void AsioServer::run()
    {
        io_.run();
    }

    void AsioServer::asyncRun()
    {
        runthread_ = std::thread(boost::bind(&boost::asio::io_service::run, &io_));
    }

    bool AsioServer::isRunning() const
    {
        return !io_.stopped();
    }

    void AsioServer::stop()
    {
        if (!io_.stopped()) {
            io_.stop();
            if (runthread_.joinable()) {
                runthread_.join();
            }
        }
    }

    void AsioServer::init()
    {
        this->addHandleMessage(ProtoBase::SubscribeReq::descriptor()->full_name(),
            boost::bind(&AsioServer::doSubscribe, this, _1, _2));

        this->addHandleMessage(ProtoBase::HeartbeatReq::descriptor()->full_name(),
            boost::bind(&AsioServer::doHeartbeat, this, _1, _2));
    }

    void AsioServer::startAccept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), room_, taskManager_)->start();
            }
            startAccept();
        });
    }

    void AsioServer::doSubscribe(const SessionPtr &sessionPtr, const PackagePtr &reqPtr)
    {
        auto msg = static_cast<ProtoBase::SubscribeReq*>(reqPtr->msgPtr.get());
        if (msg->typenamelist_size() != msg->msgtypelist_size()) {
            LOG(ERROR) << "doSubscribe error, size error";
            return;
        }

        LOG(INFO) << "===subscribe session id:" << sessionPtr->id() << "===";
        for (int i = 0; i < msg->typenamelist_size(); i++) {
            const std::string &name = msg->typenamelist(i);
            int msgtype = msg->msgtypelist(i);
            if (msg->type() == 1) {
                room_.addSubscribe(sessionPtr->id(), msgtype, name);
                LOG(INFO) << "subscribe add typename:" << name;
            } else {
                LOG(INFO) << "subscribe del typename:" << name;
                room_.delSubscribe(sessionPtr->id(), msgtype, name);
            }
        }
        auto rspPtr = std::make_shared<ProtoBase::SubscribeRsp>();
        rspPtr->set_errorcode(0);
        sessionPtr->replyMessage(reqPtr, rspPtr);
    }

    void AsioServer::doHeartbeat(const SessionPtr &sessionPtr, const PackagePtr &reqPtr) const
    {
        auto rsp = new ProtoBase::HeartbeatRsp();
        rsp->set_servertime(util::currentMillisecond());
        sessionPtr->replyMessage(reqPtr, MessagePtr(rsp));
    }

    void AsioServer::addHandleMessage(const std::string &protoName, const Task &task)
    {
        taskManager_.addHandleTask(protoName, task);
    }

    void AsioServer::setPreHandleTask(const Task &task)
    {
        taskManager_.setPreHandleTask(task);
    }

    void AsioServer::publishMessage(const MessagePtr &msg)
    {
        room_.publishMessage(msg);
    }

    void AsioServer::deliverPackage(const PackagePtr &pac)
    {
        room_.deliverPackage(pac);
    }

    void AsioServer::deliverPackage(int id, const PackagePtr &pac)
    {
        room_.deliverPackage(id, pac);
    }
}