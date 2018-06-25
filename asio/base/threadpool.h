#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <limits>
#include <memory>
#include <vector>
#include "desc.h"

class Runnable;
class ThreadPoolPrivate;

// 线程池，外部接口供用户使用
class ThreadPool {
public:
	ThreadPool(void);
	~ThreadPool(void);
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	// 启动一个任务，如果没有空闲的线程就入队
	void start(Runnable* runnable, int priority = 0);
	// 试着去启动一个任务，如果没有空闲线程就返回false
	bool tryStart(Runnable* runnable);

	// 到期时间，等待时间
	unsigned long int expiryTimeout(void) const;
	void setExpiryTimeout(unsigned long int v);

	// 最大线程数，初始化为CPU内核数
	std::size_t maxThreadCount(void) const;
	void setMaxThreadCount(std::size_t n);

	// 激活的线程数
	std::size_t activeThreadCount(void) const;

	// 任务队列大小
	std::size_t queueSize(void) const;

	// 保留线程，暂时不使用，调用一次新增一个
	void reserveThread(void);
	// 释放线程，将保留的线程拿出来使用，调用一次释放一次
	void releaseThread(void);

	// 等待所有激活的线程执行完成
	bool waitForDone(unsigned long int timeout = std::numeric_limits<unsigned long int>::max());

	// 清理掉所有未执行的任务
	void clear(void);

	// 删除一个任务
	void cancel(Runnable* runnable);

private:
	D_PRIVATE(ThreadPoolPrivate)
};


#endif // THREADPOOL_H
