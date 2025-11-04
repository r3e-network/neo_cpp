#include <gtest/gtest.h>
#include <neo/rpc/rpc_server.h>

TEST(RpcServerMetrics, DefaultCountersPresent)
{
    neo::rpc::RpcConfig config;
    config.enable_rate_limiting = true;
    config.max_requests_per_second = 10;

    neo::rpc::RpcServer server(config);

    auto stats = server.GetStatistics();
    ASSERT_TRUE(stats.IsObject());

    auto totalRequests = stats.GetValue("totalRequests");
    auto failedRequests = stats.GetValue("failedRequests");
    auto rateLimited = stats.GetValue("rateLimitedRequests");
    auto authFailures = stats.GetValue("authFailures");
    auto corsFailures = stats.GetValue("corsFailures");
    auto uptime = stats.GetValue("uptime");

    EXPECT_TRUE(totalRequests.IsNumber());
    EXPECT_TRUE(failedRequests.IsNumber());
    EXPECT_TRUE(rateLimited.IsNumber());
    EXPECT_TRUE(authFailures.IsNumber());
    EXPECT_TRUE(corsFailures.IsNumber());
    EXPECT_TRUE(uptime.IsNumber());

    EXPECT_EQ(totalRequests.GetUInt64(), 0u);
    EXPECT_EQ(failedRequests.GetUInt64(), 0u);
    EXPECT_EQ(rateLimited.GetUInt64(), 0u);
    EXPECT_EQ(authFailures.GetUInt64(), 0u);
    EXPECT_EQ(corsFailures.GetUInt64(), 0u);
}
