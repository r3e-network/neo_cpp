#include <gtest/gtest.h>
#include <chrono>
#include <neo/rpc/rpc_session_manager.h>

#include <nlohmann/json.hpp>

using neo::rpc::RpcSessionManager;

TEST(RpcSessionManagerTest, StoresAndTraversesIterators)
{
    auto& manager = RpcSessionManager::Instance();

    const auto sessionId = manager.CreateSession();
    ASSERT_FALSE(sessionId.empty());
    ASSERT_TRUE(manager.SessionExists(sessionId));

    std::vector<nlohmann::json> payload = {nlohmann::json{{"index", 0}}, nlohmann::json{{"index", 1}}};
    auto iteratorId = manager.StoreIterator(sessionId, payload);
    ASSERT_TRUE(iteratorId.has_value());

    auto result = manager.Traverse(sessionId, *iteratorId, payload.size());
    ASSERT_TRUE(result.found);
    ASSERT_EQ(result.items.size(), payload.size());
    EXPECT_FALSE(result.hasMore);
    EXPECT_EQ(result.items[0]["index"], 0);
    EXPECT_EQ(result.items[1]["index"], 1);

    // Iterator should be removed after exhaustion.
    auto second = manager.Traverse(sessionId, *iteratorId, payload.size());
    EXPECT_FALSE(second.found);

    EXPECT_TRUE(manager.TerminateSession(sessionId));
}

TEST(RpcSessionManagerTest, SessionTimeoutExpiresEntries)
{
    auto& manager = RpcSessionManager::Instance();
    const auto defaultTimeout = std::chrono::minutes(5);
    manager.SetSessionTimeoutForTests(defaultTimeout);

    const auto sessionId = manager.CreateSession();
    ASSERT_TRUE(manager.SessionExists(sessionId));

    manager.SetSessionTimeoutForTests(std::chrono::milliseconds(0));
    EXPECT_FALSE(manager.SessionExists(sessionId));

    manager.SetSessionTimeoutForTests(defaultTimeout);
}
