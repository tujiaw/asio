#ifndef RUNNABLE_H
#define RUNNABLE_H

// �����е�����Ĭ��֧���Զ����٣�ͨ������run��ʵ��Ҫִ�е�����
class Runnable {
public:
	Runnable(void) : m_ref(0) {}
	virtual ~Runnable(void) {}
	virtual void run(void) = 0;

	bool autoDelete(void) const { return this->m_ref != -1; }
	void setAutoDelete(bool v) { this->m_ref = v ? 0 : -1; }

private:
	int m_ref;

	friend class ThreadPool;
	friend class ThreadPoolPrivate;
	friend class ThreadPoolThread;
};

#endif // RUNNABLE_H
