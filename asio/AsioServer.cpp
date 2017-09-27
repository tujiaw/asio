#include "AsioServer.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "ProtoHelp.h"
#include "Session.h"

std::string endpoint2str(const tcp::endpoint &endpoint)
{
	std::string host = endpoint.address().to_string();
	host += (":" + boost::lexical_cast<std::string>(endpoint.port()));
	return host;
}


AsioServer::AsioServer(unsigned short port)
	: acceptor_(io_, tcp::endpoint(tcp::v4(), port))
	, socket_(io_)
{
	onAccept();
}


AsioServer::~AsioServer()
{
}

void AsioServer::run()
{
	runthread_ = std::move(std::thread([this]() {
		io_.run();
	}));
}

void AsioServer::stop()
{
	io_.stop();
	runthread_.join();
}

void AsioServer::onAccept()
{
	acceptor_.async_accept(socket_,
		[this](boost::system::error_code ec)
	{
		if (!ec) {
			std::string remoteHost = endpoint2str(socket_.remote_endpoint());
			std::make_shared<session>(std::move(socket_))->start();
			std::cout << "remote host:" << remoteHost << std::endl;
		} else {
			std::cerr << ec.message() << std::endl;
		}
		onAccept();
	});
}

void AsioServer::addHandleMessage(const std::string &protoName, const Task &task)
{
	TaskManager::instance()->addHandleTask(protoName, task);
}