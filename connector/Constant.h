#pragma once

#include <functional>
#include "google/protobuf/message.h"

class session;

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
struct Package {
	int totalSize;
	int id;
	int typeNameLen;
	std::string typeName;
	MessagePtr msgPtr;
	int checksum;
};

typedef std::shared_ptr<session> SessionPtr;
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
typedef std::shared_ptr<Package> PackagePtr;

typedef std::function<void(const SessionPtr&, const PackagePtr&)> Task;
typedef std::function<void(int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr)> Response;

//const int kHeaderLen = sizeof(int32_t);
const int kHeaderLen = 4;