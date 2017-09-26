// asio.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
//#include <boost/asio/signal_set.hpp>
#include <boost/lexical_cast.hpp>

// test
#include "./addressbook.pb.h"
#include "./ProtoHelp.h"

using boost::asio::ip::tcp;

std::string endpoint2str(const tcp::endpoint &endpoint)
{
	std::string host = endpoint.address().to_string();
	host += (":" + boost::lexical_cast<std::string>(endpoint.port()));
	return host;
}

class session
	: public std::enable_shared_from_this<session>
{
public:
	session(tcp::socket socket)
		: socket_(std::move(socket))
	{
	}

	~session()
	{
		std::cout << "~session" << std::endl;
	}

	void start()
	{
		do_read();
	}

private:
	void do_read()
	{
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(data_, max_length),
			[this, self](boost::system::error_code ec, std::size_t length)
		{
			if (!ec)
			{
				std::cout << "read length:" << length << std::endl;
				do_write(length);
			}
		});
	}

	void do_write(std::size_t length)
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
			[this, self](boost::system::error_code ec, std::size_t /*length*/)
		{
			if (!ec)
			{
				do_read();
			}
		});
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
};

class server
{
public:
	server(boost::asio::io_service& io_service, short port)
		: acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
		socket_(io_service)
	{
		std::cout << "bind:" << endpoint2str(acceptor_.local_endpoint()) << std::endl;
		do_accept();
	}

private:
	void do_accept()
	{
		acceptor_.async_accept(socket_,
			[this](boost::system::error_code ec)
		{
			if (!ec) {
				std::string remoteHost = endpoint2str(socket_.remote_endpoint());
				std::shared_ptr<session> newSession = std::make_shared<session>(std::move(socket_));
				sessionList_.push_back(newSession);
				newSession->start();
				std::cout << "remote host:" << remoteHost << std::endl;
			} else {
				std::cerr << ec.message() << std::endl;
			}
			do_accept();
		});
	}

	std::vector<std::shared_ptr<session>> sessionList_;
	tcp::acceptor acceptor_;
	tcp::socket socket_;
};

int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_service io_service;
		boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
		signals.async_wait([&io_service](const boost::system::error_code& error, int signalNumber) {
			if (!error) {
				std::cout << "signals:" << signalNumber << std::endl;
			}
			io_service.stop();
		});

		server s(io_service, 5566);
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	system("pause");
	return 0;
}