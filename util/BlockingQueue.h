#pragma once
#include <assert.h>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>

template <typename T>
class BlockingQueue 
{
public:
    BlockingQueue()
        : mutex_()
        , notEmpty_()
        , queue_()
    {
    }

    void put(const T& x)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(x);
        notEmpty_.notify_all();
    }

    void put(T&& x)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(std::move(x));
        notEmpty_.notify_all();
    }

    int take(T *out)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // always use a while-loop, due to spurious wakeup
        while (queue_.empty()) {
            notEmpty_.wait(lock);
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        *out = front;
        return 1;
    }

    int take(T *out, int ms)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // always use a while-loop, due to spurious wakeup
        while (queue_.empty()) {
            auto ret = notEmpty_.wait_for(lock, std::chrono::milliseconds(ms));
            if (ret == std::cv_status::timeout)
                return 0;
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        *out = front;
        return 1;
    }

    size_t size() const
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::deque<T> queue_;
};


