#include "ProtoHelp.h"
#include <boost/asio.hpp>
#include <boost/crc.hpp>

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

std::string ProtoHelp::encode(const PackagePtr &package)
{
	std::string result;

	// ��Ϣ�����ܴ�С(4bytes)����ռλ
	result.resize(kHeaderLen);
	// ������Ϣ���к�
	int be32 = socket_ops::host_to_network_long(package->id);
	result.append((char*)&be32, sizeof be32);
	std::string typeName = std::move(package->typeName);
	be32 = socket_ops::host_to_network_long(package->typeNameLen);
	// �������ĳ���(�̶�4bytes)
	result.append((char*)&be32, sizeof be32);
	// ������
	result.append(typeName.c_str(), package->typeNameLen);
	// protobuf����
	if (package->msgPtr->AppendToString(&result)) {
		int checksum = crc32(result.c_str() + kHeaderLen, result.size() - kHeaderLen);
		be32 = socket_ops::host_to_network_long(checksum);
		// crcd��С(�̶�4bytes)
		result.append((char*)&be32, sizeof be32);
		// �����ͷ�ĸ��ֽ������Ϣ���С
		int bodyLen = socket_ops::host_to_network_long(result.size() - kHeaderLen);
		std::copy((char*)&bodyLen, ((char*)&bodyLen) + sizeof(bodyLen), result.begin());
	} else {
		result.clear();
	}

	return result;
}

// ��typename���ȿ�ʼ��len�Ѿ�����ȡ��
PackagePtr ProtoHelp::decode(const std::string &buf)
{
	PackagePtr result(new Package());
	int len = buf.size();
	if (len > 10) {
		int checksum = net2int(buf.c_str() + len - kHeaderLen);
		int verifyChecksum = crc32(buf.c_str(), len - kHeaderLen);
		if (checksum == verifyChecksum) {
			result->checksum = checksum;
			result->id = net2int(buf.c_str());
			result->typeNameLen = net2int(buf.c_str() + kHeaderLen);
			if (result->typeNameLen >= 2 && result->typeNameLen <= len - 3 * kHeaderLen) {
				std::string typeName(buf.begin() + 2 * kHeaderLen, buf.begin() + 2 * kHeaderLen + result->typeNameLen - 1);
				google::protobuf::Message *msg = createMessage(typeName);
				if (msg) {
					const char *data = buf.c_str() + 2 * kHeaderLen + result->typeNameLen;
					int dataLen = len - result->typeNameLen - 3 * kHeaderLen;
					if (msg->ParseFromArray(data, dataLen)) {
						result->msgPtr = MessagePtr(msg);
					} else {
						delete msg;
					}
				} else {
					std::cout << "createMessage failed" << std::endl;
				}
			} else {
				std::cout << "name len error:" << result->typeNameLen << std::endl;
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