#ifndef ASIO_BASE_ASIOSERVER_H_
#define ASIO_BASE_ASIOSERVER_H_

#include <boost/thread.hpp>
#include "Buffer.h"
#include "TaskManager.h"
#include "Session.h"

using boost::asio::ip::tcp;
class AsioServer
{
public:
	explicit AsioServer(unsigned short port);
    AsioServer(unsigned short port, unsigned int workSize, unsigned int poolSize);
	virtual ~AsioServer();

	void run();
    void asyncRun();
    bool isRunning() const;
	void stop();

	void addHandleMessage(const std::string &protoName, const Task &task);
	void publishMessage(const MessagePtr &msg);

private:
    AsioServer(const AsioServer&);
    AsioServer& operator=(const AsioServer&);
	void init();
	void startAccept();
    void doAccept(SessionPtr session, boost::system::error_code ec);
    void doSubscribe(const SessionPtr &sessionPtr, const PackagePtr &reqPtr);
    void doHeartbeat(const SessionPtr &sessionPtr, const PackagePtr &reqPtr);

private:
	boost::asio::io_service io_;
	tcp::acceptor acceptor_;
	boost::thread runthread_;
    TaskManager taskManager_;
    PublisherRoom room_;
};

#endif // ASIO_BASE_ASIOSERVER_H_
