#include "ProtoHelp.h"
#include <boost/asio.hpp>
#include "Buffer.h"

using namespace boost::asio::detail;

static const Package emptyPackage;
static const int kFlagLen = sizeof(emptyPackage.flag);
static const int kTotalLen = sizeof(emptyPackage.totalSize);
static const int kIDLen = sizeof(emptyPackage.id);
static const int kTypeNameLen = sizeof(emptyPackage.typeNameLen);
static const int kMaxPackageLen = 10 * 1024 * 1024;

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

BufferPtr ProtoHelp::encode(const PackagePtr &package)
{	
	std::string content;
	content.resize(package->msgPtr->ByteSize());
	if (package->msgPtr->SerializePartialToArray(&content[0], content.size())) {
		BufferPtr buffer(new Buffer());
		buffer->append("PP");
		// ��ʶ�� + ��Ϣ�����ܴ�С(4bytes)����ռλ
		buffer->appendInt32(0);

		// ������Ϣ���к�
		int be32 = socket_ops::host_to_network_long(package->id);
		buffer->appendInt32(be32);

		std::string typeName = std::move(package->typeName);
		be32 = socket_ops::host_to_network_long(package->typeNameLen);
		// �������ĳ���(�̶�4bytes)
		buffer->appendInt32(be32);
		// ������
		buffer->append(&typeName[0], package->typeNameLen);
		// ��Ϣ����
		buffer->append(content);
		// �����ͷ�ĸ��ֽ������Ϣ���С
		int bodyLen = socket_ops::host_to_network_long(buffer->readableBytes());
		std::memcpy((char*)(buffer->peek() + kFlagLen), (char*)&bodyLen, kTotalLen);
		return buffer;
	}
	return nullptr;
}

PackagePtr ProtoHelp::decode(Buffer &buf)
{
	bool isOk = false;
	char flags[kFlagLen];
	while (buf.readableBytes() >= 11) {
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

	buf.retrieveInt16();
	int totalSize = net2int(buf.readInt32());
	if (totalSize <= 0 || totalSize > kMaxPackageLen) {
		std::cout << "total size error:" << totalSize << std::endl;
		return nullptr;
	}

	if ((int)buf.readableBytes() + kFlagLen + kTotalLen < totalSize) {
		std::cout << "buffer read able bytes error:" << buf.readableBytes() << ",total:" << totalSize << std::endl;
		return nullptr;
	}

	int id = net2int(buf.readInt32());
	int typeNameLen = net2int(buf.readInt32());
	if (typeNameLen <= 0 || typeNameLen > 1024) {
		std::cout << "type name len error:" << typeNameLen << std::endl;
		return nullptr;
	}

	std::string typeName = buf.retrieveAsString(typeNameLen);
	google::protobuf::Message *msg = createMessage(typeName);
	int msgLen = totalSize - kFlagLen - kTotalLen - kIDLen - kTypeNameLen - typeNameLen;
	if (msg && msg->ParseFromArray(buf.peek(), msgLen)) {
		buf.retrieve(msgLen);
		PackagePtr result(new Package());
		result->totalSize = totalSize;
		result->id = id;
		result->typeNameLen = typeNameLen;
		result->typeName = typeName;
		result->msgPtr = MessagePtr(msg);
		return result;
	}
	std::cout << "protobuf parse error:" << typeName << std::endl;
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