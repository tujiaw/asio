#pragma once

#include <string>
#include "Package.h"

namespace ningto
{
    class Buffer;
    class ProtoHelp
    {
    public:
        static void initEnableCompressSize(int size = 1024);
        static int enableCompressSize();
        static int net2int(const char *buf);
        static int net2int(int i);
        static int int2net(int i);
        static short net2short(short i);
        static short short2net(short i);
        static bool encode(const PackagePtr &package, BufferPtr &buffer);
        static PackagePtr decode(Buffer &buf);
        static google::protobuf::Message* createMessage(const std::string & typeName);

    private:
        static int s_enableCompressSize;
    };
}