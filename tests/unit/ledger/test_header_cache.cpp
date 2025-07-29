#include <gtest/gtest.h>
#include <neo/ledger/block_header.h>
#include <neo/ledger/header_cache.h>

namespace neo::ledger::tests
{
class HeaderCacheTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create test headers with full initialization
        header1 = std::make_shared<BlockHeader>();
        header1->SetIndex(1);
        header1->SetTimestamp(1000);
        header1->SetPrevHash(neo::io::UInt256::Zero());
        header1->SetMerkleRoot(neo::io::UInt256::Zero());
        header1->SetNonce(0);
        header1->SetPrimaryIndex(0);
        header1->SetNextConsensus(neo::io::UInt160::Zero());

        header2 = std::make_shared<BlockHeader>();
        header2->SetIndex(2);
        header2->SetTimestamp(2000);

        header3 = std::make_shared<BlockHeader>();
        header3->SetIndex(3);
        header3->SetTimestamp(3000);
    }

    std::shared_ptr<BlockHeader> header1;
    std::shared_ptr<BlockHeader> header2;
    std::shared_ptr<BlockHeader> header3;
};

TEST_F(HeaderCacheTest, TestConstructor)
{
    HeaderCache cache(100);

    EXPECT_EQ(100, cache.MaxSize());
    EXPECT_EQ(0, cache.Size());
    EXPECT_FALSE(cache.IsFull());
}

TEST_F(HeaderCacheTest, TestConstructorZeroSize)
{
    HeaderCache cache(0);

    // Should default to minimum size of 1
    EXPECT_EQ(1, cache.MaxSize());
}

TEST_F(HeaderCacheTest, TestAddAndGet)
{
    HeaderCache cache(10);

    cache.Add(header1);

    EXPECT_EQ(1, cache.Size());
    EXPECT_TRUE(cache.Contains(header1->GetHash()));

    auto retrieved = cache.Get(header1->GetHash());
    EXPECT_NE(nullptr, retrieved);
    EXPECT_EQ(header1->GetIndex(), retrieved->GetIndex());
}

TEST_F(HeaderCacheTest, TestAddNull)
{
    HeaderCache cache(10);

    cache.Add(nullptr);

    EXPECT_EQ(0, cache.Size());
}

TEST_F(HeaderCacheTest, TestGetNonExistent)
{
    HeaderCache cache(10);

    auto hash = io::UInt256::Parse("0x1234567890123456789012345678901234567890123456789012345678901234");
    auto retrieved = cache.Get(hash);

    EXPECT_EQ(nullptr, retrieved);
    EXPECT_FALSE(cache.Contains(hash));
}

TEST_F(HeaderCacheTest, TestRemove)
{
    HeaderCache cache(10);

    cache.Add(header1);
    EXPECT_TRUE(cache.Contains(header1->GetHash()));

    bool removed = cache.Remove(header1->GetHash());
    EXPECT_TRUE(removed);
    EXPECT_FALSE(cache.Contains(header1->GetHash()));
    EXPECT_EQ(0, cache.Size());

    // Try to remove again
    removed = cache.Remove(header1->GetHash());
    EXPECT_FALSE(removed);
}

TEST_F(HeaderCacheTest, TestClear)
{
    HeaderCache cache(10);

    cache.Add(header1);
    cache.Add(header2);
    cache.Add(header3);

    EXPECT_EQ(3, cache.Size());

    cache.Clear();

    EXPECT_EQ(0, cache.Size());
    EXPECT_FALSE(cache.Contains(header1->GetHash()));
    EXPECT_FALSE(cache.Contains(header2->GetHash()));
    EXPECT_FALSE(cache.Contains(header3->GetHash()));
}

TEST_F(HeaderCacheTest, TestEviction)
{
    HeaderCache cache(2);  // Small cache for testing eviction

    cache.Add(header1);
    cache.Add(header2);

    EXPECT_EQ(2, cache.Size());
    EXPECT_TRUE(cache.IsFull());

    // Add third header, should evict one
    cache.Add(header3);

    EXPECT_EQ(2, cache.Size());
    EXPECT_TRUE(cache.IsFull());

    // header1 should be evicted (lowest index)
    EXPECT_FALSE(cache.Contains(header1->GetHash()));
    EXPECT_TRUE(cache.Contains(header2->GetHash()));
    EXPECT_TRUE(cache.Contains(header3->GetHash()));
}

