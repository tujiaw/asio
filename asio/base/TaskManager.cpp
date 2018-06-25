#include "TaskManager.h"
#include <mutex>
#include "runnable.h"
#include "threadpool.h"

class TaskRunnable : public Runnable
{
public:
	TaskRunnable(const SessionPtr &sessionPtr, const PackagePtr &pacPtr, const Task &task)
		: sessionPtr_(sessionPtr), pacPtr_(pacPtr), task_(task)
	{
	}

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
	: pool_(new ThreadPool())
{

}

void TaskManager::addHandleTask(const std::string &protoName, const Task &task)
{
	handler_[protoName] = task;
}

void TaskManager::handleMessage(const PackagePtr &pacPtr, const SessionPtr &sessionPtr)
{
	std::string name = std::move(pacPtr->typeName);
	auto iter = handler_.find(name);
	if (iter != handler_.end()) {
		TaskRunnable *runable = new TaskRunnable(sessionPtr, pacPtr, iter->second);
		pool_->start(runable);
	} else {
		std::cout << "message discard:" << name;
	}
}

void TaskManager::destory()
{
	pool_->clear();
	pool_->waitForDone();
}