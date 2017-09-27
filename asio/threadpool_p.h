#ifndef THREADPOOL_P_H
#define THREADPOOL_P_H

#include <condition_variable>
#include <list>
#include <mutex>
#include <set>
#include <utility>

class Runnable;
class ThreadPoolThread;

// 线程池内部接口，所有的成员变量都封装在这里
class ThreadPoolPrivate {
public:
	ThreadPoolPrivate(void);

	// 试着去执行一个任务，如果激活线程已经满了返回false
	bool tryStart(Runnable* runnable);

	// 将新任务插入到优先级相同的任务后面
	void enqueueTask(Runnable* runnable, int priority = 0);

	std::size_t activeThreadCount(void) const;

	// 使用更多的线程执行更多的任务
	void tryToStartMoreThreads(void);
	bool tooManyThreadsActive(void) const;

	void startThread(Runnable* runnable = nullptr);
	void reset(void);
	bool waitForDone(unsigned long int msecs);
	void clear(void);

	// 偷窃一个任务（删除一个任务）
	bool stealRunnable(Runnable* runnable);
	// 偷窃一个任务（删除一个任务）并运行此任务
	void stealAndRunRunnable(Runnable* runnable);

	mutable std::mutex mutex;
	std::set<ThreadPoolThread*> allThreads;
	std::list<ThreadPoolThread*> waitingThreads;
	std::list<ThreadPoolThread*> expiredThreads;
	std::list<std::pair<Runnable*, int> > queue;
	std::condition_variable noActiveThreads;

	bool isExiting;
	unsigned long int expiryTimeout;
	std::size_t maxThreadCount;
	std::size_t reservedThreads;
	std::size_t activeThreads;
};

#endif // THREADPOOL_P_H
