// asio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Session.h"
#include "AsioServer.h"
#include "AsioClient.h"
#include <iostream>
#include "addressbook.pb.h"
#include <thread>
#include <direct.h>

void initlog(const std::string &path, bool console = false)
{
	google::InitGoogleLogging(path.c_str());
	std::string dir = path.substr(0, path.find_last_of('.'));
	google::SetLogDestination(google::GLOG_INFO, dir.c_str());
	FLAGS_alsologtostderr = console;
}

int main(int argc, char* argv[])
{
	initlog(argv[0], true);
	LOG(INFO) << "start...";
#if 0
	AsioServer server(5566);
	server.addHandleMessage(Test::HelloReq::descriptor()->full_name(), [](const SessionPtr &sessionPtr, const PackagePtr &reqPtr) {
		Test::HelloReq *hello = dynamic_cast<Test::HelloReq*>(reqPtr->msgPtr.get());

		std::shared_ptr<Test::HelloRsp> rsp(new Test::HelloRsp());
		rsp->set_hello(hello->address());

		PackagePtr rspPtr(new Package());
		rspPtr->id = reqPtr->id;
		rspPtr->typeNameLen = rsp->GetTypeName().length();
		rspPtr->typeName = rsp->GetTypeName();
		rspPtr->msgPtr = rsp;
		sessionPtr->replyMessage(rspPtr);
	});

	server.run();
#else
	AsioClient client("127.0.0.1:5566");
	for (int i = 0; i < 100; i++) {
		Test::HelloReq *msg = new Test::HelloReq();
		msg->set_id(1233);
		msg->set_name("hello");
		msg->set_address("dddddgasdgsdasgasdgsadgasfsadfdsgasdgsadgasdg");
		client.postMessage(MessagePtr(msg), [](int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr) {
			std::cout << rspMsgPtr->msgPtr->ShortDebugString() << std::endl;
		});

		//MessagePtr rsp;
		//client.sendMessage(MessagePtr(msg), rsp, 1000);
		//if (rsp) {
		//	Test::HelloRsp *hello = static_cast<Test::HelloRsp*>(rsp.get());
		//	LOG(INFO) << hello->hello();
		//}
	}
#endif

	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
