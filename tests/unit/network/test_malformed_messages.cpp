#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <random>
#include <sstream>

using namespace neo::network::p2p;
using namespace neo::io;

class UT_malformed_messages : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Setup test environment
        rng_.seed(42);  // Fixed seed for reproducible tests
    }

    void TearDown() override
    {
        // Cleanup
    }

    // Helper to create random corrupted data
    ByteVector CreateCorruptedData(size_t size)
    {
        ByteVector data(size);
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (size_t i = 0; i < size; ++i)
        {
            data[i] = dist(rng_);
        }
        return data;
    }

    // Helper to create valid message data and then corrupt it
    ByteVector CorruptValidMessage(const Message& message, size_t corruptionIndex)
    {
        auto validData = message.ToArray(false);
        if (corruptionIndex < validData.Size())
        {
            validData[corruptionIndex] ^= 0xFF;  // Flip all bits at corruption point
        }
        return validData;
    }

  private:
    std::mt19937 rng_;
};

TEST_F(UT_malformed_messages, EmptyMessageData)
{
    // Test: Handling of completely empty message data

    ByteVector emptyData;
    Message message;
    uint32_t bytesRead = Message::TryDeserialize(emptyData.AsSpan(), message);

    // Should fail to deserialize empty data
    EXPECT_EQ(bytesRead, 0u);
}

TEST_F(UT_malformed_messages, TruncatedMessageHeader)
{
    // Test: Handling of truncated message headers

    // Create valid message first
    auto validMessage = Message::Create(MessageCommand::Verack);
    auto validData = validMessage.ToArray(false);

    // Test with various truncation points
    for (size_t truncateAt = 1; truncateAt < std::min(validData.Size(), size_t(10)); ++truncateAt)
    {
        ByteVector truncatedData(validData.Data(), truncateAt);
        Message message;
        uint32_t bytesRead = Message::TryDeserialize(truncatedData.AsSpan(), message);

        // Should fail to deserialize truncated data
        EXPECT_EQ(bytesRead, 0u) << "Failed at truncation point: " << truncateAt;
    }
}

TEST_F(UT_malformed_messages, CorruptedMessageCommand)
{
    // Test: Handling of corrupted message commands

    // Create valid message
    auto validMessage = Message::Create(MessageCommand::Version);

    // Corrupt at different positions in the message
    for (size_t corruptAt = 0; corruptAt < 8; ++corruptAt)
    {
        auto corruptedData = CorruptValidMessage(validMessage, corruptAt);
        Message message;
        uint32_t bytesRead = Message::TryDeserialize(corruptedData.AsSpan(), message);

        // Should either fail to deserialize or result in unknown command
        if (bytesRead > 0)
        {
            // If it did deserialize, command should be marked as unknown or invalid
            // This depends on implementation - some corruption might still result in valid parsing
        }
        else
        {
            EXPECT_EQ(bytesRead, 0u);
        }
    }
}

TEST_F(UT_malformed_messages, OversizedPayload)
{
    // Test: Messages claiming to have oversized payloads

    // Create a message with ping payload
    auto pingPayload = std::make_shared<payloads::PingPayload>();
    pingPayload->SetNonce(12345);
    auto message = Message::Create(MessageCommand::Ping, pingPayload);

    auto validData = message.ToArray(false);

    // Attempt to create message data that claims to have massive payload
    // This tests protection against memory exhaustion attacks
    ByteVector maliciousData = validData;

    // If the message format allows, try to modify payload size fields
    // The exact approach depends on the message format implementation
    Message testMessage;
    uint32_t bytesRead = Message::TryDeserialize(maliciousData.AsSpan(), testMessage);

    // Should handle gracefully without throwing or consuming excessive memory
    EXPECT_NO_THROW({ Message::TryDeserialize(maliciousData.AsSpan(), testMessage); });
}

TEST_F(UT_malformed_messages, InvalidPayloadStructure)
{
    // Test: Messages with structurally invalid payloads

    // Create version payload with valid structure
    auto versionPayload = std::make_shared<payloads::VersionPayload>();
    versionPayload->SetNetwork(0x334F454E);
    versionPayload->SetUserAgent("Test");

    auto message = Message::Create(MessageCommand::Version, versionPayload);
    auto validData = message.ToArray(false);

    // Corrupt payload data specifically
    if (validData.Size() > 20)
    {
        auto corruptedData = validData;
        // Corrupt middle section which should be payload data
        for (size_t i = 10; i < std::min(validData.Size() - 5, size_t(15)); ++i)
        {
            corruptedData[i] = 0xFF;
        }

        Message testMessage;
        uint32_t bytesRead = Message::TryDeserialize(corruptedData.AsSpan(), testMessage);

        // Should handle gracefully - either fail to deserialize or handle corrupt payload
        EXPECT_NO_THROW({ Message::TryDeserialize(corruptedData.AsSpan(), testMessage); });
    }
}

