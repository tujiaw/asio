#include "ProtoHelp.h"
#include <boost/asio.hpp>
#include <boost/crc.hpp>

const int kHeaderLen = sizeof(int32_t);

using namespace boost::asio::detail;

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

std::string ProtoHelp::encode(const google::protobuf::Message &message)
{
	std::string result;

	// 消息包的总大小(4bytes)，先占位
	result.resize(kHeaderLen);

	std::string typeName = std::move(message.GetTypeName());
	int nameLen = typeName.size() + 1;
	int be32 = socket_ops::host_to_network_long(nameLen);
	// 类型名的长度(固定4bytes)
	result.append((char*)&be32, sizeof be32);
	// 类型名
	result.append(typeName.c_str(), nameLen);
	// protobuf数据
	if (message.AppendToString(&result)) {
		int checksum = crc32(result.c_str() + kHeaderLen, result.size() - kHeaderLen);
		be32 = socket_ops::host_to_network_long(checksum);
		// crcd大小(固定4bytes)
		result.append((char*)&be32, sizeof be32);
		// 最后在头四个字节填充消息体大小
		int bodyLen = socket_ops::host_to_network_long(result.size() - kHeaderLen);
		std::copy((char*)&bodyLen, ((char*)&bodyLen) + sizeof(bodyLen), result.begin());
	} else {
		result.clear();
	}

	return result;
}

// 从typename长度开始，len已经被读取了
google::protobuf::Message* ProtoHelp::decode(const std::string &buf)
{
	google::protobuf::Message *result = nullptr;
	int len = buf.size();
	if (len > 10) {
		int checksum = net2int(buf.c_str() + len - kHeaderLen);
		int verifyChecksum = crc32(buf.c_str(), len - kHeaderLen);
		if (checksum == verifyChecksum) {
			int nameLen = net2int(buf.c_str());
			if (nameLen >= 2 && nameLen <= len - 2 * kHeaderLen) {
				std::string typeName(buf.begin() + kHeaderLen, buf.begin() + kHeaderLen + nameLen - 1);
				google::protobuf::Message *msg = createMessage(typeName);
				if (msg) {
					const char *data = buf.c_str() + kHeaderLen + nameLen;
					int dataLen = len - nameLen - 2 * kHeaderLen;
					if (msg->ParseFromArray(data, dataLen)) {
						result = msg;
					} else {
						delete msg;
					}
				} else {
					std::cout << "createMessage failed" << std::endl;
				}
			} else {
				std::cout << "name len error:" << nameLen << std::endl;
			}
		} else {
			std::cout << "verify checksum failed, src:" << checksum << ",dst:" << verifyChecksum << std::endl;
		}
	}
	return result;
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