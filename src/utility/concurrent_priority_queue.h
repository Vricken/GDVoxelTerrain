#ifndef CONCURRENT_PRIORITY_QUEUE_H
#define CONCURRENT_PRIORITY_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

namespace godot {

template <typename T, typename Compare = std::less<T>>
class ConcurrentPriorityQueue {
public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            q.push(std::move(value));
        }
        cv.notify_one();
    }

    bool try_pop(T& result) {
        std::lock_guard<std::mutex> lock(mtx);
        if (q.empty()) return false;
        result = std::move(q.top());
        q.pop();
        return true;
    }

    T wait_and_pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]{ return !q.empty(); });
        T value = std::move(q.top());
        q.pop();
        return value;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return q.empty();
    }

private:
    mutable std::mutex mtx;
    std::priority_queue<T, std::vector<T>, Compare> q;
    std::condition_variable cv;
};
}

#endif // CONCURRENT_PRIORITY_QUEUE_H