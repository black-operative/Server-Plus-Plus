#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>

using std::mutex;
using std::queue;
using std::vector;
using std::jthread;
using std::function;
using std::unique_lock;
using std::condition_variable_any;

class Thread_Pool {
    private:
        vector<jthread>         workers;
        queue<function<void()>> tasks;
        mutex                   q_mutex;
        condition_variable_any  condition;

    public:
        explicit Thread_Pool(size_t threads);

        ~Thread_Pool();

        template<class F>
        void enqueue(F&& f) {
            {
                unique_lock<mutex> lock(q_mutex);
                tasks.emplace(std::forward<F>(f));
            }
            
            condition.notify_one();
        }
};