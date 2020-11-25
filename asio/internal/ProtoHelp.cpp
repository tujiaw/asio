#include "ProtoHelp.h"
#include <boost/asio.hpp>
#include <zlib.h>
#include "glog/logging.h"
#include "Buffer.h"
#include "tool/strutil.h"

using namespace boost::asio::detail;

namespace ningto
{
    const char PacHeader::flag0 = '`';
    const char PacHeader::flag1 = '~';
    static const Package emptyPackage;
    static const int kFlagLen = sizeof(emptyPackage.header.flag);
    static const int kMaxPackageLen = 1024 * 1024 * 100;

    bool gotoFlag(Buffer &buf)
    {
        while (buf.readableBytes() > kPackageHeaderSize) {
            const char *p = buf.peek();
            if (p[0] == PacHeader::flag0 && p[1] == PacHeader::flag1) {
                return true;
            } else {
                // LOG(INFO) << "flag error, flag0:" << p[0] << ", flag1:" << p[1];
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

    int ProtoHelp::s_enableCompressSize = -1;
    void ProtoHelp::initEnableCompressSize(int size)
    {
        s_enableCompressSize = size;
    }

    int ProtoHelp::enableCompressSize()
    {
        return s_enableCompressSize;
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
        if (package->typeName.empty()) {
            return false;
        }

#ifdef MSGBUS
        if (!strutil::HasPrefixString(package->typeName, "ProtoBase")) {
            bufferAppendHeader(*buffer.get(), package->header);
            buffer->append(&package->typeName[0], package->header.typeNameLen);
            buffer->append(&package->body[0], package->body.size());
            return true;
        }
#endif
        std::string content;
        content.resize(package->msgPtr->ByteSizeLong());
        if (package->msgPtr->SerializePartialToArray(&content[0], content.size())) {
            // need compress
            if (ProtoHelp::enableCompressSize() >= 0 && (int)content.size() > ProtoHelp::enableCompressSize()) {
                unsigned long bufferSize = compressBound(content.size());
                std::string compressedBuffer(bufferSize, 0);
                int errcode = compress((unsigned char*)&compressedBuffer[0], &bufferSize, (unsigned char*)&content[0], content.size());
                if (errcode == Z_OK) {
                    LOG(INFO) << "compress success from:" << content.size() << ", to:" << bufferSize;
                    package->header.iszip = 1;
                    content.assign(&compressedBuffer[0], bufferSize);
                } else {
                    LOG(ERROR) << "compress error:" << errcode << ", size:" << content.size();
                }
            }

            package->typeName = package->msgPtr->GetTypeName();
            package->header.typeNameLen = (short)package->typeName.size();
            package->header.msgSize = package->msgPtr->ByteSizeLong();
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
            LOG(ERROR) << "go to flag error";
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
            LOG(INFO) << "pac size error:" << header.pacSize;
            return decode(buf);
        }

        if (header.typeNameLen <= 0 || header.typeNameLen > 1024) {
            LOG(INFO) << "type name len error:" << header.typeNameLen;
            return decode(buf);
        }

        std::string typeName = buf.retrieveAsString(header.typeNameLen);

#ifdef MSGBUS
        if (!strutil::HasPrefixString(typeName, "ProtoBase")) {
            int remainSize = header.pacSize - kPackageHeaderSize - header.typeNameLen;
            auto result = std::make_shared<Package>();
            result->header = header;
            result->typeName = typeName;
            result->body.resize(remainSize);
            buf.read(&result->body[0], remainSize);
            return result;
        }
#endif
        google::protobuf::Message *msg = createMessage(typeName);
        if (!msg) {
            LOG(ERROR) << "create message failed:" << typeName;
            return decode(buf);
        }

        bool parseResult = false;
        int remainSize = header.pacSize - kPackageHeaderSize - header.typeNameLen;
        if (header.iszip) {
            unsigned long bufferSize = header.msgSize;
            std::string unCompressedBuffer(bufferSize, 0);
            int errcode = uncompress((unsigned char*)&unCompressedBuffer[0], &bufferSize, (const unsigned char*)buf.peek(), remainSize);
            if (errcode == Z_OK) {
                LOG(INFO) << "uncompress success, from:" << remainSize << ", to:" << unCompressedBuffer.size();
                header.iszip = false;
                header.pacSize = kPackageHeaderSize + header.typeNameLen + header.msgSize;
                parseResult = msg->ParseFromArray(&unCompressedBuffer[0], bufferSize);
            } else {
                LOG(ERROR) << "uncompress error:" << errcode << ", size:" << bufferSize;
            }
        } else {
            parseResult = msg->ParseFromArray(buf.peek(), header.msgSize);
        }

        if (parseResult) {
            buf.retrieve(remainSize);
            auto result = std::make_shared<Package>();
            result->header = header;
            result->typeName = typeName;
            result->msgPtr = MessagePtr(msg);
            return result;
        }

        LOG(ERROR) << "decode failed:" << typeName;
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
}