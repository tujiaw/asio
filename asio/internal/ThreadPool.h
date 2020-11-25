#pragma once

#include <stdlib.h>
#include <cstdio>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>
#include <algorithm>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace ningto
{
    class ThreadPool
    {
    public:
        typedef std::function<void()> Task;

        explicit ThreadPool(int num) : num_(num) { }
        ThreadPool(const ThreadPool&) = delete;
        void operator=(const ThreadPool&) = delete;

        void setMaxQueueSize(int maxSize)
        {
            maxQueueSize_ = maxSize;
        }

        void start()
        {
            assert(threads_.empty());
            running_ = true;
            threads_.reserve(num_);
            for (int i = 0; i < num_; i++) {
                threads_.push_back(std::thread(std::bind(&ThreadPool::threadFunc, this)));
            }
        }

        void stop()
        {
            {
                std::unique_lock<std::mutex> ul(mutex_);
                running_ = false;
                notEmpty_.notify_all();
                notFull_.notify_all();
            }

            for (auto &t : threads_) {
                if (t.joinable()) {
                    t.join();
                }
            }
        }

        void run(const Task &t)
        {
            if (threads_.empty()) {
                t();
            } else {
                std::unique_lock<std::mutex> ul(mutex_);
                while (isFull() && running_) {
                    notFull_.wait(ul);
                }
                if (!running_) {
                    return;
                }
                assert(!isFull());
                queue_.push_back(std::move(t));
                notEmpty_.notify_one();
            }
        }

        bool isFull() const
        {
            return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
        }

        void threadFunc()
        {
            try {
                while (running_) {
                    Task task(take());
                    if (task) {
                        task();
                    }
                }
            } catch (const std::exception &err) {
                fprintf(stderr, "exception caught in ThreadPool reason: %s\n", err.what());
                abort();
            } catch (...) {
                fprintf(stderr, "unknown exception caught in ThreadPool");
                throw;
            }
        }

        Task take()
        {
            std::unique_lock<std::mutex> ul(mutex_);
            while (queue_.empty() && running_) {
                notEmpty_.wait(ul);
            }
            Task task;
            if (!queue_.empty()) {
                task = queue_.front();
                queue_.pop_front();
                if (maxQueueSize_ > 0) {
                    notFull_.notify_one();
                }
            }
            return task;
        }

    private:
        int num_;
        std::mutex mutex_;
        std::condition_variable notEmpty_;
        std::condition_variable notFull_;
        std::vector<std::thread> threads_;
        std::deque<Task> queue_;
        size_t maxQueueSize_ = 0;
        bool running_ = false;
    };
}