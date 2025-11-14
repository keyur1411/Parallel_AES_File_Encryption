#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

// Define our chunk type
using Chunk = std::vector<unsigned char>;

class ThreadSafeQueue {
public:
    // Copy the constructor and public methods from your original code
    ThreadSafeQueue(size_t max_size);
    void push(Chunk&& chunk, const std::atomic<bool>& error_flag);
    bool pop(Chunk& chunk, const std::atomic<bool>& error_flag);
    void finish();

private:
    // Copy all the private member variables
    std::queue<Chunk> queue_;
    std::mutex mtx_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
    size_t max_size_;
    std::atomic<bool> done_; // Note: Your original 'done_' was atomic, keep it that way!
};