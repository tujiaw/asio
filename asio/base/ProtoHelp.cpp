#include "ProtoHelp.h"
#include <boost/asio.hpp>
#include <zlib.h>
#include "Buffer.h"

using namespace boost::asio::detail;

static const Package emptyPackage;
static const int kFlagLen = sizeof(emptyPackage.header.flag);
static const int kMaxPackageLen = 1024 * 1024 * 1024;
static const int kMinZipLen = 1024;

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

bool ProtoHelp::encode(const PackagePtr &package, BufferPtr &buffer)
{
    std::string content;
    content.resize(package->msgPtr->ByteSize());
    if (package->msgPtr->SerializePartialToArray(&content[0], content.size())) {
        if (content.size() > kMinZipLen) {
            unsigned long bufferSize = compressBound(content.size());
            std::string buffer(bufferSize, 0);
            int errcode = compress((unsigned char*)&buffer[0], &bufferSize, (unsigned char*)&content[0], content.size());
            if (errcode == Z_OK) {
                package->header.iszip = 1;
                content.assign(&buffer[0], bufferSize);
            } else {
                LOG(ERROR) << "compress error:" << errcode;
            }
        }

        package->typeName = package->msgPtr->GetTypeName();
        package->header.typeNameLen = package->typeName.size();
        package->header.msgSize = package->msgPtr->ByteSize();
        package->header.pacSize = kPackageHeaderSize + package->typeName.size() + content.size();
        buffer->append(&package->header, kPackageHeaderSize);
        buffer->append(&package->typeName[0], package->header.typeNameLen);
        buffer->append(content);
        return true;
    }
    return false;
}

PackagePtr ProtoHelp::decode(Buffer &buf)
{
	bool isOk = false;
	char flags[kFlagLen];
	while (buf.readableBytes() > kPackageHeaderSize) {
		memcpy(flags, buf.peek(), kFlagLen);
		if (flags[0] == 'P' || flags[1] == 'P') {
			isOk = true;
			break;
		} else {
			std::cout << "buffer flag error:" << flags[0] << "," << flags[1] << std::endl;
			buf.retrieve(1);
		}
	}
	if (!isOk) {
		return nullptr;
	}

    PacHeader header;
    memcpy(&header, buf.peek(), kPackageHeaderSize);
	if (buf.readableBytes() < (std::size_t)header.pacSize) {
		return nullptr;
	}

    if (header.typeNameLen <= 0 || header.typeNameLen > 1024) {
        std::cout << "type name len error:" << header.typeNameLen << std::endl;
        return nullptr;
    }

    buf.retrieve(kPackageHeaderSize);
	std::string typeName = buf.retrieveAsString(header.typeNameLen);
    google::protobuf::Message *msg = createMessage(typeName);;
    if (!msg) {
        LOG(ERROR) << "create message failed:" << typeName;
        return nullptr;
    }

    bool parseResult = false;
    if (header.iszip) {
        unsigned long bufferSize = header.msgSize;
        std::string buffer(bufferSize, 0);
        int errcode = uncompress((unsigned char*)&buffer[0], &bufferSize, (unsigned char*)buf.peek(), header.pacSize - kPackageHeaderSize - header.typeNameLen);
        if (errcode == Z_OK) {
            parseResult = msg->ParseFromArray(&buffer[0], bufferSize);
        } else {
            LOG(ERROR) << "uncompress error:" << errcode;
        }
    } else {
        parseResult = msg->ParseFromArray(buf.peek(), header.msgSize);
    }

    if (parseResult) {
        buf.retrieve(header.msgSize);
        PackagePtr result(new Package());
        result->header = header;
        result->typeName = typeName;
        result->msgPtr = MessagePtr(msg);
        return result;
    }
    return nullptr;
}

google::protobuf::Message* ProtoHelp::createMessage(const std::string & typeName)
{
	google::protobuf::Message *message = nullptr;
	const google::protobuf::Descriptor* desc = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
	if (desc) {
		const google::protobuf::Message *prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(desc);
		if (prototype) {
			message = prototype->New();
		}
	}
	return message;
}