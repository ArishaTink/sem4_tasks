#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
struct QueueItem {
    T value;
    int priority;

    bool operator<(const QueueItem& other) const {
        return priority < other.priority;
    }
};

template<typename T>
class PriorityQueue {
private:
    std::priority_queue<QueueItem<T>> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;

public:
    void push(T value, int priority) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push({value, priority});
            std::cout << "Push by thread " << std::this_thread::get_id()
                      << " value = " << value << ", priority = " << priority << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cv_.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock, [this]() { return !queue_.empty(); });

        QueueItem<T> item = queue_.top();
        queue_.pop();

        std::cout << "Pop by thread " << std::this_thread::get_id()
                  << " value = " << item.value << " priority = " << item.priority << std::endl;

        return item.value;
    }
};

int main() {
    PriorityQueue<int> pq;

    auto producer = [&pq](int id) {
        for (int i = 0; i < 5; ++i) {
            pq.push(id * 10 + i, rand() % 100);
            std::this_thread::yield();
        }
    };

    auto consumer = [&pq]() {
        for (int i = 0; i < 5; ++i) {
            pq.pop();
            std::this_thread::yield();
        }
    };

    std::vector<std::thread> threads;

    for (int i = 0; i < 2; ++i) {
        threads.emplace_back(producer, i);
    }

    for (int i = 0; i < 2; ++i) {
        threads.emplace_back(consumer);
    }

    for (auto& t : threads) {
        t.detach();
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
