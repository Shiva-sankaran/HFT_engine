#pragma once
#include<deque>
#include "trade_event.h"
#include<mutex>
#include<condition_variable>


template <typename T>
class ThreadSafeQueue {
    private:
        std::deque<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable cond_;

    public:

        void push(const T& value){

            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push_back(value);
            }
            cond_.notify_one();
        }

        T pop_blocking(){
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this]() {return !queue_.empty();});
            T front = std::move(queue_.front());
            queue_.pop_front();
            return front;

        }

        bool empty () const{
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size() == 0;
        }
};