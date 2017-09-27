#include "Session.h"
#include "ProtoHelp.h"

Session::Session(tcp::socket socket)
	: socket_(std::move(socket))
{
	std::cout << "Session create:" << this << std::endl;
}

Session::~Session()
{
	std::cout << "Session delete:" << this << std::endl;
}

void Session::start()
{
	onRead();
}

void Session::replyMessage()
{
	std::cout << "reply message" << std::endl;
}


void Session::onRead()
{
	auto self(shared_from_this());
	socket_.async_read_some(boost::asio::buffer(tempBuf, kTempBufSize),
		[this, self](boost::system::error_code ec, std::size_t length)
	{
		if (!ec) {
			buffer_.append(tempBuf, length);
			while (buffer_.readableBytes() > 4) {
				int packLen = buffer_.peekInt32();
				if (packLen <= 0) {
					buffer_.retrieve(1);
				} else if (packLen > 1024 * 1024 * 10) {
					buffer_.retrieve(1);
				} else if (buffer_.readableBytes() >= (size_t)packLen + 4) {
					std::string packBuf;
					packBuf.resize(packLen);
					memcpy(&packBuf[0], buffer_.peek() + 4, packLen);
					google::protobuf::Message* msg = ProtoHelp::decode(packBuf);
					if (msg) {
						buffer_.retrieve(packLen + 4);
						std::shared_ptr<google::protobuf::Message> msgPtr(msg);
						TaskManager::instance()->handleMessage(msgPtr, self);
					} else {
						buffer_.retrieve(1);
					}
				} else {
					break;
				}
			}

			onRead();
		}
	});
}

void Session::onWrite(std::size_t length)
{
	//auto self(shared_from_this());
	//boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
	//	[this, self](boost::system::error_code ec, std::size_t /*length*/)
	//{
	//	if (!ec)
	//	{
	//		onRead();
	//	}
	//});
}
