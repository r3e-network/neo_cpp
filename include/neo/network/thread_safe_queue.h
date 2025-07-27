#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace neo::network
{
/**
 * @brief Thread-safe queue for messages.
 */
template <typename T>
class ThreadSafeQueue
{
  public:
    /**
     * @brief Pushes an item to the queue.
     * @param item The item to push.
     */
    void Push(T item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
        condition_.notify_one();
    }

    /**
     * @brief Pops an item from the queue.
     * @param item The item that was popped.
     * @return True if an item was popped, false if the queue was empty.
     */
    bool TryPop(T& item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty())
            return false;
        item = queue_.front();
        queue_.pop();
        return true;
    }

    /**
     * @brief Pops an item from the queue, waiting if necessary.
     * @param item The item that was popped.
     */
    void WaitAndPop(T& item)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty(); });
        item = queue_.front();
        queue_.pop();
    }

    /**
     * @brief Checks if the queue is empty.
     * @return True if the queue is empty, false otherwise.
     */
    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Gets the size of the queue.
     * @return The size of the queue.
     */
    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

  private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable condition_;
};
}  // namespace neo::network
