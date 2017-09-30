#pragma once

#include "Constant.h"
#include <string>

class ProtoHelp
{
public:
	static int crc32(const char *start, int len);
	static int net2int(const char *buf);
	static std::string encode(const PackagePtr &package);
	static PackagePtr decode(const std::string &buf);
	static google::protobuf::Message* createMessage(const std::string & typeName);
};

