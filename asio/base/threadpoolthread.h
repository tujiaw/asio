#ifndef ASIO_BASE_THREADPOOLTHREAD_H
#define ASIO_BASE_THREADPOOLTHREAD_H

#include <condition_variable>

class Runnable;
class ThreadPoolPrivate;

namespace std {
	class thread;
}

class ThreadPoolThread {
public:
	ThreadPoolThread(ThreadPoolPrivate* manager);

	// 线程启动后立即调用
	void operator()(void);

	void registerThreadInactive(void);

	std::condition_variable runnableReady;
	ThreadPoolPrivate* manager;
	Runnable* runnable;
	std::thread* thread;
};

#endif // ASIO_BASE_THREADPOOLTHREAD_H
