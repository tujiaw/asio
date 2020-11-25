#pragma once

#include "internal/Package.h"
#include "internal/Session.h"

namespace ningto
{
    class AsioServer;
    class MsgServer;
    class MsgHandler {
    public:
        virtual ~MsgHandler() {}
        virtual void advise(MsgServer *server) = 0;
    };

    using MsgHandlerPtr = std::shared_ptr<MsgHandler>;

#define ADVISE_HANDLER(PROTO_REQ, PROTO_RSP, NAME) \
    server->addHandleMessage(TYPE_NAME(PROTO_REQ), [this](const SessionPtr &session, const PackagePtr &request) { \
        auto req = static_cast<PROTO_REQ*>(request->msgPtr.get()); \
        auto rsp = std::make_shared<PROTO_RSP>(); \
        this->NAME(req, rsp.get()); \
        session->replyMessage(request, rsp); \
    });

    class MsgServer {
    public:
        explicit MsgServer(unsigned short port);
        explicit MsgServer(unsigned short port, unsigned int workSize, unsigned int poolSize);
        ~MsgServer();
        MsgServer(const MsgServer&) = delete;
        MsgServer& operator=(const MsgServer&) = delete;

        void run();
        void stop();
        void addMethodHanlder(const MsgHandlerPtr &handler);
        void addHandleMessage(const std::string &protoName, const Task &task);
        void publishMessage(const MessagePtr &msg);

    private:
        std::vector<MsgHandlerPtr> handlers_;
        D_PRIVATE(AsioServer)
    };
}