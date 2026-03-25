#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
using namespace std;

class Logger {

private:
	std::ofstream log_file;
	std::mutex mtx;

public:

	explicit Logger(const std::string& filename) {
		log_file.open(filename, std::ios::out | std::ios::trunc);
		std::cerr << "Не удалось открыть файл лога!" << std::endl;
	}

	~Logger() {
		if (log_file.is_open()) {
			log_file.close();
		}
	}

	template<typename T>
	void log(const T& message) {
		std::lock_guard<std::mutex> lock(mtx);

		auto write_message = [this, &message]() {
			cout << "Thread: " << std::this_thread::get_id() << ", " << message << endl;

			if(log_file.is_open()) {
				log_file << "Thread: " << std::this_thread::get_id() << ", " << message << endl;
				log_file.flush();
			}
		};

		write_message();
	}
};

int main() {
	Logger logger("log.txt");

	auto create_log_thread = [&](int thread_id){
		return std::thread([&, thread_id](){
			for (int i = 0; i < 10; ++i) {
				if (i % 3 == 0) {
					logger.log("ляляляля");
				} else if (i % 3 == 1) {
					logger.log("сообщение какое-то");
				} else {
					logger.log("Еще одно сообщение");
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		});
	};

	std::vector<std::thread> threads;

	for (int i = 0; i < 4; ++i) {
		threads.emplace_back(create_log_thread(i));
	}

	for (auto& t : threads) {
		t.detach();
	}

	std::this_thread::sleep_for(std::chrono::seconds(3));
	cout << "Все потоки завершили выполнение" << endl;
	return 0;
}
