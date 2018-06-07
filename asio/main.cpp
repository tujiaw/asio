// asio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Session.h"
#include "AsioServer.h"
#include <iostream>
#include "addressbook.pb.h"
#include <thread>

int main(int argc, char* argv[])
{
	AsioServer server(5566);
	
	server.addHandleMessage(Test::HelloReq::descriptor()->full_name(), [](const SessionPtr &sessionPtr, const PackagePtr &reqPtr) {
		Test::HelloReq *hello = dynamic_cast<Test::HelloReq*>(reqPtr->msgPtr.get());
		if (hello) {
			std::cout << "name:" << hello->name() << std::endl;
			std::cout << "id:" << hello->id() << std::endl;
			std::cout << "address:" << hello->address() << std::endl;
		}

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
	std::cout << "running..." << std::endl;
	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	return 0;
}
