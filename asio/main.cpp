// asio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Session.h"
#include "AsioServer.h"
#include "AsioClient.h"
#include <iostream>
#include "addressbook.pb.h"
#include <thread>

int main(int argc, char* argv[])
{
#if 1
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
	client.run();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	int index = 0;
	for (int i = 0; i < 100; i++) {
		Test::HelloReq *msg = new Test::HelloReq();
		msg->set_id(1233);
		msg->set_name("hello");
		msg->set_address("dddddgasdgsdasgasdgsadgasfsadfdsgasdgsadgasdg");
		client.postMessage(MessagePtr(msg), [&index](int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr) {
			std::cout << rspMsgPtr->msgPtr->ShortDebugString() << std::endl;
			std::cout << "index:" << ++index << std::endl;
		});
		//std::this_thread::sleep_for(std::chrono::seconds(1));
	}
#endif

	std::cout << "byebye..." << std::endl;
	while (1) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
