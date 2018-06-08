#include "Session.h"
#include "ProtoHelp.h"

session::session(tcp::socket socket)
	: socket_(std::move(socket)), io_(socket_.get_io_service())
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
		BufferPtr writeBuffer = ProtoHelp::encode(rspPtr);
		if (writeBuffer) {
			io_.post([this, writeBuffer]{ onWrite(writeBuffer); });
			//onWrite(writeBuffer);
		}
	}
}

void session::onRead()
{
	auto self(shared_from_this());
	socket_.async_read_some(boost::asio::buffer(&tempBuf_[0], kTempBufSize),
		[this, self](boost::system::error_code ec, std::size_t length)
	{
		if (!ec) {
			readBuffer_.append(&tempBuf_[0], length);
			do {
				PackagePtr pack = ProtoHelp::decode(readBuffer_);
				if (pack) {
					TaskManager::instance()->handleMessage(pack, self);
				} else {
					break;
				}
			} while (1);

			onRead();
		} else {
			std::cout << "onRead error:" << ec << std::endl;
		}
	});
}

void session::onWrite(BufferPtr writeBuffer)
{
	int writeLen = writeBuffer->readableBytes();
	if (writeLen <= 0) {
		return;
	}

	auto self(shared_from_this());
	boost::asio::async_write(socket_, boost::asio::buffer(writeBuffer->peek(), writeLen),
		[this, self, writeBuffer](boost::system::error_code ec, std::size_t length)
	{
		if (!ec) {
			writeBuffer->retrieve(length);
			onWrite(writeBuffer);
		}
	});
}
