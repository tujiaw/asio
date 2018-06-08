#pragma once

#include "Constant.h"
#include <string>

class Buffer;
class ProtoHelp
{
public:
	static int crc32(const char *start, int len);
	static int net2int(const char *buf);
	static int net2int(int i);
	static BufferPtr encode(const PackagePtr &package);
	static PackagePtr decode(Buffer &buf);
	static google::protobuf::Message* createMessage(const std::string & typeName);
};

