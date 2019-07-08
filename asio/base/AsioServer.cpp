#include "AsioServer.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "ProtoHelp.h"
#include "proto/pb_base.pb.h"
#include "util.h"

AsioServer::AsioServer(unsigned short port)
	: acceptor_(io_, tcp::endpoint(tcp::v4(), port))
{
	init();
    startAccept();
}

AsioServer::AsioServer(unsigned short port, unsigned int workSize, unsigned int poolSize)
    : acceptor_(io_, tcp::endpoint(tcp::v4(), port)), 
    taskManager_(workSize, poolSize)
{
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
    runthread_ = boost::thread(boost::bind(&boost::asio::io_service::run, &io_));
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
	this->addHandleMessage(PbBase::SubscribeReq::descriptor()->full_name(),
        boost::bind(&AsioServer::doSubscribe, this, _1, _2));

	this->addHandleMessage(PbBase::HeartbeatReq::descriptor()->full_name(),
        boost::bind(&AsioServer::doHeartbeat, this, _1, _2));
}

void AsioServer::startAccept()
{
    SessionPtr newSession(new Session(io_, room_, taskManager_));
	acceptor_.async_accept(newSession->socket(), boost::bind(&AsioServer::doAccept, this, newSession, boost::asio::placeholders::error));
}

void AsioServer::doAccept(SessionPtr session, boost::system::error_code ec)
{
    if (!ec) {
        session->start();
    } else {
        DLOG(ERROR) << "async accept error:" << ec.message();
    }
    startAccept();
}

void AsioServer::doSubscribe(const SessionPtr &sessionPtr, const PackagePtr &reqPtr)
{
    PbBase::SubscribeReq *msg = static_cast<PbBase::SubscribeReq*>(reqPtr->msgPtr.get());
    for (int i = 0; i < msg->typenamelist_size(); i++) {
        if (msg->type() == 1) {
            sessionPtr->addSubscribe(msg->typenamelist(i));
        } else {
            sessionPtr->removeSubscribe(msg->typenamelist(i));
        }
    }
    std::shared_ptr<PbBase::SubscribeRsp> rspPtr(new PbBase::SubscribeRsp());
    rspPtr->set_errorcode(0);
    sessionPtr->replyMessage(reqPtr, rspPtr);
}

void AsioServer::doHeartbeat(const SessionPtr &sessionPtr, const PackagePtr &reqPtr)
{
    PbBase::HeartbeatReq *msg = static_cast<PbBase::HeartbeatReq*>(reqPtr->msgPtr.get());
    DLOG(INFO) << "host:" << sessionPtr->remoteEndpoint() << ",cpu:" << msg->cpu() << ",memory:" << msg->memory();
    PbBase::HeartbeatRsp *rsp = new PbBase::HeartbeatRsp();
    rsp->set_servertime(util::currentMillisecond());
    sessionPtr->replyMessage(reqPtr, MessagePtr(rsp));
}

void AsioServer::addHandleMessage(const std::string &protoName, const Task &task)
{
    taskManager_.addHandleTask(protoName, task);
}

void AsioServer::publishMessage(const MessagePtr &msg)
{
	room_.publishMessage(msg);
}
