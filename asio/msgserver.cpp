#include "msgserver.h"
#include "internal/AsioServer.h"

namespace ningto
{
    MsgServer::MsgServer(unsigned short port) : d_ptr(new AsioServer(port)) { }

    MsgServer::MsgServer(unsigned short port, unsigned int workSize, unsigned int poolSize)
        : d_ptr(new AsioServer(port, workSize, poolSize)) { }

    MsgServer::~MsgServer() { }

    void MsgServer::run()
    {
        std::for_each(handlers_.begin(), handlers_.end(), [this](const MsgHandlerPtr &handler) {
            handler->advise(this);
        });
        d_func()->run();
    }

    void MsgServer::stop()
    {
        d_func()->stop();
    }

    void MsgServer::addMethodHanlder(const MsgHandlerPtr &handler)
    {
        handlers_.push_back(handler);
    }

    void MsgServer::addHandleMessage(const std::string &protoName, const Task &task) {
        d_func()->addHandleMessage(protoName, task);
    }
    void MsgServer::publishMessage(const MessagePtr &msg) {
        d_func()->publishMessage(msg);
    }
}