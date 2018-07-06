#include "TaskManager.h"
#include <mutex>
#include <boost/asio/io_context.hpp>
#include "runnable.h"
#include "threadpool.h"

class TaskRunnable : public Runnable
{
public:
	TaskRunnable(const SessionPtr &sessionPtr, const PackagePtr &pacPtr, const Task &task)
		: sessionPtr_(sessionPtr), pacPtr_(pacPtr), task_(task)
	{}

	void run()
	{
		if (task_) {
			task_(sessionPtr_, pacPtr_);
		}
	}

private:
	SessionPtr sessionPtr_;
	PackagePtr pacPtr_;
	Task task_;
};

typedef std::pair<boost::asio::io_context*, boost::asio::io_context::work*> WorkContext;

class TaskManagerPrivate {
public:
    TaskManagerPrivate() 
        : workThreadCount_(std::max(std::thread::hardware_concurrency() * 2, 1u)),
        pool_(new ThreadPool())
    {
        for (int i = 0; i < workThreadCount_; i++) {
            WorkContext w;
            w.first = new boost::asio::io_context();
            w.second = new boost::asio::io_context::work(*w.first);
            works_.push_back(w);
        }
    }

    ~TaskManagerPrivate()
    {
        for (int i = 0; i < workThreadCount_; i++) {
            delete works_[i].second;
            delete works_[i].first;
        }
    }

    int workThreadCount_;
    std::unique_ptr<ThreadPool> pool_;
    std::map<std::string, Task> handler_;
    std::vector<WorkContext> works_;
};

TaskManager* TaskManager::s_inst = nullptr;
TaskManager* TaskManager::instance()
{
	static std::once_flag instanceFlag;
	std::call_once(instanceFlag, []() {
		s_inst = new TaskManager();
	});
	return s_inst;
}

TaskManager::TaskManager()
    : d_ptr(new TaskManagerPrivate())
{
}

void TaskManager::addHandleTask(const std::string &protoName, const Task &task)
{
	d_func()->handler_[protoName] = task;
}

void TaskManager::handleMessage(const PackagePtr &pacPtr, const SessionPtr &sessionPtr)
{
	std::string name = std::move(pacPtr->typeName);
    auto iter = d_func()->handler_.find(name);
    if (iter != d_func()->handler_.end()) {
        if (pacPtr->header.isorder) {

        } else {
            TaskRunnable *runable = new TaskRunnable(sessionPtr, pacPtr, iter->second);
            d_func()->pool_->start(runable);
        }
	} else {
		std::cout << "message discard:" << name;
	}
}

void TaskManager::destory()
{
    d_func()->pool_->clear();
    d_func()->pool_->waitForDone();
}