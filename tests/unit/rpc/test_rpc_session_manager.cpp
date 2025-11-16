#include <chrono>
#include <gtest/gtest.h>
#include <neo/rpc/rpc_session_manager.h>

#include <nlohmann/json.hpp>

#include <vector>

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

TEST(RpcSessionManagerTest, RespectsMaxIteratorLimit)
{
    auto& manager = RpcSessionManager::Instance();
    manager.SetMaxIteratorItems(1);

    const auto sessionId = manager.CreateSession();
    ASSERT_TRUE(manager.SessionExists(sessionId));

    std::vector<nlohmann::json> payload = {nlohmann::json{{"value", 1}}, nlohmann::json{{"value", 2}}};
    auto iteratorId = manager.StoreIterator(sessionId, payload);
    ASSERT_TRUE(iteratorId.has_value());

    auto limited = manager.Traverse(sessionId, *iteratorId, /*maxItems*/ 5);
    EXPECT_TRUE(limited.found);
    ASSERT_EQ(limited.items.size(), 1u);
    EXPECT_TRUE(limited.hasMore);

    auto rest = manager.Traverse(sessionId, *iteratorId, 5);
    EXPECT_TRUE(rest.found);
    EXPECT_EQ(rest.items.size(), 1u);
    EXPECT_FALSE(rest.hasMore);

    manager.SetMaxIteratorItems(100);
    manager.TerminateSession(sessionId);
}