TEST_F(HeaderCacheTest, TestMultipleHeaders)
{
    HeaderCache cache(100);

    std::vector<std::shared_ptr<BlockHeader>> headers;
    for (int i = 0; i < 50; ++i)
    {
        auto header = std::make_shared<BlockHeader>();
        header->SetIndex(i);
        header->SetTimestamp(1000 + i);
        headers.push_back(header);
        cache.Add(header);
    }

    EXPECT_EQ(50, cache.Size());
    EXPECT_FALSE(cache.IsFull());

    // Verify all headers are accessible
    for (const auto& header : headers)
    {
        EXPECT_TRUE(cache.Contains(header->GetHash()));
        auto retrieved = cache.Get(header->GetHash());
        EXPECT_NE(nullptr, retrieved);
        EXPECT_EQ(header->GetIndex(), retrieved->GetIndex());
    }
}

TEST_F(HeaderCacheTest, TestThreadSafety)
{
    HeaderCache cache(1000);

    // Complete multi-threading implementation for real thread safety testing
    // Use multiple threads to concurrently access the cache

    const int num_threads = 8;
    const int headers_per_thread = 50;
    std::vector<std::shared_ptr<BlockHeader>> all_headers;

    // Pre-create headers for all threads
    for (int thread_id = 0; thread_id < num_threads; ++thread_id)
    {
        for (int i = 0; i < headers_per_thread; ++i)
        {
            auto header = std::make_shared<BlockHeader>();
            header->SetIndex(thread_id * headers_per_thread + i);
            header->SetTimestamp(1000 + thread_id * headers_per_thread + i);
            all_headers.push_back(header);
        }
    }

    std::vector<std::thread> threads;
    std::atomic<int> successful_adds(0);
    std::atomic<int> successful_lookups(0);
    std::atomic<int> successful_removes(0);
    std::mutex test_mutex;
    std::vector<std::string> thread_errors;

    // Thread function for concurrent cache operations
    auto thread_worker = [&](int thread_id)
    {
        try
        {
            // Phase 1: Add headers
            for (int i = 0; i < headers_per_thread; ++i)
            {
                int header_index = thread_id * headers_per_thread + i;
                auto& header = all_headers[header_index];

                cache.Add(header);
                successful_adds.fetch_add(1);

                // Small delay to increase contention
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }

            // Phase 2: Concurrent lookups
            for (int i = 0; i < headers_per_thread; ++i)
            {
                int header_index = thread_id * headers_per_thread + i;
                auto& header = all_headers[header_index];

                if (cache.Contains(header->GetHash()))
                {
                    successful_lookups.fetch_add(1);
                }

                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }

            // Phase 3: Remove every other header
            for (int i = 0; i < headers_per_thread; i += 2)
            {
                int header_index = thread_id * headers_per_thread + i;
                auto& header = all_headers[header_index];

                cache.Remove(header->GetHash());
                successful_removes.fetch_add(1);

                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
        catch (const std::exception& e)
        {
            std::lock_guard<std::mutex> lock(test_mutex);
            thread_errors.push_back("Thread " + std::to_string(thread_id) + ": " + e.what());
        }
    };

    // Start all threads
    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(thread_worker, i);
    }

    // Wait for all threads to complete
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Verify no errors occurred during concurrent access
    EXPECT_TRUE(thread_errors.empty()) << "Errors during concurrent access: " << thread_errors.size();
    if (!thread_errors.empty())
    {
        for (const auto& error : thread_errors)
        {
            std::cout << error << std::endl;
        }
    }

    // Verify operation counts
    EXPECT_EQ(successful_adds.load(), num_threads * headers_per_thread);
    EXPECT_EQ(successful_lookups.load(), num_threads * headers_per_thread);
    EXPECT_EQ(successful_removes.load(), num_threads * (headers_per_thread / 2));

    // Verify final cache state
    int expected_remaining = num_threads * (headers_per_thread / 2);
    int actual_remaining = 0;

    for (size_t i = 0; i < all_headers.size(); ++i)
    {
        bool should_exist = (i % 2 == 1);  // Only odd-indexed headers should remain
        bool actually_exists = cache.Contains(all_headers[i]->GetHash());

        if (should_exist)
        {
            EXPECT_TRUE(actually_exists) << "Header " << i << " should exist but doesn't";
            if (actually_exists)
                actual_remaining++;
        }
        else
        {
            EXPECT_FALSE(actually_exists) << "Header " << i << " should not exist but does";
        }
    }

    EXPECT_EQ(actual_remaining, expected_remaining);

    // Test concurrent access patterns
    std::atomic<bool> keep_running(true);
    std::atomic<int> reader_successes(0);
    std::atomic<int> writer_successes(0);

    // Reader threads
    std::vector<std::thread> reader_threads;
    for (int i = 0; i < 2; ++i)
    {
        reader_threads.emplace_back(
            [&]()
            {
                while (keep_running.load())
                {
                    for (const auto& header : all_headers)
                    {
                        if (cache.Contains(header->GetHash()))
                        {
                            reader_successes.fetch_add(1);
                        }
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                        if (!keep_running.load())
                            break;
                    }
                }
            });
    }

    // Writer thread
    std::thread writer_thread(
        [&]()
        {
            int write_counter = 0;
            while (keep_running.load() && write_counter < 50)
            {
                auto new_header = std::make_shared<BlockHeader>();
                new_header->SetIndex(10000 + write_counter);
                new_header->SetTimestamp(10000 + write_counter);

                cache.Add(new_header);
                writer_successes.fetch_add(1);
                write_counter++;

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

    // Let concurrent access run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    keep_running.store(false);

    // Wait for all concurrent threads
    for (auto& reader : reader_threads)
    {
        reader.join();
    }
    writer_thread.join();

    // Verify concurrent operations completed successfully
    EXPECT_GT(reader_successes.load(), 0);
    EXPECT_GT(writer_successes.load(), 0);
}

TEST_F(HeaderCacheTest, TestReplaceHeader)
{
    HeaderCache cache(10);

    cache.Add(header1);
    auto original = cache.Get(header1->GetHash());

    // Create new header with same index but different timestamp (different hash)
    auto updated_header = std::make_shared<BlockHeader>();
    updated_header->SetIndex(header1->GetIndex());
    updated_header->SetTimestamp(9999);  // Different timestamp
    // Set other fields to match header1 to ensure only timestamp differs
    updated_header->SetPrevHash(header1->GetPrevHash());
    updated_header->SetMerkleRoot(header1->GetMerkleRoot());
    updated_header->SetNonce(header1->GetNonce());
    updated_header->SetPrimaryIndex(header1->GetPrimaryIndex());
    updated_header->SetNextConsensus(header1->GetNextConsensus());

    // Add updated header (should replace original by index)
    cache.Add(updated_header);

    // HeaderCache replacement is working correctly
    EXPECT_EQ(1, cache.Size());  // Size remains the same after replacement

    // Original header should no longer be retrievable by its hash (replaced)
    auto retrieved = cache.Get(header1->GetHash());
    EXPECT_EQ(nullptr, retrieved);  // Original header removed

    // New header should be retrievable by its hash
    retrieved = cache.Get(updated_header->GetHash());
    EXPECT_NE(nullptr, retrieved);
    EXPECT_EQ(9999, retrieved->GetTimestamp());  // New header has correct timestamp

    // Getting by index should return the newer header
    retrieved = cache.Get(header1->GetIndex());
    EXPECT_NE(nullptr, retrieved);
    EXPECT_EQ(9999, retrieved->GetTimestamp());  // Should return the newer header
}

TEST_F(HeaderCacheTest, TestCapacityOne)
{
    HeaderCache cache(1);

    cache.Add(header1);
    EXPECT_EQ(1, cache.Size());
    EXPECT_TRUE(cache.IsFull());
    EXPECT_TRUE(cache.Contains(header1->GetHash()));

    cache.Add(header2);
    EXPECT_EQ(1, cache.Size());
    EXPECT_TRUE(cache.IsFull());
    EXPECT_FALSE(cache.Contains(header1->GetHash()));
    EXPECT_TRUE(cache.Contains(header2->GetHash()));
}

TEST_F(HeaderCacheTest, TestLargeCache)
{
    HeaderCache cache(10000);

    // Add many headers
    for (int i = 0; i < 5000; ++i)
    {
        auto header = std::make_shared<BlockHeader>();
        header->SetIndex(i);
        header->SetTimestamp(1000 + i);
        cache.Add(header);
    }

    EXPECT_EQ(5000, cache.Size());
    EXPECT_FALSE(cache.IsFull());

    // Clear and verify
    cache.Clear();
    EXPECT_EQ(0, cache.Size());
    EXPECT_FALSE(cache.IsFull());
}
}  // namespace neo::ledger::tests
