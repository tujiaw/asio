#ifndef ASIO_BASE_ASIOCLIENT_H_
#define ASIO_BASE_ASIOCLIENT_H_

#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <deque>
#include <mutex>
#include "Package.h"
#include "Buffer.h"

using boost::asio::ip::tcp;
class AsioClient {
public:
    class MsgCache {
    public:
        MsgCache(AsioClient *client, const PackagePtr &p, const Response &r, int msTimeout)
            : pac(p), res(r), timer(client->io_, boost::posix_time::milliseconds(msTimeout)) { 
            timer.async_wait(boost::bind(&AsioClient::onTimeout, client, _1, p->header.msgId));
        }
        ~MsgCache() {
            timer.cancel_one();
        }
        PackagePtr pac;
        Response res;
        boost::asio::deadline_timer timer;
    };
    typedef std::shared_ptr<MsgCache> MsgCachePtr;

    explicit AsioClient(const std::vector<std::string> &addressList, int reconnectSeconds = 3, int heartbeatSeconds = 0);
	virtual ~AsioClient();

	void addHandlePublish(const std::string &typeName, const PublishFunc &func);
    void setConnectSuccess(const boost::function<void()> &cb);
	void start();
	void stop();
	bool stopped() const;

    int sendMessage(const MessagePtr &msgPtr, MessagePtr &rspPtr, int msTimeout = kMsTimeout);
    int postMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout = kMsTimeout);
	int postOrderMessage(const MessagePtr &msgPtr, const Response &res, int msTimeout = kMsTimeout);

private:
    AsioClient(const AsioClient&);
    AsioClient& operator=(const AsioClient&);
    void connect();
    void doConnect(const boost::system::error_code &ec);
	int postMessage(bool isOrder, const MessagePtr &msgPtr, const Response &res, int msTimeout = kMsTimeout);
	void close();
    void startRead();
    void doRead(boost::system::error_code ec, std::size_t length);
    void startWrite(const PackagePtr &pacPtr);
	void startWrite();
    void doWrite(boost::system::error_code ec, std::size_t length);
    void onTimeout(boost::system::error_code err, int msgId);
	void doClose();
	void startSubscribe();
    void doSubscribe(int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr);
    void startHeartbeatTimer();
    void startHeartbeat(const boost::system::error_code &e);
    void doHeartbeat(int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr);
    void doResponse(const PackagePtr &pack);

private:
    std::size_t addressIndex_;
	std::vector<std::string> addressList_;
	boost::asio::io_service io_;
    boost::asio::io_service ioDoResponse_;
    boost::asio::io_service::work workDoResponse_;
	tcp::socket socket_;
	std::deque<PackagePtr> pendingList_;
	boost::atomic<int> id_;
	boost::thread runthread_;
    boost::thread threadDoResponse_;
	std::unique_ptr<boost::asio::deadline_timer> heartbeatTimer_;
    int reconnectSeconds_;
	int heartbeatSeconds_;
	boost::atomic<bool> isOnline_;
    boost::function<void()> connectSuccess_;

    static const int kTempBufSize = boost::asio::detail::default_max_transfer_size;
    char tempBuf_[kTempBufSize];
	Buffer readBuffer_;

	boost::mutex mutex_;
	boost::condition_variable cond_;

	boost::mutex responseMutex_;
    std::map<int, MsgCachePtr> responseMap_;
	std::map<std::string, PublishFunc> publishMap_;
};

#endif // ASIO_BASE_ASIOCLIENT_H_