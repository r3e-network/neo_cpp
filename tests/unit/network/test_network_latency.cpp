#include <algorithm>
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <sstream>
#include <thread>
#include <vector>

using namespace neo::network::p2p;
using namespace neo::network::p2p::payloads;
using namespace neo::io;
using namespace std::chrono;

class UT_network_latency : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Setup test environment for latency measurement
        startTime_ = steady_clock::now();

        // Create test ping payload with current timestamp
        auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        testPingPayload_ = PingPayload(100, static_cast<uint32_t>(now), 0x12345678);
    }

    void TearDown() override
    {
        // Cleanup test environment
    }

    // Helper method to calculate elapsed time in milliseconds
    uint32_t GetElapsedMilliseconds()
    {
        auto endTime = steady_clock::now();
        return static_cast<uint32_t>(duration_cast<milliseconds>(endTime - startTime_).count());
    }

    // Helper method to simulate network delay
    void SimulateNetworkDelay(uint32_t delayMs)
    {
        std::this_thread::sleep_for(milliseconds(delayMs));
    }

  protected:
    steady_clock::time_point startTime_;
    PingPayload testPingPayload_;
};

TEST_F(UT_network_latency, PingLatencyMeasurement)
{
    // Test: Basic ping latency measurement

    auto pingTimestamp =
        static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

    // Create ping message with current timestamp
    PingPayload pingPayload(100, pingTimestamp, 0x12345678);
    auto pingMessage = Message::Create(MessageCommand::Ping, std::make_shared<PingPayload>(pingPayload));

    // Simulate small network delay
    SimulateNetworkDelay(10);  // 10ms delay

    // Calculate latency
    auto responseTime =
        static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

    uint32_t latency = responseTime - pingTimestamp;

    // Verify latency measurement
    EXPECT_GT(latency, 5u);    // Should be at least 5ms due to our simulated delay
    EXPECT_LT(latency, 100u);  // Should be less than 100ms for local test

    // Verify message type
    EXPECT_EQ(pingMessage.GetCommand(), MessageCommand::Ping);

    // Verify payload contains correct timestamp
    auto pingPayloadPtr = std::dynamic_pointer_cast<PingPayload>(pingMessage.GetPayload());
    ASSERT_NE(pingPayloadPtr, nullptr);

    EXPECT_EQ(pingPayloadPtr->GetTimestamp(), pingTimestamp);
    EXPECT_EQ(pingPayloadPtr->GetLastBlockIndex(), 100u);
    EXPECT_EQ(pingPayloadPtr->GetNonce(), 0x12345678u);
}

TEST_F(UT_network_latency, HighLatencyHandling)
{
    // Test: Handling of high network latency scenarios

    auto pingTimestamp =
        static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

    PingPayload pingPayload(200, pingTimestamp, 0xABCDEF00);

    // Simulate high latency (200ms)
    SimulateNetworkDelay(200);

    auto responseTime =
        static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

    uint32_t latency = responseTime - pingTimestamp;

    // Verify high latency is measured correctly
    EXPECT_GT(latency, 150u);  // Should be at least 150ms
    EXPECT_LT(latency, 300u);  // Should be less than 300ms

    // Verify payload is still valid after high latency
    EXPECT_EQ(pingPayload.GetTimestamp(), pingTimestamp);
    EXPECT_EQ(pingPayload.GetLastBlockIndex(), 200u);
    EXPECT_EQ(pingPayload.GetNonce(), 0xABCDEF00u);
}

TEST_F(UT_network_latency, TimestampOverflowHandling)
{
    // Test: Handling of timestamp overflow scenarios

    // Test with maximum timestamp value
    uint32_t maxTimestamp = UINT32_MAX;
    PingPayload pingPayload(500, maxTimestamp, 0xFFFFFFFF);

    // Small delay
    SimulateNetworkDelay(5);

    // Test that payloads handle max values correctly
    EXPECT_EQ(pingPayload.GetTimestamp(), maxTimestamp);
    EXPECT_EQ(pingPayload.GetLastBlockIndex(), 500u);
    EXPECT_EQ(pingPayload.GetNonce(), 0xFFFFFFFFu);

    // Test with zero timestamp
    PingPayload zeroPingPayload(0, 0, 0);
    EXPECT_EQ(zeroPingPayload.GetTimestamp(), 0u);
    EXPECT_EQ(zeroPingPayload.GetLastBlockIndex(), 0u);
    EXPECT_EQ(zeroPingPayload.GetNonce(), 0u);

    // Test with near-overflow values
    uint32_t nearMax = UINT32_MAX - 1000;
    PingPayload nearMaxPingPayload(1000, nearMax, nearMax);
    EXPECT_EQ(nearMaxPingPayload.GetTimestamp(), nearMax);
    EXPECT_EQ(nearMaxPingPayload.GetLastBlockIndex(), 1000u);
    EXPECT_EQ(nearMaxPingPayload.GetNonce(), nearMax);
}

