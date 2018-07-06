#pragma once

#include "desc.h"
#include <thread>

class ThreadPool;

class TaskManager
{
public:
	static TaskManager* instance();

	void addHandleTask(const std::string &protoName, const Task &task);
	void handleMessage(const PackagePtr &msgPtr, const SessionPtr &sessionPtr);
	void destory();

private:
	TaskManager();
	TaskManager(TaskManager&) = delete;
	void operator=(TaskManager&) = delete;

private:
	static TaskManager *s_inst;
    D_PRIVATE(TaskManagerPrivate)
};

