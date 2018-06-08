#pragma once

#include <functional>
#include "google/protobuf/message.h"
#include <glog/logging.h>

class session;
class Buffer;
struct Package;

typedef std::shared_ptr<session> SessionPtr;
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
typedef std::shared_ptr<Package> PackagePtr;
typedef std::shared_ptr<Buffer> BufferPtr;

typedef std::function<void(const SessionPtr&, const PackagePtr&)> Task;
typedef std::function<void(int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr)> Response;

struct Package {
	Package() 
	: totalSize(0)
	, id(0)
	, typeNameLen(0)
	, msgPtr(nullptr)
	{
		flag[0] = 'P';
		flag[1] = 'P';
	}

	char flag[2];			// ��Ϣ��־'P','P'
	int totalSize;			// ����package��С

	int id;					// ��ϢID
	int typeNameLen;		// ����������
	std::string typeName;	// ������
	MessagePtr msgPtr;		// protobuf��Ϣ
};

enum ErrorCode {
	Success = 0,
	Unkown,
	Timeout,
};