#ifndef THREADPOOL_P_H
#define THREADPOOL_P_H

#include <condition_variable>
#include <list>
#include <mutex>
#include <set>
#include <utility>

class Runnable;
class ThreadPoolThread;

// �̳߳��ڲ��ӿڣ����еĳ�Ա��������װ������
class ThreadPoolPrivate {
public:
	ThreadPoolPrivate(void);

	// ����ȥִ��һ��������������߳��Ѿ����˷���false
	bool tryStart(Runnable* runnable);

	// ����������뵽���ȼ���ͬ���������
	void enqueueTask(Runnable* runnable, int priority = 0);

	std::size_t activeThreadCount(void) const;

	// ʹ�ø�����߳�ִ�и��������
	void tryToStartMoreThreads(void);
	bool tooManyThreadsActive(void) const;

	void startThread(Runnable* runnable = nullptr);
	void reset(void);
	bool waitForDone(unsigned long int msecs);
	void clear(void);

	// ͵��һ������ɾ��һ������
	bool stealRunnable(Runnable* runnable);
	// ͵��һ������ɾ��һ�����񣩲����д�����
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
