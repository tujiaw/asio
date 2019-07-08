#include "Session.h"
#include <deque>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include "ProtoHelp.h"
#include "Buffer.h"
#include "TaskManager.h"

using boost::asio::ip::tcp;

static const int kTempBufSize = boost::asio::detail::default_max_transfer_size;
static boost::atomic<int> s_sessionId(0);

std::string endpoint2str(const tcp::endpoint &endpoint)
{
	std::string host = endpoint.address().to_string();
	host += (":" + boost::lexical_cast<std::string>(endpoint.port()));
	return host;
}

struct SessionData {
    SessionData(boost::asio::io_service &io)
		: io_(io)
        , socket_(io)
        , id_(0)
		, sessionId_(s_sessionId++)
	{
	}

    boost::asio::io_service &io_;
	tcp::socket socket_;
	char tempBuf_[kTempBufSize];
	Buffer readBuffer_;
    std::deque<BufferPtr> writeDeque_;
	boost::mutex subscribeMutex_;
	std::set<std::string> subscribeList_;
    boost::atomic<int> id_;
	int sessionId_;
};

Session::Session(boost::asio::io_service &io, PublisherRoom &room, TaskManager &taskManager)
    : d(new SessionData(io)), room_(room), taskManager_(taskManager)
{
}

Session::~Session()
{
	std::cout << "Session delete:" << remoteEndpoint() << std::endl;
	delete d;
}

void Session::start()
{
    std::cout << "Session create:" << remoteEndpoint() << std::endl;
    room_.join(shared_from_this());
    startRead();
}

void Session::replyMessage(const PackagePtr &req, const MessagePtr &rspMsg)
{ 
	if (!req || !rspMsg) {
		return;
	}
	PackagePtr rspPtr(new Package());
    rspPtr->header = req->header;
	rspPtr->typeName = rspMsg->GetTypeName();
	rspPtr->msgPtr = rspMsg;
	postPackage(rspPtr);
}

void Session::publishMessage(const MessagePtr &msg)
{
	if (!msg) {
		return;
	}

	{
        boost::lock_guard<boost::mutex> lock(d->subscribeMutex_);
		if (std::find(d->subscribeList_.begin(), d->subscribeList_.end(), msg->GetTypeName()) == d->subscribeList_.end()) {
            DLOG(WARNING) << "publishMessage no subscribe, typeName:" << msg->GetTypeName();
			return;
		}
	}

	PackagePtr pack(new Package());
    pack->header.msgType = PacHeader::PUBSUB;
    pack->header.msgId = ++d->id_ % INT_MAX;
	pack->header.typeNameLen = msg->GetTypeName().length();
	pack->typeName = msg->GetTypeName();
	pack->msgPtr = msg;
	postPackage(pack);
}

void Session::addSubscribe(const std::string &typeName)
{
    boost::lock_guard<boost::mutex> lock(d->subscribeMutex_);
    d->subscribeList_.insert(typeName);
}

void Session::removeSubscribe(const std::string &typeName)
{
    boost::lock_guard<boost::mutex> lock(d->subscribeMutex_);
    d->subscribeList_.erase(typeName);
}

std::string Session::remoteEndpoint() const
{
    if (d->socket_.is_open()) {
        return endpoint2str(d->socket_.remote_endpoint());
    }
    return "";
}

int Session::sessionId() const
{
	return d->sessionId_;
}

tcp::socket& Session::socket()
{
    return d->socket_;
}

void Session::startRead()
{
	d->socket_.async_read_some(boost::asio::buffer(d->tempBuf_, kTempBufSize),
        boost::bind(&Session::onRead, this, _1, _2));
}

void Session::onRead(boost::system::error_code ec, std::size_t length)
{
    if (!ec) {
        d->readBuffer_.append(d->tempBuf_, length);
        do {
            PackagePtr pack = ProtoHelp::decode(d->readBuffer_);
            if (pack) {
                taskManager_.handleMessage(pack, shared_from_this());
            } else {
                break;
            }
        } while (d->readBuffer_.readableBytes() > 0);

        startRead();
    } else {
        room_.leave(shared_from_this());
        DLOG(INFO) << "Session::startRead error:" << ec.message() << "(" << ec << ")";
    }
}

void Session::write(BufferPtr writeBuffer)
{
    bool writeInProgress = !d->writeDeque_.empty();
    d->writeDeque_.push_back(writeBuffer);
    if (!writeInProgress) {
        const BufferPtr &buf = d->writeDeque_.front();
        boost::asio::async_write(d->socket_, boost::asio::buffer(buf->peek(), buf->readableBytes()),
            boost::bind(&Session::onWrite, this, _1, _2));
    }
}

void Session::onWrite(boost::system::error_code ec, std::size_t length)
{
    if (!ec) {
        d->writeDeque_.pop_front();
        if (!d->writeDeque_.empty()) {
            const BufferPtr &buf = d->writeDeque_.front();
            boost::asio::async_write(d->socket_, boost::asio::buffer(buf->peek(), buf->readableBytes()),
                boost::bind(&Session::onWrite, this, _1, _2));
        }
    } else {
        room_.leave(shared_from_this());
    }
}

void Session::postPackage(const PackagePtr &pack)
{
    BufferPtr writeBuffer(new Buffer());
    if (ProtoHelp::encode(pack, writeBuffer)) {
        d->io_.post(boost::bind(&Session::write, this, writeBuffer));
    }
}

//////////////////////////////////////////////////////////////////////////
void PublisherRoom::join(PublisherPtr publisher)
{
	std::lock_guard<std::mutex> lock(mutex_);
    publishers_.insert(publisher);
}

void PublisherRoom::leave(PublisherPtr publisher)
{
	std::lock_guard<std::mutex> lock(mutex_);
    publishers_.erase(publisher);
}

void PublisherRoom::publishMessage(const MessagePtr &msg)
{
	std::lock_guard<std::mutex> lock(mutex_);
    for (std::set<PublisherPtr>::iterator it = publishers_.begin(); it != publishers_.end(); ++it) {
		(*it)->publishMessage(msg);
	}
}
