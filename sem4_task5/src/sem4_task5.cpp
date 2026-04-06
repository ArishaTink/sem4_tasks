#include <iostream>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
using namespace std;

template <typename Key, typename Value>
class Cache {
private:
    std::map<Key, Value> data;
    std::mutex mtx;
    std::condition_variable cv;

public:
    inline void set(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mtx);

        bool isUpdate = (data.find(key) != data.end());
        data[key] = value;

        if (isUpdate) {
            std::cout << "Thread " << std::this_thread::get_id()
                      << " обновил ключ " << key
                      << " значением: " << value << std::endl;
        } else {
            std::cout << "Thread " << std::this_thread::get_id()
                      << " добавил ключ " << key
                      << " значением: " << value << std::endl;
        }

        cv.notify_all();
    }

    inline Value get(const Key& key) {
            std::unique_lock<std::mutex> lock(mtx);

            while (data.find(key) == data.end()) {
                std::cout << "Thread " << std::this_thread::get_id()
                          << " ожидает ключ " << key << "..." << std::endl;
                cv.wait(lock);
            }

            Value value = data[key];
            std::cout << "Thread " << std::this_thread::get_id()
                      << " получил ключ " << key
                      << " значение: " << value << std::endl;

            return value;
        }

    void printAll() {
            std::lock_guard<std::mutex> lock(mtx);
            std::cout << "Содержимое кэша (Thread " << std::this_thread::get_id() << ")" << std::endl;
            if (data.empty()) {
                std::cout << "(кэш пуст)" << std::endl;
            } else {
                for (const auto& pair : data) {
                    std::cout << "  " << pair.first << " - " << pair.second << std::endl;
                }
            }
        }

};

int main() {
	Cache<int, std::string> cache;
	std::vector<std::thread> threads;

	threads.emplace_back([&cache]() {
		std::this_thread::yield();
		cache.get(1);
	});

	threads.emplace_back([&cache]() {
		cache.get(2);
	});

	threads.emplace_back([&cache]() {
		std::this_thread::yield();
		cache.get(3);
	});

	threads.emplace_back([&cache]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		cache.set(1, "Первое значение ключа 1");
	});

	threads.emplace_back([&cache]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(600));
		cache.set(2, "Значение ключа 2");
	});

	threads.emplace_back([&cache]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(900));
		cache.set(1, "Обновлённое значение ключа 1");
	});

	threads.emplace_back([&cache]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(1200));
		cache.set(3, "Значение ключа 3");
	});

	for (auto& t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	std::cout << "\nФинальное состояние кэша:" << std::endl;
	cache.printAll();
	return 0;
}
