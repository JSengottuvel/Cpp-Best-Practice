/*
 * JSengottuvel
 * Copyright (c) 2025 www.jayakumar.de
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <vector>
#include <chrono>

// Mutex to protect std::cout from interleaved output
std::mutex cout_mutex;

class SimpleThreadPool {
public:
    SimpleThreadPool(size_t num_workers) : stop_(false) {
        // emplace_back constructs a std::thread in-place in the vector.
        // The lambda [this] { this->Worker(); } is passed to the std::thread constructor,
        // which starts the thread and runs the Worker() method.
        for (size_t i = 0; i < num_workers; ++i) {
            workers_.emplace_back([this] { this->Worker(); });
        }
    }

    ~SimpleThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true; // Signal workers to stop
        }
        cv_.notify_all(); // Wake up all workers so they can exit
        for (auto& t : workers_) t.join(); // Wait for all threads to finish        
    }

    void Enqueue(const std::string& msg) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.push(msg); // Add a new task to the queue
        }
        cv_.notify_one(); // Notify one waiting worker
    }

private:
    void Worker() {
        while (true) {
            std::string task;
            {
                // Lock the queue mutex to safely access the shared queue
                std::unique_lock<std::mutex> lock(queue_mutex_);
                // Wait until there is a task or stop_ is true
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                // If stopping and no tasks remain, exit the thread
                if (stop_ && tasks_.empty()) return;
                // Get the next task
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            // Protect std::cout with a separate mutex to prevent interleaved output
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Thread " << std::this_thread::get_id() << ": " << task << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
        }
    }

    std::vector<std::thread> workers_;      // Holds the worker threads
    std::queue<std::string> tasks_;         // Shared task queue
    std::mutex queue_mutex_;                // Protects access to the queue
    std::condition_variable cv_;            // Used to notify workers of new tasks
    bool stop_;                             // Signals workers to stop
};

int main() {

    {    
        std::cout << "Starting the thread pool..." << std::endl;
        SimpleThreadPool pool(2); // Create a pool with 2 worker threads

        // Enqueue some messages
        for (int i = 1; i <= 5; ++i) {
            pool.Enqueue("Message " + std::to_string(i));
        }
    }
    // Destructor will stop workers gracefully
    return 0;
}
