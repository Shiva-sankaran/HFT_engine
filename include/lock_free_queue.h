#pragma once
#include <atomic>
#include <vector>

template<typename T>
class LockFreeQueue {
public:
    explicit LockFreeQueue(size_t cap)
        : buffer(cap), head(0), tail(0), capacity(cap) {}

    bool enque(const T& val) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % capacity;
        if (next_tail == head.load(std::memory_order_acquire)) {
            return false; // queue full
        }
        buffer[current_tail] = val;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool deque(T& result) {
        size_t current_head = head.load(std::memory_order_relaxed);
        if (current_head == tail.load(std::memory_order_acquire)) {
            return false; // queue empty
        }
        result = buffer[current_head];
        head.store((current_head + 1) % capacity, std::memory_order_release);
        return true;
    }

private:
    std::vector<T> buffer;
    alignas(64) std::atomic<size_t> head;
    alignas(64) std::atomic<size_t> tail;
    size_t capacity;
};
