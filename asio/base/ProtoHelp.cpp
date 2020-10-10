#include "ProtoHelp.h"
#include <boost/asio.hpp>
#include "glog/logging.h"
#include "Buffer.h"

using namespace boost::asio::detail;

static const Package emptyPackage;
static const int kFlagLen = sizeof(emptyPackage.header.flag);
static const int kMaxPackageLen = 1024 * 1024 * 100;

bool gotoFlag(Buffer &buf)
{
    while (buf.readableBytes() > kPackageHeaderSize) {
        const char *p = buf.peek();
        if (p[0] == 'P' && p[1] == 'P') {
            return true;
        } else {
            DLOG(INFO) << "flag error";
            buf.retrieve(1);
        }
    }
    return false;
}

void bufferAppendHeader(Buffer &buffer, const PacHeader &header)
{
    PacHeader newHeader = header;
    newHeader.msgType = ProtoHelp::short2net(header.msgType);
    newHeader.typeNameLen = ProtoHelp::short2net(header.typeNameLen);
    newHeader.extInfo = ProtoHelp::short2net(header.extInfo);
    newHeader.msgId = ProtoHelp::int2net(header.msgId);
    newHeader.msgSize = ProtoHelp::int2net(header.msgSize);
    newHeader.pacSize = ProtoHelp::int2net(header.pacSize);
    buffer.append(&newHeader, kPackageHeaderSize);
}

void bufferPeekHeader(Buffer &buffer, PacHeader &header)
{
    memcpy(&header, buffer.peek(), kPackageHeaderSize);
    header.msgType = ProtoHelp::net2short(header.msgType);
    header.typeNameLen = ProtoHelp::net2short(header.typeNameLen);
    header.extInfo = ProtoHelp::net2short(header.extInfo);
    header.msgId = ProtoHelp::net2int(header.msgId);
    header.msgSize = ProtoHelp::net2int(header.msgSize);
    header.pacSize = ProtoHelp::net2int(header.pacSize);
}

int ProtoHelp::net2int(const char *buf)
{
    int result = 0;
    memcpy(&result, buf, sizeof result);
    return socket_ops::network_to_host_long(result);
}

int ProtoHelp::net2int(int i)
{
    return socket_ops::network_to_host_long(i);
}

int ProtoHelp::int2net(int i)
{
    return socket_ops::host_to_network_long(i);
}

short ProtoHelp::net2short(short i)
{
    return socket_ops::network_to_host_short(i);
}

short ProtoHelp::short2net(short i)
{
    return socket_ops::host_to_network_short(i);
}

bool ProtoHelp::encode(const PackagePtr &package, BufferPtr &buffer)
{
    std::string content;
    content.resize(package->msgPtr->ByteSize());
    if (package->msgPtr->SerializePartialToArray(&content[0], content.size())) {
        package->typeName = package->msgPtr->GetTypeName();
        package->header.typeNameLen = package->typeName.size();
        package->header.msgSize = package->msgPtr->ByteSize();
        package->header.pacSize = kPackageHeaderSize + package->typeName.size() + content.size();

        bufferAppendHeader(*buffer.get(), package->header);
        buffer->append(&package->typeName[0], package->header.typeNameLen);
        buffer->append(content);
        return true;
    }
    return false;
}

PackagePtr ProtoHelp::decode(Buffer &buf)
{
    if (!gotoFlag(buf)) {
        DLOG(ERROR) << "go to flag error";
        return PackagePtr();
    }

    PacHeader header;
    bufferPeekHeader(buf, header);

    // 数据不够等待下次读取更多的数据
    if (buf.readableBytes() < (std::size_t)header.pacSize) {
        return PackagePtr();
    }

    buf.retrieve(kPackageHeaderSize);
    if (header.pacSize < 0 || header.pacSize > kMaxPackageLen) {
        DLOG(INFO) << "pac size error:" << header.pacSize;
        return decode(buf);
    }

    if (header.typeNameLen <= 0 || header.typeNameLen > 1024) {
        DLOG(INFO) << "type name len error:" << header.typeNameLen;
        return decode(buf);
    }

    std::string typeName = buf.retrieveAsString(header.typeNameLen);
    google::protobuf::Message *msg = createMessage(typeName);
    if (!msg) {
        DLOG(ERROR) << "create message failed:" << typeName;
        return decode(buf);
    }

    bool parseResult = false;
    int remainSize = header.pacSize - kPackageHeaderSize - header.typeNameLen;
    parseResult = msg->ParseFromArray(buf.peek(), header.msgSize);

    if (parseResult) {
        buf.retrieve(remainSize);
        PackagePtr result(new Package());
        result->header = header;
        result->typeName = typeName;
        result->msgPtr = MessagePtr(msg);
        return result;
    }

    DLOG(ERROR) << "decode failed:" << typeName;
    return PackagePtr();;
}

google::protobuf::Message* ProtoHelp::createMessage(const std::string & typeName)
{
    google::protobuf::Message *message = NULL;
    const google::protobuf::Descriptor* desc = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
    if (desc) {
        const google::protobuf::Message *prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(desc);
        if (prototype) {
            message = prototype->New();
        }
    }
    return message;
}