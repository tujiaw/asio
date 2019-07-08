#ifndef ASIO_BASE_TASKMANAGER_H_
#define ASIO_BASE_TASKMANAGER_H_

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "Package.h"

class ThreadPool;
class Worker;
typedef std::shared_ptr<Worker> WorkerPtr;
typedef std::shared_ptr<ThreadPool> ThreadPoolPtr;

class TaskManager
{
public:
    class TaskManagerPrivate {
    public:
        TaskManagerPrivate();
        TaskManagerPrivate(unsigned int workSize, unsigned int poolSize);
        void init(unsigned int workSize, unsigned int poolSize);
        ThreadPoolPtr pool_;
        std::map<std::string, Task> handler_;
        std::vector<WorkerPtr> workers_;
    };

    TaskManager();
    TaskManager(unsigned int workSize, unsigned int poolSize);
	void addHandleTask(const std::string &protoName, const Task &task);
	void handleMessage(const PackagePtr &msgPtr, const SessionPtr &sessionPtr);
	void destory();

private:
	TaskManager(TaskManager&);
    TaskManager& operator=(TaskManager&);
    void handleTask(const Task &task, const PackagePtr &pacPtr, const SessionPtr &sessionPtr);

private:
    D_PRIVATE(TaskManagerPrivate)
};

#endif // ASIO_BASE_TASKMANAGER_H_
