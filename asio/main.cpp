// asio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Session.h"
#include "AsioServer.h"
#include <iostream>
#include "addressbook.pb.h"

int main(int argc, char* argv[])
{
	AsioServer server(5566);
	
	server.addHandleMessage(Test::HelloReq::descriptor()->full_name(), [](const SessionPtr &sessionPtr, const MessagePtr &msgPtr) {
		Test::HelloReq *hello = dynamic_cast<Test::HelloReq*>(msgPtr.get());
		if (hello) {
			std::cout << "name:" << hello->name() << std::endl;
			std::cout << "id:" << hello->id() << std::endl;
			std::cout << "address:" << hello->address() << std::endl;
		}

		std::shared_ptr<Test::HelloRsp> rsp(new Test::HelloRsp());
		rsp->set_hello("hello, world!");
		sessionPtr->replyMessage(rsp);
	});

	server.run();
	system("pause");
	return 0;
}
