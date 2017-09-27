#pragma once

#include <functional>
#include "google/protobuf/message.h"

class Session;
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
typedef std::shared_ptr<Session> SessionPtr;
typedef std::function<void(const SessionPtr&, const MessagePtr&)> Task;

class ThreadPool;
class TaskManager
{
public:
	static TaskManager* instance();

	void addHandleTask(const std::string &protoName, const Task &task);
	void handleMessage(const MessagePtr &msgPtr, const SessionPtr &sessionPtr);
	void destory();

private:
	TaskManager();
	TaskManager(TaskManager&);
	void operator=(TaskManager&);

private:
	std::unique_ptr<ThreadPool> pool_;
	std::map<std::string, Task> handler_;
};

