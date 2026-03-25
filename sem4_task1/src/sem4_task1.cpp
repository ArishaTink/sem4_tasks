#include <iostream>
#include <thread>
#include <vector>
#include <numeric>
#include <mutex>
#include <condition_variable>
using namespace std;


struct Pair {
	std::thread::id thread_id;
	int sum;
};

template <typename T>
class ParallelSum {

private:
	std::vector<T> data;
	size_t n_threads;
	vector<Pair> result;
	std::condition_variable cv;
	std::mutex mtx;
	std::mutex resultMtx;

	inline int sumSegment(const std::vector<T>& vec, size_t start, size_t end) const {
		if (end > vec.size()) end = vec.size();
		return std::accumulate(vec.begin() + start, vec.begin() + end, 0);
	}

public:

	ParallelSum(std::vector<T> d, size_t nt)
	        : data(std::move(d)), n_threads(nt) {}

	void compute_sum() {
		if (n_threads == 0 || data.empty()) return;

		size_t total = data.size();
	    size_t base = total / n_threads;
	    size_t remainder = total % n_threads;

	    for (size_t i = 0; i < n_threads; ++i) {

	    	size_t start = i * base + std::min(i, remainder);
	        size_t end   = start + base + (i < remainder ? 1 : 0);

	        std::thread([&, i, start, end, this]() {
	        int seg_sum = sumSegment(data, start, end);

	        {
	        	std::lock_guard<std::mutex> lock(resultMtx);
	            result.push_back({std::this_thread::get_id(), seg_sum});
	        }

	        cv.notify_one();
	        }).detach();

	        }

	        {
	        	std::unique_lock<std::mutex> lock(mtx);
	            cv.wait(lock, [this]() {
	            	return result.size() >= n_threads;
	            });
	        }


	        int finalSum = 0;
	        for (const auto& p : result) {
	            cout << "Thread id: " << p.thread_id
	                 << ", segment sum: " << p.sum << endl;
	            finalSum += p.sum;
	        }
	        cout << "Final sum: " << finalSum << endl;
	    }


};

int main() {
	vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	size_t n_threads;
	cout << "Enter the number of threads: ";
	cin >> n_threads;

	ParallelSum<int> ps(numbers, n_threads);
	ps.compute_sum();

	return 0;
}
