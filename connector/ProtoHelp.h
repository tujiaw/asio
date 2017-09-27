#pragma once

#include <google/protobuf/message.h>
#include <string>

class ProtoHelp
{
public:
	static int crc32(const char *start, int len);
	static int net2int(const char *buf);
	static std::string encode(const google::protobuf::Message &message);
	static google::protobuf::Message* decode(const std::string &buf);
	static google::protobuf::Message* createMessage(const std::string & typeName);
};

