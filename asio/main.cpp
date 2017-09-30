// asio.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "Session.h"
#include "AsioServer.h"
#include <iostream>
#include "addressbook.pb.h"

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
		rsp->set_hello("hello, world!");

		PackagePtr rspPtr(new Package());
		rspPtr->id = reqPtr->id;
		rspPtr->typeNameLen = rsp->GetTypeName().length();
		rspPtr->typeName = rsp->GetTypeName();
		rspPtr->msgPtr = rsp;
		sessionPtr->replyMessage(rspPtr);
	});

	server.run();
	system("pause");
	return 0;
}
