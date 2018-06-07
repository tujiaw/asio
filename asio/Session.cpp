#include "Session.h"
#include "ProtoHelp.h"

session::session(tcp::socket socket)
	: socket_(std::move(socket))
{
	std::cout << "session create:" << this << std::endl;
}

session::~session()
{
	std::cout << "session delete:" << this << std::endl;
}

void session::start()
{
	onRead();
}

void session::replyMessage(const PackagePtr &rspPtr)
{
	if (rspPtr) {
		std::string buf = ProtoHelp::encode(rspPtr);
		writeBuffer_.append(buf);
		onWrite();
	}
}

void session::onRead()
{
	auto self(shared_from_this());
	socket_.async_read_some(boost::asio::buffer(tempBuf_, kTempBufSize),
		[this, self](boost::system::error_code ec, std::size_t length)
	{
		if (!ec) {
			readBuffer_.append(tempBuf_, length);
			PackagePtr pack = ProtoHelp::decode(readBuffer_);
			if (pack) {
				TaskManager::instance()->handleMessage(pack, self);
			}

			onRead();
		}
	});
}

void session::onWrite()
{
	int readLen = writeBuffer_.readableBytes();
	if (readLen <= 0) {
		return;
	}

	auto self(shared_from_this());
	boost::asio::async_write(socket_, boost::asio::buffer(writeBuffer_.peekRetrieve(readLen), readLen),
		[this, self](boost::system::error_code ec, std::size_t length)
	{
		if (!ec)
		{
			onWrite();
			std::cout << "write length:" << length << std::endl;
		}
	});
}
