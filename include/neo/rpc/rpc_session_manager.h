/**
 * @file rpc_session_manager.h
 * @brief Lightweight session and iterator manager for RPC calls.
 */

#pragma once

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::rpc
{
/**
 * @brief Represents the traversal result for an iterator.
 */
struct IteratorResult
{
    std::vector<nlohmann::json> items;
    bool hasMore{false};
    bool found{false};
};

/**
 * @brief Singleton manager that tracks RPC sessions and lightweight iterators.
 *
 * The implementation is intentionally minimal. It stores iterator payloads as JSON
 * snapshots so that TraverseIterator can return data without depending on live VM
 * state. This mirrors the behaviour of the C# RpcSessionManager closely enough for
 * integration testing while the full iterator plumbing is completed.
 */
class RpcSessionManager
{
   public:
    /**
     * @brief Gets the singleton instance.
     * @return Reference to the global session manager.
     */
    static RpcSessionManager& Instance();

    /**
     * @brief Creates a new session identifier.
     * @return The new session id.
     */
    std::string CreateSession();

    /**
     * @brief Stores an iterator payload for the session.
     * @param sessionId The parent session id.
     * @param values The iterator contents expressed as JSON values.
     * @return Iterator id on success, std::nullopt if the session is unknown.
     */
    std::optional<std::string> StoreIterator(const std::string& sessionId, std::vector<nlohmann::json> values);

    /**
     * @brief Fetches up to @p maxItems entries from the iterator.
     * @param sessionId The session id.
     * @param iteratorId The iterator id.
     * @param maxItems Maximum number of items to return.
     * @return Iterator traversal result.
     */
    IteratorResult Traverse(const std::string& sessionId, const std::string& iteratorId, size_t maxItems);

    /**
     * @brief Terminates a session and all associated iterators.
     * @param sessionId The session id.
     * @return True if the session existed, false otherwise.
     */
    bool TerminateSession(const std::string& sessionId);

    /**
     * @brief Checks if a session exists.
     * @param sessionId The session id.
     * @return True if the session is tracked.
     */
    bool SessionExists(const std::string& sessionId) const;

    /**
     * @brief Adjusts the session timeout duration.
     */
    void SetSessionTimeout(std::chrono::steady_clock::duration duration);

    /**
     * @brief Sets the maximum iterator items that can be returned in one traversal.
     */
    void SetMaxIteratorItems(size_t max_items);

    /**
     * @brief Gets the maximum iterator items allowed per traversal.
     */
    size_t GetMaxIteratorItems() const { return max_iterator_items_; }

    /**
     * @brief Test-only helper retained for compatibility.
     */
    void SetSessionTimeoutForTests(std::chrono::steady_clock::duration duration) { SetSessionTimeout(duration); }

   private:
    static constexpr auto kSessionTimeout = std::chrono::minutes(5);

    struct IteratorState
    {
        std::vector<nlohmann::json> values;
        size_t index{0};
    };

    struct Session
    {
        std::unordered_map<std::string, IteratorState> iterators;
        std::chrono::steady_clock::time_point createdAt{std::chrono::steady_clock::now()};
        std::chrono::steady_clock::time_point lastAccess{createdAt};
    };

    RpcSessionManager() = default;

    std::string NextSessionId();
    std::string NextIteratorId(Session& session);
    void ExpireSessionsLocked();

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Session> sessions_;
    std::atomic<uint64_t> sessionCounter_{0};
    std::chrono::steady_clock::duration sessionTimeout_{kSessionTimeout};
    size_t max_iterator_items_{100};
};
}  // namespace neo::rpc
