#ifndef ASIO_BASE_SESSION_H_
#define ASIO_BASE_SESSION_H_

#include <mutex>
#include <memory>
#include <set>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include "Package.h"

using boost::asio::ip::tcp;

class PublisherRoom;
class TaskManager;
struct SessionData;

class Publisher {
public:
    virtual ~Publisher() {}
    virtual void publishMessage(const MessagePtr &msg) = 0;
};

typedef std::shared_ptr<Publisher> PublisherPtr;

class Session : public Publisher, public std::enable_shared_from_this<Session>
{
public:
    explicit Session(boost::asio::io_service &io, PublisherRoom &room, TaskManager &taskManager);
    ~Session();
	
	void start();
	void replyMessage(const PackagePtr &req, const MessagePtr &rspMsg);
	void publishMessage(const MessagePtr &msg);
	void addSubscribe(const std::string &typeName);
	void removeSubscribe(const std::string &typeName);
	std::string remoteEndpoint() const;
	int sessionId() const;
    tcp::socket& socket();

private:
    Session(const Session&);
    Session& operator=(const Session&);
	void startRead();
    void onRead(boost::system::error_code ec, std::size_t length);
	void write(BufferPtr writeBuffer);
    void onWrite(boost::system::error_code ec, std::size_t length);
	void postPackage(const PackagePtr &pack);

private:
	SessionData *d;
    PublisherRoom &room_;
    TaskManager &taskManager_;
};

typedef std::shared_ptr<Session> SessionPtr;

class PublisherRoom
{
public:
	void join(PublisherPtr publisher);
    void leave(PublisherPtr publisher);
	void publishMessage(const MessagePtr &msg);

private:
	std::mutex mutex_;
    std::set<PublisherPtr> publishers_;
};

#endif // ASIO_BASE_SESSION_H_