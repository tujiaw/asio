#pragma once

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "Package.h"

namespace ningto
{
    class ThreadPool;
    class Worker;
    typedef std::shared_ptr<Worker> WorkerPtr;
    typedef std::shared_ptr<ThreadPool> ThreadPoolPtr;

    class TaskManager
    {
    public:
        class TaskManagerPrivate {
        public:
            TaskManagerPrivate(unsigned int workSize, unsigned int poolSize);
            void init(unsigned int workSize, unsigned int poolSize);
            ThreadPoolPtr pool_;
            std::map<std::string, Task> handler_;
            std::vector<WorkerPtr> workers_;
        };

        TaskManager(unsigned int workSize, unsigned int poolSize);
        TaskManager(TaskManager&) = delete;
        TaskManager& operator=(TaskManager&) = delete;
        void addHandleTask(const std::string &protoName, const Task &task);
        void setPreHandleTask(const Task &task);
        void handleMessage(const PackagePtr &msgPtr, const SessionPtr &sessionPtr);
        void destory();

    private:
        Task preTask_{ nullptr };
        D_PRIVATE(TaskManagerPrivate)
    };
}