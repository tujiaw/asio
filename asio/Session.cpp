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

void session::replyMessage(const MessagePtr &msgPtr)
{
	if (msgPtr) {
		std::string buf = ProtoHelp::encode(*msgPtr.get());
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
			if (readBuffer_.readableBytes() > 4) {
				int packLen = readBuffer_.peekInt32();
				if (packLen <= 0) {
					readBuffer_.retrieve(1);
				} else if (packLen > 1024 * 1024 * 10) {
					readBuffer_.retrieve(1);
				} else if (readBuffer_.readableBytes() >= (size_t)packLen + 4) {
					std::string packBuf;
					packBuf.resize(packLen);
					memcpy(&packBuf[0], readBuffer_.peek() + 4, packLen);
					google::protobuf::Message* msg = ProtoHelp::decode(packBuf);
					if (msg) {
						readBuffer_.retrieve(packLen + 4);
						std::shared_ptr<google::protobuf::Message> msgPtr(msg);
						TaskManager::instance()->handleMessage(msgPtr, self);
					} else {
						readBuffer_.retrieve(1);
					}
				}
			}

			onRead();
		}
	});
}

void session::onWrite()
{
	if (writeBuffer_.readableBytes() <= 0) {
		return;
	}

	auto self(shared_from_this());
	boost::asio::async_write(socket_, boost::asio::buffer(writeBuffer_.peek(), writeBuffer_.readableBytes()),
		[this, self](boost::system::error_code ec, std::size_t length)
	{
		if (!ec)
		{
			writeBuffer_.retrieve(length);
			onWrite();
			std::cout << "write length:" << length << std::endl;
		}
	});
}