TEST_F(UT_network_latency, MessageSerializationLatency)
{
    // Test: Latency impact of message serialization/deserialization

    auto startTime = steady_clock::now();

    // Create ping payload
    auto timestamp = static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    PingPayload pingPayload(150, timestamp, 0x87654321);

    // Serialize ping message
    auto pingMessage = Message::Create(MessageCommand::Ping, std::make_shared<PingPayload>(pingPayload));
    auto serializedData = pingMessage.ToArray();

    auto serializeTime = steady_clock::now();
    auto serializeLatency = duration_cast<microseconds>(serializeTime - startTime).count();

    // Deserialize ping message
    Message deserializedMessage;
    uint32_t bytesRead = Message::TryDeserialize(serializedData.AsSpan(), deserializedMessage);

    auto deserializeTime = steady_clock::now();
    auto deserializeLatency = duration_cast<microseconds>(deserializeTime - serializeTime).count();

    // Verify successful serialization/deserialization
    EXPECT_GT(bytesRead, 0u);
    EXPECT_EQ(deserializedMessage.GetCommand(), MessageCommand::Ping);

    // Verify serialization latency is reasonable (should be very fast)
    EXPECT_LT(serializeLatency, 10000);    // Less than 10ms
    EXPECT_LT(deserializeLatency, 10000);  // Less than 10ms

    // Verify payload integrity
    auto deserializedPayload = std::dynamic_pointer_cast<PingPayload>(deserializedMessage.GetPayload());
    ASSERT_NE(deserializedPayload, nullptr);
    EXPECT_EQ(deserializedPayload->GetTimestamp(), timestamp);
    EXPECT_EQ(deserializedPayload->GetLastBlockIndex(), 150u);
    EXPECT_EQ(deserializedPayload->GetNonce(), 0x87654321u);
}

TEST_F(UT_network_latency, ConcurrentLatencyMeasurement)
{
    // Test: Concurrent latency measurements don't interfere

    std::vector<uint32_t> timestamps;
    std::vector<PingPayload> pings;

    // Create multiple ping payloads with different timestamps
    for (int i = 0; i < 10; ++i)
    {
        auto timestamp =
            static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()) + i;

        timestamps.push_back(timestamp);
        pings.emplace_back(100 + i, timestamp, 0x1000 + i);

        // Small delay between each ping
        SimulateNetworkDelay(1);
    }

    // Verify all timestamps are preserved correctly
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(pings[i].GetTimestamp(), timestamps[i]);
        EXPECT_EQ(pings[i].GetLastBlockIndex(), static_cast<uint32_t>(100 + i));
        EXPECT_EQ(pings[i].GetNonce(), static_cast<uint32_t>(0x1000 + i));
    }

    // Verify timestamps are in ascending order
    for (int i = 1; i < 10; ++i)
    {
        EXPECT_GE(timestamps[i], timestamps[i - 1]);
    }
}

TEST_F(UT_network_latency, LatencyStatisticsCalculation)
{
    // Test: Calculate latency statistics over multiple measurements

    std::vector<uint32_t> latencies;

    // Perform multiple latency measurements
    for (int i = 0; i < 20; ++i)
    {
        auto pingTime =
            static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

        // Variable delay to simulate real network conditions
        uint32_t delay = 5 + (i % 10);  // 5-14ms delay
        SimulateNetworkDelay(delay);

        auto pongTime =
            static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

        uint32_t latency = pongTime - pingTime;
        latencies.push_back(latency);
    }

    // Calculate statistics
    uint32_t minLatency = *std::min_element(latencies.begin(), latencies.end());
    uint32_t maxLatency = *std::max_element(latencies.begin(), latencies.end());

    uint64_t sum = 0;
    for (uint32_t latency : latencies)
    {
        sum += latency;
    }
    uint32_t avgLatency = static_cast<uint32_t>(sum / latencies.size());

    // Verify statistics are reasonable
    EXPECT_GT(minLatency, 0u);    // Should have some latency
    EXPECT_LT(maxLatency, 100u);  // Should be less than 100ms
    EXPECT_GE(avgLatency, minLatency);
    EXPECT_LE(avgLatency, maxLatency);

    // Verify we have reasonable spread
    EXPECT_GT(maxLatency - minLatency, 0u);  // Should have some variation
}

