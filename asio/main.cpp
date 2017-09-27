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
	
	server.addHandleMessage(Test::Hello::descriptor()->full_name(), [](const SessionPtr &sessionPtr, const MessagePtr &msgPtr) {
		Test::Hello *hello = reinterpret_cast<Test::Hello*>(msgPtr.get());
		if (hello) {
			std::cout << "name:" << hello->name() << std::endl;
			std::cout << "id:" << hello->id() << std::endl;
			std::cout << "address:" << hello->address() << std::endl;
		}
		sessionPtr->replyMessage();
	});

	server.run();
	system("pause");
	return 0;
}
