#include "TaskManager.h"
#include <map>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "Session.h"
#include "ThreadPool.h"

class Worker {
public:
    Worker() : work(io), t(boost::bind(&boost::asio::io_service::run, &io)) {}
    ~Worker() { io.stop(); }
    boost::asio::io_service io;
    boost::asio::io_service::work work;
    boost::thread t;
};

TaskManager::TaskManagerPrivate::TaskManagerPrivate()
{
    int count = std::max(boost::thread::hardware_concurrency(), 1u);
    init(count, count);
}

TaskManager::TaskManagerPrivate::TaskManagerPrivate(unsigned int workSize, unsigned int poolSize)
{
    init(std::max(workSize, 1u), std::max(poolSize, 1u));
}

void TaskManager::TaskManagerPrivate::init(unsigned int workSize, unsigned int poolSize)
{
    for (int i = 0; i < workSize; i++) {
        workers_.push_back(std::make_shared<Worker>());
        printf("create work id:0x%x\n", workers_[i]->t.get_id());
    }

    pool_.reset(new ThreadPool(poolSize));
    pool_->start();
}

TaskManager::TaskManager()
    : d_ptr(new TaskManagerPrivate())
{
}

TaskManager::TaskManager(unsigned int workSize, unsigned int poolSize)
    : d_ptr(new TaskManagerPrivate(workSize, poolSize))
{
}

void TaskManager::addHandleTask(const std::string &protoName, const Task &task)
{
	d_func()->handler_[protoName] = task;
}

void TaskManager::handleMessage(const PackagePtr &pacPtr, const SessionPtr &sessionPtr)
{
	std::string name = boost::move(pacPtr->typeName);
    std::map<std::string, Task>::iterator iter = d_func()->handler_.find(name);
    if (iter != d_func()->handler_.end()) {
        const Task &task = iter->second;
        if (pacPtr->header.isorder) {
			int workId = sessionPtr->sessionId() % d_func()->workers_.size();
            d_func()->workers_[workId]->io.post(boost::bind(&TaskManager::handleTask, this, task, pacPtr, sessionPtr));
        } else {
            d_func()->pool_->run(boost::bind(&TaskManager::handleTask, this, task, pacPtr, sessionPtr));
        }
	} else {
		std::cout << "message discard:" << name;
	}
}

void TaskManager::destory()
{
    d_func()->pool_->stop();
}

void TaskManager::handleTask(const Task &task, const PackagePtr &pacPtr, const SessionPtr &sessionPtr)
{
    task(sessionPtr, pacPtr);
}
