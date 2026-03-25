#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
using namespace std;

template <typename T>
class Buffer {
private:
	std::vector<T> buffer;
	size_t capacity;
	std::mutex mtx;
	std::condition_variable cv;

public:
	explicit Buffer(size_t cap) : capacity(cap) {
	        buffer.reserve(cap);
	    }

	void produce(T value) {
		while(true) {
			{
				std::unique_lock<std::mutex> lock(mtx);
				if (buffer.size() < capacity) {
					buffer.push_back(std::move(value));
					cout << "Добавлен элемент " << value << " потоком " << this_thread::get_id() << endl;
					cv.notify_one();
					return;
				}
			}
			std::this_thread::yield();
		}
	}

	T consume() {
		while (true) {
			{
				std::unique_lock<std::mutex> lock(mtx);
				if (!buffer.empty()) {
					T value = std::move(buffer.front());
					buffer.erase(buffer.begin());
					cout << "Извлечен элемент " << value << " потоком " << std::this_thread::get_id() << endl;
					cv.notify_one();
					return value;
				}
			}
			std::this_thread::yield();
		}
	}
};

int main() {
	const size_t capasity = 5;
	Buffer<int> buffer(capasity);

	auto create_producer = [&](int producer_id) {
		return std::thread([&, producer_id]() {
			for (int i = 0; i < 5; ++i) {
				buffer.produce(producer_id * 10 + i);
			}
		});
	};

	auto create_consumer = [&](int consumer_id) {
	    return std::thread([&, consumer_id]() {
	        for (int i = 0; i < 5; ++i) {
	            buffer.consume();
	        }
	    });
	};

	std::vector<std::thread> threads;

	for (int i = 0; i < 3; ++i) {
		threads.push_back(create_producer(i));
	}

	for (int i = 0; i < 3; ++i) {
		threads.push_back(create_consumer(i));
	}

	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	return 0;
}
