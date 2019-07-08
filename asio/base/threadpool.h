#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>
#include <algorithm>
#include <queue>
#include <boost/thread.hpp>

class ThreadPool
{
public:
    typedef boost::function<void()> Task;

    ThreadPool(int num)
        : num_(num)
        , maxQueueSize_(0)
        , running_(false)
    {
    }

    ~ThreadPool()
    {
        if (running_) {
            stop();
        }
    }

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
            threads_.push_back(boost::thread(boost::bind(&ThreadPool::threadFunc, this)));
        }
    }

    void stop()
    {
        {
            boost::unique_lock<boost::mutex> ul(mutex_);
            running_ = false;
            notEmpty_.notify_all();
        }

        for (std::vector<boost::thread>::iterator iter = threads_.begin(); iter != threads_.end(); ++iter) {
            if (iter->joinable()) {
                iter->join();
            }
        }
    }

    void run(const Task &t)
    {
        if (threads_.empty()) {
            t();
        } else {
            boost::unique_lock<boost::mutex> ul(mutex_);
            while (isFull()) {
                notFull_.wait(ul);
            }
            assert(!isFull());
            queue_.push_back(t);
            notEmpty_.notify_one();
        }
    }

private:
    ThreadPool(const ThreadPool&);
    void operator=(const ThreadPool&);

    bool isFull() const
    {
        return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
    }

    void threadFunc()
    {
        printf("create thread id:0x%x\n", boost::this_thread::get_id());
        while (running_) {
            Task task(take());
            if (task) {
                task();
            }
        }
        printf("end thread id:0x%x\n", boost::this_thread::get_id());
    }

    Task take()
    {
        boost::unique_lock<boost::mutex> ul(mutex_);
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
    boost::mutex mutex_;
    boost::condition_variable notEmpty_;
    boost::condition_variable notFull_;
    std::vector<boost::thread> threads_;
    std::deque<Task> queue_;
    size_t maxQueueSize_;
    bool running_;
};