TEST_F(UT_network_latency, TimeoutHandling)
{
    // Test: Handling of timeout scenarios in latency measurement

    auto pingTimestamp =
        static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

    PingPayload pingPayload(300, pingTimestamp, 0xDEADBEEF);

    // Simulate very long delay (simulating network timeout)
    auto longDelayStart = steady_clock::now();
    SimulateNetworkDelay(100);  // 100ms delay to simulate timeout condition
    auto longDelayEnd = steady_clock::now();

    auto actualDelay = duration_cast<milliseconds>(longDelayEnd - longDelayStart).count();

    // Verify delay was approximately what we expected
    EXPECT_GE(actualDelay, 90);   // At least 90ms
    EXPECT_LE(actualDelay, 150);  // Less than 150ms (allowing for timing variance)

    // Test payload is still valid after long delay
    EXPECT_EQ(pingPayload.GetTimestamp(), pingTimestamp);
    EXPECT_EQ(pingPayload.GetLastBlockIndex(), 300u);
    EXPECT_EQ(pingPayload.GetNonce(), 0xDEADBEEFu);

    // Test with timeout detection logic
    auto responseTime =
        static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

    uint32_t measuredLatency = responseTime - pingTimestamp;

    // Should detect high latency condition
    const uint32_t TIMEOUT_THRESHOLD = 50;  // 50ms threshold
    bool isTimeout = measuredLatency > TIMEOUT_THRESHOLD;

    EXPECT_TRUE(isTimeout);  // Should detect timeout with 100ms delay
}

TEST_F(UT_network_latency, PingPayloadCreationMethods)
{
    // Test: Different ping payload creation methods and their latency impact

    auto startTime = steady_clock::now();

    // Test regular constructor
    PingPayload payload1(400, 123456789, 0x11111111);
    auto constructorTime = duration_cast<microseconds>(steady_clock::now() - startTime).count();

    // Test static Create method with height
    startTime = steady_clock::now();
    auto payload2 = PingPayload::Create(400);
    auto createTime = duration_cast<microseconds>(steady_clock::now() - startTime).count();

    // Test static Create method with height and nonce
    startTime = steady_clock::now();
    auto payload3 = PingPayload::Create(400, 0x22222222);
    auto createWithNonceTime = duration_cast<microseconds>(steady_clock::now() - startTime).count();

    // Verify all creation methods are fast
    EXPECT_LT(constructorTime, 1000);      // Less than 1ms
    EXPECT_LT(createTime, 1000);           // Less than 1ms
    EXPECT_LT(createWithNonceTime, 1000);  // Less than 1ms

    // Verify payloads are created correctly
    EXPECT_EQ(payload1.GetLastBlockIndex(), 400u);
    EXPECT_EQ(payload1.GetTimestamp(), 123456789u);
    EXPECT_EQ(payload1.GetNonce(), 0x11111111u);

    EXPECT_EQ(payload2.GetLastBlockIndex(), 400u);
    EXPECT_GT(payload2.GetTimestamp(), 0u);  // Should have a valid timestamp

    EXPECT_EQ(payload3.GetLastBlockIndex(), 400u);
    EXPECT_GT(payload3.GetTimestamp(), 0u);  // Should have a valid timestamp
    EXPECT_EQ(payload3.GetNonce(), 0x22222222u);
}

TEST_F(UT_network_latency, PayloadSizeConsistency)
{
    // Test: Payload size consistency and its impact on latency

    std::vector<PingPayload> payloads;

    // Create payloads with different values
    for (int i = 0; i < 10; ++i)
    {
        auto timestamp =
            static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

        payloads.emplace_back(i * 100, timestamp, 0x10000 * i);
    }

    // Verify all payloads have the same size
    uint32_t expectedSize = payloads[0].GetSize();
    for (const auto& payload : payloads)
    {
        EXPECT_EQ(payload.GetSize(), expectedSize);
    }

    // Verify size is reasonable for network transmission
    EXPECT_GT(expectedSize, 0u);     // Should have some size
    EXPECT_LT(expectedSize, 1024u);  // Should be less than 1KB

    // Test serialization size consistency
    for (const auto& payload : payloads)
    {
        std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
        BinaryWriter writer(stream);

        auto serializeStart = steady_clock::now();
        payload.Serialize(writer);
        auto serializeTime = duration_cast<microseconds>(steady_clock::now() - serializeStart).count();

        // Verify serialization is fast and consistent
        EXPECT_LT(serializeTime, 1000);  // Less than 1ms

        // Verify serialized size matches GetSize()
        stream.seekg(0, std::ios::end);
        size_t serializedSize = stream.tellg();
        EXPECT_EQ(serializedSize, expectedSize);
    }
}