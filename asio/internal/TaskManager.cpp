#include "TaskManager.h"
#include <map>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "Session.h"
#include "ThreadPool.h"
#include "tool/util.h"

namespace ningto
{
    class Worker {
    public:
        Worker() : work(io), t(boost::bind(&boost::asio::io_service::run, &io)) {}
        Worker(const Worker&) = delete;
        Worker& operator=(const Worker&) = delete;
        boost::asio::io_service io;
        boost::asio::io_service::work work;
        boost::thread t;
    };

    TaskManager::TaskManagerPrivate::TaskManagerPrivate(unsigned int workSize, unsigned int poolSize)
    {
        init(std::max(workSize, 1u), std::max(poolSize, 1u));
    }

    void TaskManager::TaskManagerPrivate::init(unsigned int workSize, unsigned int poolSize)
    {
        LOG(INFO) << "TaskManager init workSize:" << workSize << ", poolSize:" << poolSize;
        for (unsigned int i = 0; i < workSize; i++) {
            workers_.push_back(std::make_shared<Worker>());
            LOG(INFO) << "create work index:" << i << ", id:" << workers_[i]->t.get_id();
        }

        pool_.reset(new ThreadPool(poolSize));
        pool_->start();
    }

    TaskManager::TaskManager(unsigned int workSize, unsigned int poolSize)
        : d_ptr(util::make_unique<TaskManagerPrivate>(workSize, poolSize))
    {
    }

    void TaskManager::addHandleTask(const std::string &protoName, const Task &task)
    {
        d_func()->handler_[protoName] = task;
    }

    void TaskManager::setPreHandleTask(const Task &task)
    {
        preTask_ = task;
    }

    void TaskManager::handleMessage(const PackagePtr &pacPtr, const SessionPtr &sessionPtr)
    {
        const std::string &name = pacPtr->typeName;
        LOG(INFO) << "task manager handle message, typename:" << name
            << ", msgtype:" << pacPtr->header.msgType
            << ", msgid:" << pacPtr->header.msgId
            << ", msgsize:" << pacPtr->header.msgSize << ", pacsize:" << pacPtr->header.pacSize;

        if (preTask_) {
            preTask_(sessionPtr, pacPtr);
        }

        std::map<std::string, Task>::iterator iter = d_func()->handler_.find(name);
        if (iter != d_func()->handler_.end()) {
            const Task &task = iter->second;
            if (pacPtr->header.isorder) {
                if (d_func()->workers_.empty()) {
                    LOG(ERROR) << "worker is empty!!!";
                } else {
                    int workId = sessionPtr->id() % d_func()->workers_.size();
                    d_func()->workers_[workId]->io.post([task, sessionPtr, pacPtr]() {
                        time_t start = util::currentMillisecond();
                        task(sessionPtr, pacPtr); 
                        LOG(INFO) << "handle order message finished, typename:" << pacPtr->typeName << ", msgid:" << pacPtr->header.msgId
                            << ", cost:" << (util::currentMillisecond() - start) << "ms";
                    });
                }
            } else {
                d_func()->pool_->run([task, sessionPtr, pacPtr]() {
                    time_t start = util::currentMillisecond();
                    task(sessionPtr, pacPtr);
                    LOG(INFO) << "handle pool message finished, typename:" << pacPtr->typeName << ", msgid:" << pacPtr->header.msgId
                        << ", cost:" << (util::currentMillisecond() - start) << "ms";
                });
            }
        } else if (!preTask_) {
            LOG(INFO) << "message discard:" << name;
        }
    }

    void TaskManager::destory()
    {
        d_func()->pool_->stop();
    }
}