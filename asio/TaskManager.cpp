#include "TaskManager.h"
#include "runnable.h"
#include "threadpool.h"

class TaskRunnable : public Runnable
{
public:
	TaskRunnable(const SessionPtr &sessionPtr, const MessagePtr &msgPtr, const Task &task)
		: sessionPtr_(sessionPtr), msgPtr_(msgPtr), task_(task)
	{
	}

	void run()
	{
		if (task_) {
			task_(sessionPtr_, msgPtr_);
		}
	}

private:
	SessionPtr sessionPtr_;
	MessagePtr msgPtr_;
	Task task_;
};

TaskManager* TaskManager::instance()
{
	static TaskManager s_inst;
	return &s_inst;
}

TaskManager::TaskManager()
	: pool_(new ThreadPool())
{

}

void TaskManager::addHandleTask(const std::string &protoName, const Task &task)
{
	handler_[protoName] = task;
}

void TaskManager::handleMessage(const MessagePtr &msgPtr, const SessionPtr &sessionPtr)
{
	if (!msgPtr) {
		return;
	}

	std::string name = std::move(msgPtr->GetDescriptor()->full_name());
	auto iter = handler_.find(name);
	if (iter != handler_.end()) {
		TaskRunnable *runable = new TaskRunnable(sessionPtr, msgPtr, iter->second);
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