#pragma once

#include <functional>
#include "google/protobuf/message.h"

class session;

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
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

typedef std::shared_ptr<session> SessionPtr;
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
typedef std::shared_ptr<Package> PackagePtr;

typedef std::function<void(const SessionPtr&, const PackagePtr&)> Task;
typedef std::function<void(int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr)> Response;
