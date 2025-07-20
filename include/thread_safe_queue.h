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

        ThreadSafeQueue() = default;

        // Delete copy constructor & copy assignment
        ThreadSafeQueue(const ThreadSafeQueue&) = delete;
        ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

        // Allow move constructor
        ThreadSafeQueue(ThreadSafeQueue&& other) noexcept {
            std::lock_guard<std::mutex> lock(other.mutex_);
            queue_ = std::move(other.queue_);
        }

        // Allow move assignment
        ThreadSafeQueue& operator=(ThreadSafeQueue&& other) noexcept {
            if (this != &other) {
                std::lock_guard<std::mutex> lock1(mutex_);
                std::lock_guard<std::mutex> lock2(other.mutex_);
                queue_ = std::move(other.queue_);
            }
            return *this;
        }

        void push(const T& value){

            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push_back(value);
            }
            cond_.notify_one();
        }
        // void push_with_timestamp(TradeEvent value){

        //     {
        //         std::lock_guard<std::mutex> lock(mutex_);
        //         value.received_time = std::chrono::steady_clock::now();
        //         queue_.push_back(value);
        //     }
        //     cond_.notify_one();
        // }

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