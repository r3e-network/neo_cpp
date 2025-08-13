/**
 * @file metrics.h
 * @brief Performance metrics collection
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace neo::metrics
{
/**
 * @brief Simple metrics collection for Neo node
 */
class Metrics
{
   public:
    static Metrics& GetInstance()
    {
        static Metrics instance;
        return instance;
    }

    // Transaction metrics
    void IncrementTransactionsProcessed() { transactionsProcessed_++; }
    void IncrementTransactionsVerified() { transactionsVerified_++; }
    void IncrementTransactionsFailed() { transactionsFailed_++; }

    // Block metrics
    void IncrementBlocksProcessed() { blocksProcessed_++; }
    void IncrementBlocksVerified() { blocksVerified_++; }
    void IncrementBlocksFailed() { blocksFailed_++; }

    // Network metrics
    void IncrementPeersConnected() { peersConnected_++; }
    void IncrementPeersDisconnected() { peersDisconnected_++; }
    void IncrementMessagesReceived() { messagesReceived_++; }
    void IncrementMessagesSent() { messagesSent_++; }

    // Getters
    uint64_t GetTransactionsProcessed() const { return transactionsProcessed_; }
    uint64_t GetTransactionsVerified() const { return transactionsVerified_; }
    uint64_t GetTransactionsFailed() const { return transactionsFailed_; }
    uint64_t GetBlocksProcessed() const { return blocksProcessed_; }
    uint64_t GetBlocksVerified() const { return blocksVerified_; }
    uint64_t GetBlocksFailed() const { return blocksFailed_; }
    uint64_t GetPeersConnected() const { return peersConnected_; }
    uint64_t GetPeersDisconnected() const { return peersDisconnected_; }
    uint64_t GetMessagesReceived() const { return messagesReceived_; }
    uint64_t GetMessagesSent() const { return messagesSent_; }

   private:
    std::atomic<uint64_t> transactionsProcessed_{0};
    std::atomic<uint64_t> transactionsVerified_{0};
    std::atomic<uint64_t> transactionsFailed_{0};
    std::atomic<uint64_t> blocksProcessed_{0};
    std::atomic<uint64_t> blocksVerified_{0};
    std::atomic<uint64_t> blocksFailed_{0};
    std::atomic<uint64_t> peersConnected_{0};
    std::atomic<uint64_t> peersDisconnected_{0};
    std::atomic<uint64_t> messagesReceived_{0};
    std::atomic<uint64_t> messagesSent_{0};
};

/**
 * @brief A simple counter metric.
 */
class Counter
{
   public:
    Counter() : value_(0) {}

    void Increment(int64_t delta = 1) { value_.fetch_add(delta, std::memory_order_relaxed); }

    int64_t Get() const { return value_.load(std::memory_order_relaxed); }

    void Put(int64_t value) { value_.store(value, std::memory_order_relaxed); }

   private:
    std::atomic<int64_t> value_;
};

/**
 * @brief A simple histogram metric.
 */
class Histogram
{
   public:
    Histogram() = default;

    void Observe(double value)
    {
        // Simple implementation - just track observations
        observations_.push_back({value, std::chrono::steady_clock::now()});
    }

    size_t Count() const { return observations_.size(); }

   private:
    struct Observation
    {
        double value;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::vector<Observation> observations_;
};
}  // namespace neo::metrics