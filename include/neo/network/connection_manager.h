#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace neo::network
{

/**
 * @brief Connection pool for managing network connections
 */
template <typename Connection>
class ConnectionPool
{
   public:
    struct Config
    {
        size_t minConnections = 5;
        size_t maxConnections = 50;
        std::chrono::milliseconds connectionTimeout = std::chrono::milliseconds(30000);
        std::chrono::milliseconds idleTimeout = std::chrono::milliseconds(300000);
        std::chrono::milliseconds validationInterval = std::chrono::milliseconds(60000);
        size_t maxRetries = 3;
    };

    using ConnectionFactory = std::function<std::shared_ptr<Connection>()>;
    using ConnectionValidator = std::function<bool(const std::shared_ptr<Connection>&)>;

    ConnectionPool(const Config& config, ConnectionFactory factory, ConnectionValidator validator)
        : config_(config), factory_(factory), validator_(validator), running_(true)
    {
        // Pre-create minimum connections
        for (size_t i = 0; i < config_.minConnections; ++i)
        {
            try
            {
                auto conn = factory_();
                if (conn && validator_(conn))
                {
                    available_.push(conn);
                }
            }
            catch (const std::exception&)
            {
                // Log but continue
            }
        }

        // Start maintenance thread
        maintenanceThread_ = std::thread(&ConnectionPool::MaintenanceLoop, this);
    }

    ~ConnectionPool() { Shutdown(); }

    /**
     * @brief Acquire a connection from the pool
     * @return Connection or nullptr if timeout
     */
    std::shared_ptr<Connection> Acquire()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        auto deadline = std::chrono::steady_clock::now() + config_.connectionTimeout;

        while (running_)
        {
            // Try to get an available connection
            if (!available_.empty())
            {
                auto conn = available_.front();
                available_.pop();

                // Validate connection before returning
                if (validator_(conn))
                {
                    active_[conn.get()] = std::chrono::steady_clock::now();
                    return conn;
                }
                // Invalid connection, continue to create new one
            }

            // Check if we can create a new connection
            if (available_.size() + active_.size() < config_.maxConnections)
            {
                lock.unlock();

                try
                {
                    auto conn = factory_();
                    if (conn && validator_(conn))
                    {
                        lock.lock();
                        active_[conn.get()] = std::chrono::steady_clock::now();
                        return conn;
                    }
                }
                catch (const std::exception&)
                {
                    // Failed to create connection
                }

                lock.lock();
            }

            // Wait for a connection to become available
            if (cv_.wait_until(lock, deadline) == std::cv_status::timeout)
            {
                return nullptr;  // Timeout
            }
        }

        return nullptr;
    }

    /**
     * @brief Release a connection back to the pool
     * @param conn Connection to release
     */
    void Release(std::shared_ptr<Connection> conn)
    {
        if (!conn) return;

        std::lock_guard<std::mutex> lock(mutex_);

        auto it = active_.find(conn.get());
        if (it != active_.end())
        {
            active_.erase(it);

            // Check if connection is still valid
            if (validator_(conn))
            {
                available_.push(conn);
                cv_.notify_one();
            }
            // Otherwise let it be destroyed
        }
    }

    /**
     * @brief Get current pool statistics
     */
    struct Stats
    {
        size_t totalConnections;
        size_t activeConnections;
        size_t availableConnections;
        size_t failedConnections;
    };

    Stats GetStats() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return {available_.size() + active_.size(), active_.size(), available_.size(), failedConnections_};
    }

    /**
     * @brief Shutdown the connection pool
     */
    void Shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
            cv_.notify_all();
        }

        if (maintenanceThread_.joinable())
        {
            maintenanceThread_.join();
        }

        // Clear all connections
        std::lock_guard<std::mutex> lock(mutex_);
        while (!available_.empty())
        {
            available_.pop();
        }
        active_.clear();
    }

   private:
    void MaintenanceLoop()
    {
        while (running_)
        {
            std::this_thread::sleep_for(config_.validationInterval);

            std::lock_guard<std::mutex> lock(mutex_);

            // Remove idle connections
            auto now = std::chrono::steady_clock::now();
            size_t removed = 0;

            std::queue<std::shared_ptr<Connection>> validated;
            while (!available_.empty())
            {
                auto conn = available_.front();
                available_.pop();

                if (validator_(conn))
                {
                    validated.push(conn);
                }
                else
                {
                    removed++;
                }
            }

            available_ = std::move(validated);

            // Ensure minimum connections
            size_t currentSize = available_.size() + active_.size();
            while (currentSize < config_.minConnections && running_)
            {
                try
                {
                    auto conn = factory_();
                    if (conn && validator_(conn))
                    {
                        available_.push(conn);
                        currentSize++;
                    }
                }
                catch (const std::exception&)
                {
                    break;
                }
            }
        }
    }

    Config config_;
    ConnectionFactory factory_;
    ConnectionValidator validator_;

    std::queue<std::shared_ptr<Connection>> available_;
    std::unordered_map<Connection*, std::chrono::steady_clock::time_point> active_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_;
    std::atomic<size_t> failedConnections_{0};
    std::thread maintenanceThread_;
};

/**
 * @brief Connection timeout manager
 */
class TimeoutManager
{
   public:
    using TimeoutCallback = std::function<void()>;

