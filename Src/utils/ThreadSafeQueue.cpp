#include "utils/ThreadSafeQueue.hpp"
#include <iostream>

// constructor implementation
ThreadSafeQueue::ThreadSafeQueue(size_t max_size)
    : max_size_(max_size), done_(false) {}

/**
 * @brief Pushes a new chunk onto the queue. Blocks if the queue is full.
 **/
void ThreadSafeQueue::push(Chunk &&chunk, const std::atomic<bool> &error_flag)
{
  std::unique_lock<std::mutex> lock(mtx_);
  // Wait until the queue is not full, or we're done/errored
  cv_not_full_.wait(lock, [this, &error_flag]
                    { return queue_.size() < max_size_ || done_ || error_flag; });
  if (done_ || error_flag)
    return; // Don't push if we're stopping
  queue_.push(std::move(chunk));
  lock.unlock();
  cv_not_empty_.notify_one(); // Signal to a waiting consumer
}

/**
 * @brief Pops a chunk from the queue. Blocks if the queue is empty.
 * @return false if the queue is empty AND done/errored, true otherwise.
 */

bool ThreadSafeQueue::pop(Chunk &chunk, const std::atomic<bool> &error_flag)
{
  std::unique_lock<std::mutex> lock(mtx_);
  // Wait until the queue is not empty, or we're done/errored
  cv_not_empty_.wait(lock, [this, &error_flag]
                     { return !queue_.empty() || done_ || error_flag; });

  // Check for stop conditions
  if (queue_.empty() && (done_ || error_flag))
  {
    return false;
  }

  chunk = std::move(queue_.front());
  queue_.pop();
  lock.unlock();
  cv_not_full_.notify_one(); // Signal to a waiting producer
  return true;
}

/**
 * @brief Signals that no more items will be pushed.
 */
void ThreadSafeQueue::finish()
{
  std::unique_lock<std::mutex> lock(mtx_);
  done_ = true;
  lock.unlock();
  // Wake up all waiting threads
  cv_not_empty_.notify_all();
  cv_not_full_.notify_all();
}