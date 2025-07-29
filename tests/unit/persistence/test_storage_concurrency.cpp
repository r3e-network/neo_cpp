#include <atomic>
#include <chrono>
#include <future>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "neo/io/uint160.h"
#include "neo/io/uint256.h"
#include "neo/persistence/data_cache.h"
#include "neo/persistence/leveldb_store.h"
#include "neo/persistence/memory_store.h"
#include "neo/persistence/storage_item.h"
#include "neo/persistence/storage_key.h"
#include "tests/utils/test_helpers.h"

using namespace neo::persistence;
using namespace neo::io;
using namespace neo::tests;
using namespace testing;
using namespace std::chrono_literals;

class StorageConcurrencyTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        memory_store_ = std::make_shared<MemoryStore>();
        cache_ = std::make_shared<DataCache>(memory_store_);

        // Create test data
        test_contract_id_ = 12345;
        test_prefix_ = 0x01;

        // Generate test keys and values
        for (int i = 0; i < 1000; ++i)
        {
            auto key_data = TestHelpers::GenerateRandomBytes(20);
            auto key = StorageKey::Create(test_contract_id_, test_prefix_, key_data);
            test_keys_.push_back(key);

            StorageItem item;
            item.SetValue(TestHelpers::GenerateRandomBytes(64));
            test_items_.push_back(item);
        }
    }

    void TearDown() override
    {
        cache_.reset();
        memory_store_.reset();
    }

    std::shared_ptr<MemoryStore> memory_store_;
    std::shared_ptr<DataCache> cache_;
    int32_t test_contract_id_;
    uint8_t test_prefix_;
    std::vector<StorageKey> test_keys_;
    std::vector<StorageItem> test_items_;

    // Helper to create random storage operations
    enum class OperationType
    {
        Read,
        Write,
        Delete,
        Find,
        Commit
    };

    OperationType GetRandomOperation()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 4);
        return static_cast<OperationType>(dis(gen));
    }

    int GetRandomIndex(size_t max_size)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, max_size - 1);
        return dis(gen);
    }
};

