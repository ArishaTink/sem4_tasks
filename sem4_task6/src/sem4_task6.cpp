#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <algorithm>
#include <iomanip>

template <typename T>
class MatrixProcessor {
private:
    std::vector<std::vector<T>>& matrix;  // ссылка на матрицу (обрабатываем in-place)
    size_t rows;
    size_t cols;
    int num_threads;

    std::mutex mtx;                       // защита общих ресурсов
    std::condition_variable cv;           // синхронизация через condition_variable
    int completed_threads;                // счётчик завершённых потоков (общий ресурс)
    size_t stats_processed;               // счётчик статистики (обработано элементов)

public:
    MatrixProcessor(std::vector<std::vector<T>>& mat, int threads)
        : matrix(mat),
          rows(mat.size()),
          cols(rows > 0 ? mat[0].size() : 0),
          num_threads(threads),
          completed_threads(0),
          stats_processed(0) {}
    void apply(std::function<void(T&)> func) {
    	if (rows == 0 || cols == 0 || num_threads <= 0) {
            std::cout << "Нечего обрабатывать или неверное число потоков" << std::endl;
            return;
        }

        size_t total_elements = rows * cols;

        {
            std::lock_guard<std::mutex> lock(mtx);
            completed_threads = 0;
            stats_processed = 0;
        }

        size_t chunk_size = (total_elements + static_cast<size_t>(num_threads) - 1) / static_cast<size_t>(num_threads);

        for (int i = 0; i < num_threads; ++i) {
            size_t start = static_cast<size_t>(i) * chunk_size;
            size_t end = std::min(start + chunk_size, total_elements);

            std::thread worker([this, func, start, end]() {
                std::cout << "Thread " << std::this_thread::get_id()
                          << " начал обработку сегмента [" << start << " - " << end << ")" << std::endl;

                size_t local_processed = 0;
                for (size_t idx = start; idx < end; ++idx) {
                    size_t r = idx / cols;
                    size_t c = idx % cols;
                    func(matrix[r][c]);
                    ++local_processed;

                    if (local_processed % 20 == 0) {
                        std::this_thread::yield();
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(mtx);
                    stats_processed += local_processed;
                    ++completed_threads;

                    std::cout << "Thread " << std::this_thread::get_id()
                              << " завершил обработку сегмента [" << start << " - " << end << ")"
                              << " (обработано локально: " << local_processed << ")" << std::endl;

                    if (completed_threads == num_threads) {
                        cv.notify_all();
                    }
                }
            });

            worker.detach();
        }

        {
            std::unique_lock<std::mutex> lock(mtx);
            while (completed_threads < num_threads) {
                cv.wait(lock);
            }
        }

        std::cout << "Все " << num_threads << " потоков завершили обработку матрицы" << std::endl;
    }

    size_t getProcessedCount() {
        std::lock_guard<std::mutex> lock(mtx);
        return stats_processed;
    }
};

int main() {
    std::vector<std::vector<double>> matrix(6, std::vector<double>(6, 0.0));
    for (size_t i = 0; i < 6; ++i) {
        for (size_t j = 0; j < 6; ++j) {
            matrix[i][j] = static_cast<double>(i * 6 + j + 1);
        }
    }

    auto print_matrix = [&](const std::string& title) {
        std::cout << title << " (Thread " << std::this_thread::get_id() << "):" << std::endl;
        for (const auto& row : matrix) {
            for (double val : row) {
                std::cout << val << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    };

    print_matrix("Исходная матрица");

    MatrixProcessor<double> processor(matrix, 4);

    std::cout << "Возводим параллельно каждый элемент в квадрат" << std::endl;
    processor.apply([](double& val) {
        val = val * val;
    });

    print_matrix("\nМатрица после параллельной обработки");

    std::cout << "Всего обработано элементов: " << processor.getProcessedCount() << std::endl;

    return 0;
}
