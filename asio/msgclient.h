#ifndef ASIO_MSGCLIENT_H_
#define ASIO_MSGCLIENT_H_

#include "base/desc.h"

class MsgClient {
public:
    explicit MsgClient(const std::string &addressint, int heartbeatSeconds = 0);
    ~MsgClient();
    MsgClient(const MsgClient&) = delete;
    MsgClient& operator=(const MsgClient&) = delete;

    void addHandlePublish(const std::string &typeName, const PublishFunc &func);
    void start();
    void stop();

    int sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout = 3000);
    int postMessage(const MessagePtr &msgPtr, const Response &res);

private:
    const std::string address_;
    D_PRIVATE(AsioClient);
};

#endif  // ASIO_MSGCLIENT_H_