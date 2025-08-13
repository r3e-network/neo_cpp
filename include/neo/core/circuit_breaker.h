/**
 * @file circuit_breaker.h
 * @brief Circuit Breaker
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>

namespace neo::core
{

/**
 * @brief Circuit breaker pattern implementation for fault tolerance
 *
 * States:
 * - CLOSED: Normal operation, requests pass through
 * - OPEN: Failures exceeded threshold, requests fail fast
 * - HALF_OPEN: Testing if service recovered
 */
class CircuitBreaker
{
   public:
    enum class State
    {
        CLOSED,
        OPEN,
        HALF_OPEN
    };

    struct Config
    {
        size_t failureThreshold = 5;                                           // Failures before opening
        double failureRateThreshold = 0.5;                                     // Failure rate to open (50%)
        size_t requestVolumeThreshold = 10;                                    // Minimum requests for rate calculation
        std::chrono::milliseconds timeout = std::chrono::milliseconds(60000);  // Open state timeout
        size_t successThreshold = 3;                                           // Successes in half-open to close
        std::chrono::milliseconds windowSize = std::chrono::milliseconds(60000);  // Rolling window
    };

    using OnStateChangeCallback = std::function<void(State, State)>;

    explicit CircuitBreaker(const std::string& name, const Config& config)
        : name_(name), config_(config), state_(State::CLOSED)
    {
    }

    explicit CircuitBreaker(const std::string& name) : name_(name), config_(Config{}), state_(State::CLOSED) {}

    /**
     * @brief Execute a function through the circuit breaker
     * @param func Function to execute
     * @return Function result
     * @throws std::runtime_error if circuit is open
     */
    template <typename Func>
    auto Execute(Func&& func) -> decltype(func())
    {
        // Check if we should allow the request
        if (!AllowRequest())
        {
            throw std::runtime_error("Circuit breaker '" + name_ + "' is OPEN");
        }

        auto startTime = std::chrono::steady_clock::now();

        try
        {
            auto result = func();
            RecordSuccess(startTime);
            return result;
        }
        catch (const std::exception&)
        {
            RecordFailure(startTime);
            throw;
        }
    }

    /**
     * @brief Execute a function with fallback
     * @param func Function to execute
     * @param fallback Fallback function if circuit is open
     * @return Function result or fallback result
     */
    template <typename Func, typename Fallback>
    auto ExecuteWithFallback(Func&& func, Fallback&& fallback) -> decltype(func())
    {
        try
        {
            return Execute(std::forward<Func>(func));
        }
        catch (const std::runtime_error& e)
        {
            // Circuit is open, use fallback
            return fallback();
        }
    }

    /**
     * @brief Get current circuit breaker state
     * @return Current state
     */
    State GetState() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    /**
     * @brief Get circuit breaker statistics
     */
    struct Stats
    {
        size_t totalRequests;
        size_t successCount;
        size_t failureCount;
        double failureRate;
        State currentState;
        std::chrono::system_clock::time_point lastStateChange;
    };

    Stats GetStats() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t total = metrics_.successCount + metrics_.failureCount;
        double rate = total > 0 ? static_cast<double>(metrics_.failureCount) / total : 0.0;

