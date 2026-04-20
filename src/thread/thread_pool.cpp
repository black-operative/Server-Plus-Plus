#include <mutex>

#include "thread/thread_pool.hpp"

using std::mutex;
using std::stop_token;
using std::unique_lock;

Thread_Pool::Thread_Pool(size_t threads) {
    for (size_t i = 0; i < threads; i++) {
        workers.emplace_back(
            [this] (stop_token st){
                while (true) {
                    function<void()> task;

                    // Lock task queue and check if there's a task for execution
                    {
                        unique_lock<mutex> lock(this->q_mutex);

                        /* 
                         * Thread will wait until task appears or pool is closed 
                         * (stop_token will handle the later part)
                         */ 
                        bool success = this->condition.wait(
                            lock,
                            st,
                            [this] { 
                                return !this->tasks.empty(); 
                            }
                        );

                        if (!success) return;

                        // Grab task
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    // Run task 
                    task();
                }
            }
        );
    }
}

Thread_Pool::~Thread_Pool() {
    for (auto& worker : workers) {
        worker.request_stop();
    }
}