    /**
     * @brief Schedule a timeout
     * @param duration Timeout duration
     * @param callback Callback to execute on timeout
     * @return Timeout ID for cancellation
     */
    uint64_t Schedule(std::chrono::milliseconds duration, TimeoutCallback callback)
    {
        auto deadline = std::chrono::steady_clock::now() + duration;

        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t id = nextId_++;

        timeouts_.emplace(id, TimeoutEntry{deadline, callback});
        cv_.notify_one();

        return id;
    }

    /**
     * @brief Cancel a scheduled timeout
     * @param id Timeout ID
     */
    void Cancel(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        timeouts_.erase(id);
    }

    /**
     * @brief Start the timeout manager
     */
    void Start()
    {
        if (running_.exchange(true)) return;

        workerThread_ = std::thread(&TimeoutManager::WorkerLoop, this);
    }

    /**
     * @brief Stop the timeout manager
     */
    void Stop()
    {
        if (!running_.exchange(false)) return;

        cv_.notify_all();

        if (workerThread_.joinable())
        {
            workerThread_.join();
        }
    }

    ~TimeoutManager() { Stop(); }

   private:
    struct TimeoutEntry
    {
        std::chrono::steady_clock::time_point deadline;
        TimeoutCallback callback;
    };

    void WorkerLoop()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        while (running_)
        {
            if (timeouts_.empty())
            {
                cv_.wait(lock);
                continue;
            }

            // Find next timeout
            auto now = std::chrono::steady_clock::now();
            std::vector<uint64_t> expired;

            for (const auto& [id, entry] : timeouts_)
            {
                if (entry.deadline <= now)
                {
                    expired.push_back(id);
                }
            }

            // Execute expired timeouts
            for (uint64_t id : expired)
            {
                auto it = timeouts_.find(id);
                if (it != timeouts_.end())
                {
                    auto callback = it->second.callback;
                    timeouts_.erase(it);

                    // Execute callback without holding lock
                    lock.unlock();
                    callback();
                    lock.lock();
                }
            }

            // Wait until next timeout or notification
            if (!timeouts_.empty())
            {
                auto nextDeadline =
                    std::min_element(timeouts_.begin(), timeouts_.end(),
                                     [](const auto& a, const auto& b) { return a.second.deadline < b.second.deadline; })
                        ->second.deadline;

                cv_.wait_until(lock, nextDeadline);
            }
        }
    }

    std::unordered_map<uint64_t, TimeoutEntry> timeouts_;
    uint64_t nextId_ = 1;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::thread workerThread_;
};

/**
 * @brief Connection limits manager
 */
class ConnectionLimits
{
   public:
    struct Config
    {
        size_t maxConnectionsPerIP = 5;
        size_t maxTotalConnections = 1000;
        std::chrono::milliseconds connectionRateWindow = std::chrono::milliseconds(60000);
        size_t maxConnectionsPerWindow = 100;
    };

    explicit ConnectionLimits(const Config& config) : config_(config) {}
    explicit ConnectionLimits() : config_(Config{}) {}

    /**
     * @brief Check if a new connection is allowed
     * @param clientIP Client IP address
     * @return true if allowed
     */
    bool IsConnectionAllowed(const std::string& clientIP)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::steady_clock::now();

        // Clean old connection records
        CleanOldRecords(now);

        // Check total connections
        if (GetTotalConnections() >= config_.maxTotalConnections)
        {
            return false;
        }

        // Check per-IP limit
        if (ipConnections_[clientIP] >= config_.maxConnectionsPerIP)
        {
            return false;
        }

        // Check connection rate
        connectionTimes_.push_back(now);
        if (connectionTimes_.size() > config_.maxConnectionsPerWindow)
        {
            return false;
        }

        return true;
    }

    /**
     * @brief Register a new connection
     * @param clientIP Client IP address
     */
    void RegisterConnection(const std::string& clientIP)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ipConnections_[clientIP]++;
        totalConnections_++;
    }

    /**
     * @brief Unregister a connection
     * @param clientIP Client IP address
     */
    void UnregisterConnection(const std::string& clientIP)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = ipConnections_.find(clientIP);
        if (it != ipConnections_.end())
        {
            if (--it->second == 0)
            {
                ipConnections_.erase(it);
            }
        }

        if (totalConnections_ > 0)
        {
            totalConnections_--;
        }
    }

    /**
     * @brief Get current connection count for an IP
     * @param clientIP Client IP address
     * @return Connection count
     */
    size_t GetConnectionCount(const std::string& clientIP) const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = ipConnections_.find(clientIP);
        return it != ipConnections_.end() ? it->second : 0;
    }

    /**
     * @brief Get total connection count
     * @return Total connections
     */
    size_t GetTotalConnections() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return totalConnections_;
    }

   private:
    void CleanOldRecords(const std::chrono::steady_clock::time_point& now)
    {
        auto cutoff = now - config_.connectionRateWindow;

        connectionTimes_.erase(std::remove_if(connectionTimes_.begin(), connectionTimes_.end(),
                                              [cutoff](const auto& time) { return time < cutoff; }),
                               connectionTimes_.end());
    }

    Config config_;
    std::unordered_map<std::string, size_t> ipConnections_;
    std::vector<std::chrono::steady_clock::time_point> connectionTimes_;
    size_t totalConnections_ = 0;

    mutable std::mutex mutex_;
};

}  // namespace neo::network