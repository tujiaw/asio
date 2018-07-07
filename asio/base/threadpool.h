#ifndef ASIO_BASE_THREADPOOL_H
#define ASIO_BASE_THREADPOOL_H

#include <limits>
#include <memory>
#include <vector>
#include "desc.h"

class Runnable;
class ThreadPoolPrivate;

// �̳߳أ��ⲿ�ӿڹ��û�ʹ��
class ThreadPool {
public:
	ThreadPool(void);
	~ThreadPool(void);
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	// ����һ���������û�п��е��߳̾����
	void start(Runnable* runnable, int priority = 0);
	// ����ȥ����һ���������û�п����߳̾ͷ���false
	bool tryStart(Runnable* runnable);

	// ����ʱ�䣬�ȴ�ʱ��
	unsigned long int expiryTimeout(void) const;
	void setExpiryTimeout(unsigned long int v);

	// ����߳�������ʼ��ΪCPU�ں���
	std::size_t maxThreadCount(void) const;
	void setMaxThreadCount(std::size_t n);

	// ������߳���
	std::size_t activeThreadCount(void) const;

	// ������д�С
	std::size_t queueSize(void) const;

	// �����̣߳���ʱ��ʹ�ã�����һ������һ��
	void reserveThread(void);
	// �ͷ��̣߳����������߳��ó���ʹ�ã�����һ���ͷ�һ��
	void releaseThread(void);

	// �ȴ����м�����߳�ִ�����
	bool waitForDone(unsigned long int timeout = std::numeric_limits<unsigned long int>::max());

	// ���������δִ�е�����
	void clear(void);

	// ɾ��һ������
	void cancel(Runnable* runnable);

private:
	D_PRIVATE(ThreadPoolPrivate)
};


#endif // ASIO_BASE_THREADPOOL_H
