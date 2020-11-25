#pragma once

#include <memory>
#include <functional>
#include <thread>
#include <vector>
#include <google/protobuf/message.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

namespace ningto
{
    class Session;
    class Buffer;
    struct Package;

    typedef std::shared_ptr<Session> SessionPtr;
    typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
    typedef std::shared_ptr<Package> PackagePtr;
    typedef std::shared_ptr<Buffer> BufferPtr;

    typedef std::function<void(const SessionPtr&, const PackagePtr&)> Task;
    typedef std::function<MessagePtr(const PackagePtr&)> DeliverFunc;
    typedef std::function<void(int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr)> Response;
    typedef std::function<void(int error, const MessagePtr &pubMsgPtr)> PublishFunc;

#pragma pack(push,1)
    struct PacHeader {
        static const char flag0;
        static const char flag1;

        enum MsgType {
            REQREP = 1,
            PUBSUB,
            DELIVER,
        };

        char flag[2]{flag0, flag1};
        short msgType{ 0 };
        short typeNameLen{ 0 };
        union {
            short extInfo{ 0 };
            struct {
                unsigned char iszip : 1;
                unsigned char isorder : 1;
            };
        };
        int msgId{ 0 };
        int msgSize{ 0 };
        int pacSize{ 0 };
    };
#pragma pack(pop)

    const int kPackageHeaderSize = sizeof(PacHeader);
    const int kMsTimeout = 5000;

    struct Package {
        PacHeader header;
        std::string typeName;
#ifdef MSGBUS
        std::vector<char> body;
#endif
        MessagePtr msgPtr;
        inline google::protobuf::Message* msg() { return msgPtr.get(); }
    };

    enum ErrorCode {
        eSuccess = 0,
        eUnkown,
        eTimeout,
        eDisconnect,
        eServiceStopped,
        eParams,
    };

#define D_PRIVATE(ClassNamePrivate)                                               \
    friend class ClassNamePrivate;                                                \
    std::unique_ptr<ClassNamePrivate> d_ptr;                                      \
    inline ClassNamePrivate* d_func(void) { return this->d_ptr.get(); }           \
    inline const ClassNamePrivate* d_func(void) const { return this->d_ptr.get(); }

#define TYPE_NAME(className) className::default_instance().GetTypeName()
}