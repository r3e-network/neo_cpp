/**
 * @file memory_pool.h
 * @brief Transaction memory pool management
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace neo::core
{
/**
 * @brief Thread-safe object pool for efficient memory allocation and reuse
 * @tparam T The type of objects to pool
 */
template <typename T>
class MemoryPool
{
   public:
    /**
     * @brief Constructs a memory pool with initial capacity
     * @param initial_size Initial number of objects to pre-allocate
     * @param max_size Maximum number of objects to keep in pool
     */
    explicit MemoryPool(size_t initial_size = 1024, size_t max_size = 10240)
        : max_size_(max_size), allocated_count_(0), reused_count_(0)
    {
        pool_.reserve(initial_size);
        for (size_t i = 0; i < initial_size; ++i)
        {
            pool_.emplace_back(std::make_unique<T>());
        }
    }

    /**
     * @brief Acquires an object from the pool or creates a new one
     * @tparam Args Arguments for constructing the object
     * @param args Arguments to forward to the object constructor
     * @return Unique pointer to the object
     */
    template <typename... Args>
    std::unique_ptr<T> acquire(Args&&... args)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!pool_.empty())
        {
            auto obj = std::move(pool_.back());
            pool_.pop_back();

            // Reset the object with new values
            *obj = T(std::forward<Args>(args)...);
            reused_count_++;
            return obj;
        }

        // Pool is empty, create new object
        allocated_count_++;
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief Returns an object to the pool for reuse
     * @param obj The object to return
     */
    void release(std::unique_ptr<T> obj)
    {
        if (!obj) return;

        std::lock_guard<std::mutex> lock(mutex_);

        // Only keep objects if we haven't reached max size
        if (pool_.size() < max_size_)
        {
            pool_.push_back(std::move(obj));
        }
        // Otherwise, let the object be destroyed
    }

    /**
     * @brief Gets the current number of objects in the pool
     * @return Number of pooled objects
     */
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.size();
    }

    /**
     * @brief Gets performance statistics
     * @return Tuple of (allocated_count, reused_count, current_pool_size)
     */
    std::tuple<size_t, size_t, size_t> get_stats() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return {allocated_count_.load(), reused_count_.load(), pool_.size()};
    }

    /**
     * @brief Clears the pool, releasing all cached objects
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.clear();
    }

   private:
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<T>> pool_;
    size_t max_size_;
    std::atomic<size_t> allocated_count_;
    std::atomic<size_t> reused_count_;
};

/**
 * @brief Singleton manager for global memory pools
 */
class MemoryPoolManager
{
   public:
    static MemoryPoolManager& instance()
    {
        static MemoryPoolManager instance;
        return instance;
    }

    // Specialized pools for frequently allocated objects
    MemoryPool<std::vector<uint8_t>>& byte_vector_pool() { return byte_vector_pool_; }

    /**
     * @brief Reports memory pool statistics
     */
    void report_statistics() const;

   private:
    MemoryPoolManager() = default;
    ~MemoryPoolManager() = default;
    MemoryPoolManager(const MemoryPoolManager&) = delete;
    MemoryPoolManager& operator=(const MemoryPoolManager&) = delete;

    // Pools for different object types
    MemoryPool<std::vector<uint8_t>> byte_vector_pool_{1024, 10240};
};

/**
 * @brief RAII wrapper for pooled objects
 * @tparam T The type of pooled object
 */
template <typename T>
class PooledObject
{
   public:
    explicit PooledObject(std::unique_ptr<T> obj, MemoryPool<T>* pool) : obj_(std::move(obj)), pool_(pool) {}

    ~PooledObject()
    {
        if (obj_ && pool_)
        {
            pool_->release(std::move(obj_));
        }
    }

    // Move-only semantics
    PooledObject(PooledObject&& other) noexcept : obj_(std::move(other.obj_)), pool_(other.pool_)
    {
        other.pool_ = nullptr;
    }

    PooledObject& operator=(PooledObject&& other) noexcept
    {
        if (this != &other)
        {
            if (obj_ && pool_)
            {
                pool_->release(std::move(obj_));
            }
            obj_ = std::move(other.obj_);
            pool_ = other.pool_;
            other.pool_ = nullptr;
        }
        return *this;
    }

    // Deleted copy operations
    PooledObject(const PooledObject&) = delete;
    PooledObject& operator=(const PooledObject&) = delete;

    T* operator->() { return obj_.get(); }
    const T* operator->() const { return obj_.get(); }
    T& operator*() { return *obj_; }
    const T& operator*() const { return *obj_; }
    T* get() { return obj_.get(); }
    const T* get() const { return obj_.get(); }

   private:
    std::unique_ptr<T> obj_;
    MemoryPool<T>* pool_;
};

}  // namespace neo::core