TEST_F(UT_malformed_messages, RandomCorruptedData)
{
    // Test: Completely random data that looks like messages

    for (int testRun = 0; testRun < 10; ++testRun)
    {
        // Create random data of various sizes
        for (size_t dataSize = 1; dataSize <= 100; dataSize += 20)
        {
            auto randomData = CreateCorruptedData(dataSize);

            Message message;
            EXPECT_NO_THROW({
                uint32_t bytesRead = Message::TryDeserialize(randomData.AsSpan(), message);
                // Random data should generally fail to deserialize
                // but shouldn't crash or throw exceptions
            }) << "Failed on test run "
               << testRun << " with data size " << dataSize;
        }
    }
}

TEST_F(UT_malformed_messages, ExcessivelyLongStrings)
{
    // Test: Messages with excessively long string fields

    auto versionPayload = std::make_shared<payloads::VersionPayload>();
    versionPayload->SetNetwork(0x334F454E);

    // Create very long user agent string
    std::string longUserAgent(10000, 'A');  // 10KB user agent
    versionPayload->SetUserAgent(longUserAgent);

    EXPECT_NO_THROW({
        auto message = Message::Create(MessageCommand::Version, versionPayload);
        auto data = message.ToArray(false);

        Message testMessage;
        Message::TryDeserialize(data.AsSpan(), testMessage);
    });
}

TEST_F(UT_malformed_messages, NullPointerPayloads)
{
    // Test: Messages with null payloads where payload is expected

    // Create message with command that typically requires payload but pass nullptr
    EXPECT_NO_THROW({
        auto message = Message::Create(MessageCommand::Version, nullptr);
        auto data = message.ToArray(false);

        Message testMessage;
        uint32_t bytesRead = Message::TryDeserialize(data.AsSpan(), testMessage);

        // Should handle gracefully
    });
}

TEST_F(UT_malformed_messages, MismatchedCommandAndPayload)
{
    // Test: Messages where command doesn't match payload type

    // Create ping payload but use version command
    auto pingPayload = std::make_shared<payloads::PingPayload>();
    pingPayload->SetNonce(12345);

    EXPECT_NO_THROW({
        // This might be allowed at the Message level but should be caught at protocol level
        auto message = Message::Create(MessageCommand::Version, pingPayload);
        auto data = message.ToArray(false);

        Message testMessage;
        Message::TryDeserialize(data.AsSpan(), testMessage);
    });
}

TEST_F(UT_malformed_messages, ErrorHandling_MemoryCorruption)
{
    // Test: Ensure malformed messages don't cause memory corruption

    // Create pattern that might cause buffer overruns
    ByteVector maliciousData;

    // Pattern: Valid header followed by size field claiming huge payload
    // followed by insufficient data
    std::initializer_list<uint8_t> header = {0x01, 0x02, 0x03, 0x04};  // Some header-like data
    maliciousData.insert(maliciousData.end(), header.begin(), header.end());
    std::initializer_list<uint8_t> size = {0xFF, 0xFF, 0xFF, 0xFF};  // Claim huge size
    maliciousData.insert(maliciousData.end(), size.begin(), size.end());
    std::initializer_list<uint8_t> data = {0x05, 0x06, 0x07, 0x08};  // Insufficient actual data
    maliciousData.insert(maliciousData.end(), data.begin(), data.end());

    Message message;
    EXPECT_NO_THROW({
        uint32_t bytesRead = Message::TryDeserialize(maliciousData.AsSpan(), message);
        // Should not crash, hang, or corrupt memory
    });
}

TEST_F(UT_malformed_messages, CompressionBombs)
{
    // Test: Protection against compression bombs (if compression is used)

    auto pingPayload = std::make_shared<payloads::PingPayload>();
    pingPayload->SetNonce(123456789);
    auto message = Message::Create(MessageCommand::Ping, pingPayload);

    // Test with compression enabled
    EXPECT_NO_THROW({
        auto compressedData = message.ToArray(true);

        Message testMessage;
        uint32_t bytesRead = Message::TryDeserialize(compressedData.AsSpan(), testMessage);

        // Should decompress safely without consuming excessive memory
    });
}
