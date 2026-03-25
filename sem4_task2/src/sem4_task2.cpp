#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>

using namespace std;

template <typename T>
class Account {
private:
    T balance;
    std::mutex mtx;

public:
    Account(T bal) : balance(bal) {}

    T get_balance() {
        std::lock_guard<std::mutex> lock(mtx);
        return balance;
    }

    void deposit(T amount) {
        std::lock_guard<std::mutex> lock(mtx);
        balance += amount;
    }

    void withdraw(T amount) {
        std::lock_guard<std::mutex> lock(mtx);
        balance -= amount;
    }
};

template <typename T>
class Bank {
private:
    std::vector<std::unique_ptr<Account<T>>> accounts;
    std::mutex mtx;
    std::condition_variable cv;

public:
    explicit Bank(std::vector<std::unique_ptr<Account<T>>> acc)
        : accounts(std::move(acc)) {}

    void transfer(size_t from, size_t to, T amount) {
        if (from >= accounts.size() || to >= accounts.size()) {
            cout << "Invalid argument" << endl;
            return;
        }

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&, this]() {
                return accounts[from]->get_balance() >= amount;
            });
        }

        accounts[from]->withdraw(amount);
        accounts[to]->deposit(amount);

        cv.notify_all();
    }

    T get_balance(size_t index) const {
        if (index >= accounts.size()) {
            cout << "Invalid index" << endl;
            return T{};
        }
        return accounts[index]->get_balance();
    }
};

int main() {
    std::vector<std::unique_ptr<Account<double>>> accounts;
    accounts.reserve(3);
    accounts.emplace_back(std::make_unique<Account<double>>(30000.0));
    accounts.emplace_back(std::make_unique<Account<double>>(20000.0));
    accounts.emplace_back(std::make_unique<Account<double>>(45000.0));

    Bank<double> bank(std::move(accounts));

    std::thread t1([&bank]() {
        double amount = 2000.0;
        bank.transfer(0, 1, amount);
    });

    std::thread t2([&bank]() {
        double amount = 5000.0;
        bank.transfer(2, 1, amount);
    });

    std::thread t3([&bank]() {
        double amount = 30000.0;
        bank.transfer(0, 2, amount);
    });

    std::thread t4([&bank]() {
        double amount = 6000.0;
        bank.transfer(1, 0, amount);
    });

    t1.detach();
    t2.detach();
    t3.detach();
    t4.detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    cout << "Account 0 balance: " << bank.get_balance(0) << endl;
    cout << "Account 1 balance: " << bank.get_balance(1) << endl;
    cout << "Account 2 balance: " << bank.get_balance(2) << endl;

    return 0;
}
