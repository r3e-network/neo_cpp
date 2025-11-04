/**
 * @file rpc_session_manager.cpp
 * @brief Lightweight session and iterator manager for RPC calls.
 */

#include <neo/rpc/rpc_session_manager.h>

#include <sstream>

namespace neo::rpc
{
namespace
{
constexpr size_t kDefaultIteratorBatch = 100;
}

RpcSessionManager& RpcSessionManager::Instance()
{
    static RpcSessionManager instance;
    return instance;
}

std::string RpcSessionManager::CreateSession()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExpireSessionsLocked();
    auto sessionId = NextSessionId();
    sessions_.try_emplace(sessionId, Session{});
    return sessionId;
}

std::optional<std::string> RpcSessionManager::StoreIterator(const std::string& sessionId,
                                                            std::vector<nlohmann::json> values)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExpireSessionsLocked();
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return std::nullopt;

    auto iteratorId = NextIteratorId(it->second);
    it->second.iterators.emplace(iteratorId, IteratorState{std::move(values), 0});
    it->second.lastAccess = std::chrono::steady_clock::now();
    return iteratorId;
}

IteratorResult RpcSessionManager::Traverse(const std::string& sessionId, const std::string& iteratorId,
                                           size_t maxItems)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExpireSessionsLocked();
    IteratorResult result;

    auto sessionIt = sessions_.find(sessionId);
    if (sessionIt == sessions_.end()) return result;

    auto iteratorIt = sessionIt->second.iterators.find(iteratorId);
    if (iteratorIt == sessionIt->second.iterators.end()) return result;

    result.found = true;

    sessionIt->second.lastAccess = std::chrono::steady_clock::now();

    if (maxItems == 0) maxItems = kDefaultIteratorBatch;

    auto& state = iteratorIt->second;
    const auto remaining = state.values.size() - std::min(state.index, state.values.size());
    const auto take = std::min(maxItems, remaining);

    for (size_t i = 0; i < take; ++i)
    {
        result.items.push_back(state.values[state.index++]);
    }

    result.hasMore = state.index < state.values.size();
    if (!result.hasMore)
    {
        // Remove exhausted iterator to match C# behaviour.
        sessionIt->second.iterators.erase(iteratorIt);
    }

    return result;
}

bool RpcSessionManager::TerminateSession(const std::string& sessionId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExpireSessionsLocked();
    return sessions_.erase(sessionId) > 0;
}

bool RpcSessionManager::SessionExists(const std::string& sessionId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const_cast<RpcSessionManager*>(this)->ExpireSessionsLocked();
    return sessions_.find(sessionId) != sessions_.end();
}

std::string RpcSessionManager::NextSessionId()
{
    std::ostringstream stream;
    stream << "session-" << ++sessionCounter_;
    return stream.str();
}

std::string RpcSessionManager::NextIteratorId(Session& session)
{
    std::ostringstream stream;
    stream << "iterator-" << session.iterators.size() + 1;
    return stream.str();
}

void RpcSessionManager::ExpireSessionsLocked()
{
    const auto now = std::chrono::steady_clock::now();
    for (auto it = sessions_.begin(); it != sessions_.end();)
    {
        if (now - it->second.lastAccess > sessionTimeout_)
        {
            it = sessions_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void RpcSessionManager::SetSessionTimeoutForTests(std::chrono::steady_clock::duration duration)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessionTimeout_ = duration > std::chrono::steady_clock::duration::zero() ? duration : std::chrono::steady_clock::duration::zero();
    ExpireSessionsLocked();
}
}  // namespace neo::rpc
