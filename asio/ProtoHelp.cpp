#include "ProtoHelp.h"
#include <boost/asio.hpp>
#include <boost/crc.hpp>
#include "Buffer.h"

using namespace boost::asio::detail;

static const int kFlagLen = 2;
static const int kTotalLen = 4;
static const int kIDLen = 4;
static const int kTypeNameLen = 4;
static const int kMaxPackageLen = 10 * 1024 * 1024;
int ProtoHelp::crc32(const char* start, int len)
{
	boost::crc_32_type result;
	result.process_bytes(start, len);
	return result.checksum();
}

int ProtoHelp::net2int(const char *buf)
{
	int result = 0;
	memcpy(&result, buf, sizeof result);
	return socket_ops::network_to_host_long(result);
}

std::string ProtoHelp::encode(const PackagePtr &package)
{
	std::string result("PP");
	// 标识符 + 消息包的总大小(4bytes)，先占位
	result.resize(kFlagLen + kTotalLen);

	// 增加消息序列号
	int be32 = socket_ops::host_to_network_long(package->id);
	result.append((char*)&be32, sizeof be32);
	std::string typeName = std::move(package->typeName);
	be32 = socket_ops::host_to_network_long(package->typeNameLen);
	// 类型名的长度(固定4bytes)
	result.append((char*)&be32, sizeof be32);
	// 类型名
	result.append(typeName.c_str(), package->typeNameLen);
	// protobuf数据
	if (package->msgPtr->AppendToString(&result)) {
		// 最后在头四个字节填充消息体大小
		int bodyLen = socket_ops::host_to_network_long(result.size());
		std::copy((char*)&bodyLen, ((char*)&bodyLen) + sizeof(bodyLen), result.begin() + kFlagLen);
	} else {
		result.clear();
	}

	return result;
}

PackagePtr ProtoHelp::decode(Buffer &buf)
{
	if (buf.readableBytes() < 11) {
		return nullptr;
	}

	char flags[kFlagLen];
	memcpy(flags, buf.peek(), kFlagLen);
	if (flags[0] != 'P' || flags[1] != 'P') {
		std::cout << "buffer flag error:" << flags[0] << "," << flags[1] << std::endl;
		buf.retrieve(1);
		return nullptr;
	}

	PackagePtr result(new Package());
	result->totalSize = net2int(buf.peek() + kFlagLen);
	if (buf.readableBytes() + kFlagLen < result->totalSize) {
		std::cout << "buffer read able bytes error:" << buf.readableBytes() << ",total:" << result->totalSize << std::endl;
		return nullptr;
	}

	if (result->totalSize <= 0 || result->totalSize > kMaxPackageLen) {
		buf.retrieveInt8();
		std::cout << "total size error:" << result->totalSize << std::endl;
		return nullptr;
	}

	buf.retrieve(kFlagLen + kTotalLen);
	result->id = net2int(buf.peek());
	buf.retrieve(sizeof(result->id));
	result->typeNameLen = net2int(buf.peek());
	if (result->typeNameLen <= 0 || result->typeNameLen > 1024) {
		std::cout << "type name len error:" << result->typeNameLen << std::endl;
		return nullptr;
	}

	buf.retrieve(kTypeNameLen);
	result->typeName = buf.retrieveAsString(result->typeNameLen);
	google::protobuf::Message *msg = createMessage(result->typeName);
	int msgLen = result->totalSize - kFlagLen - kTotalLen - kIDLen - kTypeNameLen - result->typeNameLen;
	if (msg && msg->ParseFromArray(buf.peek(), msgLen)) {
		buf.retrieve(msgLen);
		result->msgPtr = MessagePtr(msg);
		return result;
	}
	std::cout << "protobuf parse error:" << result->typeName << std::endl;
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