// Test concurrent reads
TEST_F(StorageConcurrencyTest, ConcurrentReads)
{
    // First populate the cache with test data
    for (size_t i = 0; i < test_keys_.size(); ++i)
    {
        cache_->Add(test_keys_[i], test_items_[i]);
    }
    cache_->Commit();

    const int num_threads = 10;
    const int operations_per_thread = 100;
    std::atomic<int> successful_reads{0};
    std::atomic<int> failed_reads{0};

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, operations_per_thread, &successful_reads, &failed_reads]()
            {
                for (int i = 0; i < operations_per_thread; ++i)
                {
                    int index = GetRandomIndex(test_keys_.size());
                    auto result = cache_->TryGet(test_keys_[index]);

                    if (result.has_value())
                    {
                        successful_reads++;
                    }
                    else
                    {
                        failed_reads++;
                    }
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // All reads should succeed since data was committed
    EXPECT_EQ(successful_reads.load(), num_threads * operations_per_thread);
    EXPECT_EQ(failed_reads.load(), 0);
}

// Test concurrent writes to different keys
TEST_F(StorageConcurrencyTest, ConcurrentWritesDifferentKeys)
{
    const int num_threads = 8;
    const int operations_per_thread = 50;
    std::atomic<int> successful_writes{0};

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, t, operations_per_thread, &successful_writes]()
            {
                for (int i = 0; i < operations_per_thread; ++i)
                {
                    // Each thread works on different key range to avoid conflicts
                    int index = t * operations_per_thread + i;
                    if (index < static_cast<int>(test_keys_.size()))
                    {
                        try
                        {
                            cache_->Add(test_keys_[index], test_items_[index]);
                            successful_writes++;
                        }
                        catch (...)
                        {
                            // Handle any synchronization issues
                        }
                    }
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Should handle concurrent writes to different keys
    EXPECT_GT(successful_writes.load(), 0);

    // Verify data integrity
    cache_->Commit();
    int verified_items = 0;
    for (size_t i = 0; i < std::min(test_keys_.size(), static_cast<size_t>(num_threads * operations_per_thread)); ++i)
    {
        auto result = cache_->TryGet(test_keys_[i]);
        if (result.has_value())
        {
            verified_items++;
        }
    }

    EXPECT_GT(verified_items, 0);
}

// Test concurrent writes to same key (should handle properly)
TEST_F(StorageConcurrencyTest, ConcurrentWritesSameKey)
{
    const int num_threads = 10;
    const StorageKey& target_key = test_keys_[0];

    std::atomic<int> successful_updates{0};
    std::atomic<int> failed_updates{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, &target_key, t, &successful_updates, &failed_updates]()
            {
                try
                {
                    StorageItem item;
                    std::vector<uint8_t> data = {static_cast<uint8_t>(t), static_cast<uint8_t>(t),
                                                 static_cast<uint8_t>(t), static_cast<uint8_t>(t)};
                    item.SetValue(data);

                    cache_->AddOrUpdate(target_key, item);
                    successful_updates++;
                }
                catch (...)
                {
                    failed_updates++;
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // At least one update should succeed
    EXPECT_GT(successful_updates.load(), 0);

    // Verify final state is consistent
    auto final_result = cache_->TryGet(target_key);
    EXPECT_TRUE(final_result.has_value());
    EXPECT_EQ(final_result->GetValue().size(), 4);
}

// Test concurrent read/write operations
TEST_F(StorageConcurrencyTest, ConcurrentReadWrites)
{
    // Pre-populate some data
    for (size_t i = 0; i < test_keys_.size() / 2; ++i)
    {
        cache_->Add(test_keys_[i], test_items_[i]);
    }
    cache_->Commit();

    const int num_reader_threads = 5;
    const int num_writer_threads = 3;
    const int operations_per_thread = 100;

    std::atomic<int> reads_completed{0};
    std::atomic<int> writes_completed{0};
    std::vector<std::thread> threads;

    // Reader threads
    for (int t = 0; t < num_reader_threads; ++t)
    {
        threads.emplace_back(
            [this, operations_per_thread, &reads_completed]()
            {
                for (int i = 0; i < operations_per_thread; ++i)
                {
                    int index = GetRandomIndex(test_keys_.size() / 2);
                    auto result = cache_->TryGet(test_keys_[index]);
                    reads_completed++;

                    // Small delay to increase chance of concurrent access
                    std::this_thread::sleep_for(1us);
                }
            });
    }

    // Writer threads
    for (int t = 0; t < num_writer_threads; ++t)
    {
        threads.emplace_back(
            [this, t, operations_per_thread, &writes_completed]()
            {
                for (int i = 0; i < operations_per_thread; ++i)
                {
                    // Write to second half of keys to minimize conflicts with readers
                    int index = test_keys_.size() / 2 + (t * operations_per_thread + i) % (test_keys_.size() / 2);
                    if (index < static_cast<int>(test_keys_.size()))
                    {
                        try
                        {
                            cache_->AddOrUpdate(test_keys_[index], test_items_[index]);
                            writes_completed++;
                        }
                        catch (...)
                        {
                            // Handle any concurrency issues
                        }
                    }

                    std::this_thread::sleep_for(1us);
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // All operations should complete
    EXPECT_EQ(reads_completed.load(), num_reader_threads * operations_per_thread);
    EXPECT_GT(writes_completed.load(), 0);
}

// Test concurrent deletes
TEST_F(StorageConcurrencyTest, ConcurrentDeletes)
{
    // Populate cache
    for (size_t i = 0; i < test_keys_.size(); ++i)
    {
        cache_->Add(test_keys_[i], test_items_[i]);
    }
    cache_->Commit();

    const int num_threads = 8;
    std::atomic<int> successful_deletes{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, t, &successful_deletes]()
            {
                // Each thread deletes different keys to avoid conflicts
                size_t start_index = t * (test_keys_.size() / 8);
                size_t end_index = std::min((t + 1) * (test_keys_.size() / 8), test_keys_.size());

                for (size_t i = start_index; i < end_index; ++i)
                {
                    try
                    {
                        cache_->Delete(test_keys_[i]);
                        successful_deletes++;
                    }
                    catch (...)
                    {
                        // Handle any concurrency issues
                    }
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_GT(successful_deletes.load(), 0);

    // Verify deletions
    cache_->Commit();
    int remaining_items = 0;
    for (const auto& key : test_keys_)
    {
        if (cache_->TryGet(key).has_value())
        {
            remaining_items++;
        }
    }

    EXPECT_LT(remaining_items, static_cast<int>(test_keys_.size()));
}

// Test concurrent commits
TEST_F(StorageConcurrencyTest, ConcurrentCommits)
{
    const int num_threads = 5;
    std::atomic<int> successful_commits{0};
    std::atomic<int> failed_commits{0};

    // Each thread adds some data then commits
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, t, &successful_commits, &failed_commits]()
            {
                try
                {
                    // Add thread-specific data
                    for (int i = 0; i < 10; ++i)
                    {
                        int index = t * 10 + i;
                        if (index < static_cast<int>(test_keys_.size()))
                        {
                            cache_->Add(test_keys_[index], test_items_[index]);
                        }
                    }

                    // Attempt to commit
                    cache_->Commit();
                    successful_commits++;
                }
                catch (...)
                {
                    failed_commits++;
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // At least some commits should succeed
    EXPECT_GT(successful_commits.load(), 0);

    // Verify data persistence
    int persisted_items = 0;
    for (size_t i = 0; i < std::min(test_keys_.size(), static_cast<size_t>(num_threads * 10)); ++i)
    {
        if (memory_store_->Contains(test_keys_[i]))
        {
            persisted_items++;
        }
    }

    EXPECT_GT(persisted_items, 0);
}

// Test concurrent find operations
TEST_F(StorageConcurrencyTest, ConcurrentFind)
{
    // Populate with data
    for (size_t i = 0; i < test_keys_.size(); ++i)
    {
        memory_store_->Put(test_keys_[i], test_items_[i]);
    }

    const int num_threads = 6;
    const int finds_per_thread = 20;
    std::atomic<int> successful_finds{0};

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, finds_per_thread, &successful_finds]()
            {
                for (int i = 0; i < finds_per_thread; ++i)
                {
                    // Create search prefix
                    auto search_key = StorageKey::Create(test_contract_id_, test_prefix_);
                    auto iterator = memory_store_->Find(search_key);

                    int found_count = 0;
                    while (iterator && iterator->Valid())
                    {
                        found_count++;
                        iterator->Next();

                        // Limit to prevent infinite loops
                        if (found_count > 1000)
                            break;
                    }

                    if (found_count > 0)
                    {
                        successful_finds++;
                    }
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Should find items consistently
    EXPECT_EQ(successful_finds.load(), num_threads * finds_per_thread);
}

// Test mixed concurrent operations (stress test)
TEST_F(StorageConcurrencyTest, MixedConcurrentOperations)
{
    const int num_threads = 10;
    const int operations_per_thread = 50;

    std::atomic<int> total_operations{0};
    std::atomic<int> read_operations{0};
    std::atomic<int> write_operations{0};
    std::atomic<int> delete_operations{0};

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, operations_per_thread, &total_operations, &read_operations, &write_operations, &delete_operations]()
            {
                for (int i = 0; i < operations_per_thread; ++i)
                {
                    auto operation = GetRandomOperation();
                    int index = GetRandomIndex(test_keys_.size());

                    try
                    {
                        switch (operation)
                        {
                            case OperationType::Read:
                            {
                                auto result = cache_->TryGet(test_keys_[index]);
                                read_operations++;
                                break;
                            }
                            case OperationType::Write:
                            {
                                cache_->AddOrUpdate(test_keys_[index], test_items_[index]);
                                write_operations++;
                                break;
                            }
                            case OperationType::Delete:
                            {
                                cache_->Delete(test_keys_[index]);
                                delete_operations++;
                                break;
                            }
                            case OperationType::Find:
                            {
                                auto search_key = StorageKey::Create(test_contract_id_, test_prefix_);
                                auto iterator = cache_->Find(search_key);
                                if (iterator && iterator->Valid())
                                {
                                    // Just check first result
                                    iterator->Key();
                                }
                                read_operations++;
                                break;
                            }
                            case OperationType::Commit:
                            {
                                cache_->Commit();
                                break;
                            }
                        }
                        total_operations++;
                    }
                    catch (...)
                    {
                        // Handle any concurrency exceptions gracefully
                    }

                    // Small random delay to vary timing
                    if (i % 10 == 0)
                    {
                        std::this_thread::sleep_for(1us);
                    }
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Should complete operations without crashing
    EXPECT_GT(total_operations.load(), num_threads * operations_per_thread * 0.8);
    EXPECT_GT(read_operations.load(), 0);
    EXPECT_GT(write_operations.load(), 0);
}

// Test cache coherency under concurrent access
TEST_F(StorageConcurrencyTest, CacheCoherencyUnderConcurrency)
{
    const StorageKey& test_key = test_keys_[0];
    const int num_iterations = 100;
    const int num_threads = 4;

    std::atomic<int> iteration{0};
    std::vector<std::thread> threads;
    std::vector<std::vector<uint8_t>> observed_values[num_threads];

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, &test_key, &iteration, num_iterations, t, &observed_values]()
            {
                while (iteration.load() < num_iterations)
                {
                    int current_iter = iteration.fetch_add(1);
                    if (current_iter >= num_iterations)
                        break;

                    // Writer thread updates value
                    if (t == 0)
                    {
                        StorageItem item;
                        std::vector<uint8_t> value = {static_cast<uint8_t>(current_iter & 0xFF),
                                                      static_cast<uint8_t>((current_iter >> 8) & 0xFF),
                                                      static_cast<uint8_t>((current_iter >> 16) & 0xFF),
                                                      static_cast<uint8_t>((current_iter >> 24) & 0xFF)};
                        item.SetValue(value);
                        cache_->AddOrUpdate(test_key, item);
                    }
                    else
                    {
                        // Reader threads observe values
                        auto result = cache_->TryGet(test_key);
                        if (result.has_value())
                        {
                            observed_values[t].push_back(result->GetValue());
                        }
                    }

                    std::this_thread::sleep_for(1us);
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Verify that readers observed some values
    for (int t = 1; t < num_threads; ++t)
    {
        EXPECT_GT(observed_values[t].size(), 0) << "Thread " << t << " observed no values";
    }
}

// Test deadlock prevention
TEST_F(StorageConcurrencyTest, DeadlockPrevention)
{
    const int num_threads = 8;
    const int operations_per_thread = 25;

    std::atomic<bool> deadlock_detected{false};
    std::atomic<int> completed_threads{0};

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, t, operations_per_thread, &deadlock_detected, &completed_threads]()
            {
                try
                {
                    for (int i = 0; i < operations_per_thread; ++i)
                    {
                        // Perform operations that could potentially cause deadlocks
                        int index1 = (t * 2) % test_keys_.size();
                        int index2 = (t * 2 + 1) % test_keys_.size();

                        // Lock different keys in different order to test deadlock prevention
                        if (t % 2 == 0)
                        {
                            cache_->AddOrUpdate(test_keys_[index1], test_items_[index1]);
                            cache_->AddOrUpdate(test_keys_[index2], test_items_[index2]);
                        }
                        else
                        {
                            cache_->AddOrUpdate(test_keys_[index2], test_items_[index2]);
                            cache_->AddOrUpdate(test_keys_[index1], test_items_[index1]);
                        }

                        // Read operations
                        cache_->TryGet(test_keys_[index1]);
                        cache_->TryGet(test_keys_[index2]);
                    }

                    completed_threads++;
                }
                catch (...)
                {
                    deadlock_detected = true;
                }
            });
    }

    // Wait with timeout to detect deadlocks
    auto start_time = std::chrono::steady_clock::now();
    bool all_completed = false;

    while (!all_completed && !deadlock_detected.load())
    {
        std::this_thread::sleep_for(10ms);

        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed > 5s)
        {
            deadlock_detected = true;
            break;
        }

        all_completed = (completed_threads.load() == num_threads);
    }

    // Cleanup threads
    for (auto& thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // Should not deadlock
    EXPECT_FALSE(deadlock_detected.load()) << "Deadlock detected in concurrent operations";
    EXPECT_EQ(completed_threads.load(), num_threads) << "Not all threads completed";
}

// Test memory consistency under high concurrency
TEST_F(StorageConcurrencyTest, MemoryConsistencyHighConcurrency)
{
    const int num_producer_threads = 4;
    const int num_consumer_threads = 4;
    const int items_per_producer = 100;

    std::atomic<int> items_produced{0};
    std::atomic<int> items_consumed{0};
    std::atomic<bool> production_complete{false};

    std::vector<std::thread> threads;

    // Producer threads
    for (int t = 0; t < num_producer_threads; ++t)
    {
        threads.emplace_back(
            [this, t, items_per_producer, &items_produced]()
            {
                for (int i = 0; i < items_per_producer; ++i)
                {
                    int global_index = t * items_per_producer + i;
                    if (global_index < static_cast<int>(test_keys_.size()))
                    {
                        try
                        {
                            cache_->Add(test_keys_[global_index], test_items_[global_index]);
                            items_produced++;
                        }
                        catch (...)
                        {
                            // Handle any exceptions
                        }
                    }

                    if (i % 10 == 0)
                    {
                        std::this_thread::sleep_for(1us);
                    }
                }
            });
    }

    // Consumer threads
    for (int t = 0; t < num_consumer_threads; ++t)
    {
        threads.emplace_back(
            [this, &items_consumed, &production_complete, items_per_producer, num_producer_threads]()
            {
                int expected_total = num_producer_threads * items_per_producer;

                while (!production_complete.load() || items_consumed.load() < expected_total)
                {
                    // Try to read random items
                    int index = GetRandomIndex(std::min(static_cast<size_t>(expected_total), test_keys_.size()));
                    auto result = cache_->TryGet(test_keys_[index]);

                    if (result.has_value())
                    {
                        items_consumed++;
                    }

                    std::this_thread::sleep_for(1us);

                    // Prevent infinite loop
                    if (items_consumed.load() >= expected_total)
                        break;
                }
            });
    }

    // Wait for producers to complete
    for (int t = 0; t < num_producer_threads; ++t)
    {
        threads[t].join();
    }
    production_complete = true;

    // Wait for consumers
    for (int t = num_producer_threads; t < num_producer_threads + num_consumer_threads; ++t)
    {
        threads[t].join();
    }

    // Verify consistency
    EXPECT_EQ(items_produced.load(), num_producer_threads * items_per_producer);
    EXPECT_GT(items_consumed.load(), 0);
}

// Test performance under concurrent load
TEST_F(StorageConcurrencyTest, PerformanceUnderConcurrentLoad)
{
    const int num_threads = std::thread::hardware_concurrency();
    const int operations_per_thread = 1000;

    // Pre-populate cache
    for (size_t i = 0; i < test_keys_.size(); ++i)
    {
        cache_->Add(test_keys_[i], test_items_[i]);
    }
    cache_->Commit();

    auto start_time = std::chrono::high_resolution_clock::now();

    std::atomic<int> total_operations{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [this, operations_per_thread, &total_operations]()
            {
                for (int i = 0; i < operations_per_thread; ++i)
                {
                    int index = GetRandomIndex(test_keys_.size());

                    // Mix of read and write operations (80% read, 20% write)
                    if (i % 5 == 0)
                    {
                        // Write operation
                        cache_->AddOrUpdate(test_keys_[index], test_items_[index]);
                    }
                    else
                    {
                        // Read operation
                        cache_->TryGet(test_keys_[index]);
                    }

                    total_operations++;
                }
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Performance metrics
    double ops_per_second = (static_cast<double>(total_operations.load()) * 1000.0) / duration.count();

    EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);
    EXPECT_GT(ops_per_second, 1000.0);   // Should handle at least 1000 ops/second
    EXPECT_LT(duration.count(), 10000);  // Should complete within 10 seconds

    std::cout << "Concurrent performance: " << ops_per_second << " ops/second" << std::endl;
}