        return {total, metrics_.successCount, metrics_.failureCount, rate, state_, lastStateChange_};
    }

    /**
     * @brief Reset the circuit breaker
     */
    void Reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        State oldState = state_;
        state_ = State::CLOSED;
        metrics_.successCount.store(0);
        metrics_.failureCount.store(0);
        metrics_.totalResponseTime.store(0);
        metrics_.maxResponseTime.store(0);
        consecutiveSuccesses_ = 0;
        lastFailureTime_ = std::chrono::steady_clock::time_point{};
        lastStateChange_ = std::chrono::system_clock::now();

        if (onStateChange_ && oldState != state_)
        {
            onStateChange_(oldState, state_);
        }
    }

    /**
     * @brief Set state change callback
     * @param callback Callback function
     */
    void SetOnStateChange(OnStateChangeCallback callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        onStateChange_ = callback;
    }

    /**
     * @brief Force the circuit breaker to open state
     */
    void Trip()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        TransitionTo(State::OPEN);
    }

   private:
    bool AllowRequest()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        switch (state_)
        {
            case State::CLOSED:
                return true;

            case State::OPEN:
                // Check if timeout has passed
                if (std::chrono::steady_clock::now() - lastFailureTime_ >= config_.timeout)
                {
                    TransitionTo(State::HALF_OPEN);
                    return true;
                }
                return false;

            case State::HALF_OPEN:
                // Allow limited requests in half-open state
                return true;
        }

        return false;
    }

    void RecordSuccess(const std::chrono::steady_clock::time_point& startTime)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Update metrics
        metrics_.successCount++;
        UpdateResponseTime(startTime);

        // Handle state transitions
        switch (state_)
        {
            case State::HALF_OPEN:
                consecutiveSuccesses_++;
                if (consecutiveSuccesses_ >= config_.successThreshold)
                {
                    TransitionTo(State::CLOSED);
                }
                break;

            case State::CLOSED:
                // Clean old metrics periodically
                CleanOldMetrics();
                break;

            case State::OPEN:
                // Shouldn't happen
                break;
        }
    }

    void RecordFailure(const std::chrono::steady_clock::time_point& startTime)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Update metrics
        metrics_.failureCount++;
        lastFailureTime_ = std::chrono::steady_clock::now();
        UpdateResponseTime(startTime);

        // Handle state transitions
        switch (state_)
        {
            case State::CLOSED:
                if (ShouldTrip())
                {
                    TransitionTo(State::OPEN);
                }
                break;

            case State::HALF_OPEN:
                // Any failure in half-open state reopens the circuit
                TransitionTo(State::OPEN);
                break;

            case State::OPEN:
                // Already open
                break;
        }
    }

    bool ShouldTrip()
    {
        // Check failure threshold
        if (metrics_.failureCount >= config_.failureThreshold)
        {
            return true;
        }

        // Check failure rate
        size_t totalRequests = metrics_.successCount + metrics_.failureCount;
        if (totalRequests >= config_.requestVolumeThreshold)
        {
            double failureRate = static_cast<double>(metrics_.failureCount) / totalRequests;
            if (failureRate >= config_.failureRateThreshold)
            {
                return true;
            }
        }

        return false;
    }

    void TransitionTo(State newState)
    {
        if (state_ == newState) return;

        State oldState = state_;
        state_ = newState;
        lastStateChange_ = std::chrono::system_clock::now();

        // Reset state-specific counters
        switch (newState)
        {
            case State::CLOSED:
                metrics_.successCount.store(0);
                metrics_.failureCount.store(0);
                metrics_.totalResponseTime.store(0);
                metrics_.maxResponseTime.store(0);
                consecutiveSuccesses_ = 0;
                break;

            case State::HALF_OPEN:
                consecutiveSuccesses_ = 0;
                break;

            case State::OPEN:
                // Keep metrics for analysis
                break;
        }

        // Notify callback
        if (onStateChange_)
        {
            onStateChange_(oldState, newState);
        }
    }

    void UpdateResponseTime(const std::chrono::steady_clock::time_point& startTime)
    {
        auto duration = std::chrono::steady_clock::now() - startTime;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        metrics_.totalResponseTime += ms;
        if (ms > metrics_.maxResponseTime)
        {
            metrics_.maxResponseTime = ms;
        }
    }

    void CleanOldMetrics()
    {
        // Reset metrics periodically based on window size
        auto now = std::chrono::steady_clock::now();
        if (now - lastMetricsReset_ > config_.windowSize)
        {
            // Reset counters while preserving max response time
            auto prevMax = metrics_.maxResponseTime.load();

            metrics_.successCount.store(0);
            metrics_.failureCount.store(0);
            metrics_.totalResponseTime.store(0);
            metrics_.maxResponseTime.store(prevMax);
            lastMetricsReset_ = now;
        }
    }

    struct Metrics
    {
        std::atomic<size_t> successCount{0};
        std::atomic<size_t> failureCount{0};
        std::atomic<int64_t> totalResponseTime{0};
        std::atomic<int64_t> maxResponseTime{0};
    };

    const std::string name_;
    const Config config_;

    State state_;
    Metrics metrics_;
    size_t consecutiveSuccesses_ = 0;
    std::chrono::steady_clock::time_point lastFailureTime_;
    std::chrono::steady_clock::time_point lastMetricsReset_;
    std::chrono::system_clock::time_point lastStateChange_;

    OnStateChangeCallback onStateChange_;
    mutable std::mutex mutex_;
};

/**
 * @brief Circuit breaker manager for multiple services
 */
class CircuitBreakerManager
{
   public:
    static CircuitBreakerManager& GetInstance()
    {
        static CircuitBreakerManager instance;
        return instance;
    }

    /**
     * @brief Get or create a circuit breaker
     * @param name Circuit breaker name
     * @param config Configuration (used only on creation)
     * @return Circuit breaker reference
     */
    CircuitBreaker& GetCircuitBreaker(const std::string& name, const CircuitBreaker::Config& config = {})
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = breakers_.find(name);
        if (it == breakers_.end())
        {
            auto result = breakers_.emplace(name, std::make_unique<CircuitBreaker>(name, config));
            return *result.first->second;
        }

        return *it->second;
    }

    /**
     * @brief Reset all circuit breakers
     */
    void ResetAll()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& [name, breaker] : breakers_)
        {
            breaker->Reset();
        }
    }

    /**
     * @brief Get all circuit breaker names
     * @return Vector of names
     */
    std::vector<std::string> GetAllNames() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::string> names;
        for (const auto& [name, _] : breakers_)
        {
            names.push_back(name);
        }

        return names;
    }

   private:
    CircuitBreakerManager() = default;

    std::unordered_map<std::string, std::unique_ptr<CircuitBreaker>> breakers_;
    mutable std::mutex mutex_;
};

}  // namespace neo::core