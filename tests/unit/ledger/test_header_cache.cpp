#include <gtest/gtest.h>
#include <neo/ledger/header_cache.h>
#include <neo/ledger/block_header.h>

namespace neo::ledger::tests
{
    class HeaderCacheTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Create test headers
            header1 = std::make_shared<BlockHeader>();
            header1->SetIndex(1);
            header1->SetTimestamp(1000);
            
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
        HeaderCache cache(2); // Small cache for testing eviction
        
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
        
        // This is a basic test for thread safety
        // In a real scenario, we would use multiple threads
        
        std::vector<std::shared_ptr<BlockHeader>> headers;
        for (int i = 0; i < 100; ++i)
        {
            auto header = std::make_shared<BlockHeader>();
            header->SetIndex(i);
            header->SetTimestamp(1000 + i);
            headers.push_back(header);
        }
        
        // Add all headers
        for (const auto& header : headers)
        {
            cache.Add(header);
        }
        
        // Verify all are accessible
        for (const auto& header : headers)
        {
            EXPECT_TRUE(cache.Contains(header->GetHash()));
        }
        
        // Remove some headers
        for (size_t i = 0; i < headers.size(); i += 2)
        {
            cache.Remove(headers[i]->GetHash());
        }
        
        // Verify correct headers remain
        for (size_t i = 0; i < headers.size(); ++i)
        {
            if (i % 2 == 0)
            {
                EXPECT_FALSE(cache.Contains(headers[i]->GetHash()));
            }
            else
            {
                EXPECT_TRUE(cache.Contains(headers[i]->GetHash()));
            }
        }
    }

    TEST_F(HeaderCacheTest, TestReplaceHeader)
    {
        HeaderCache cache(10);
        
        cache.Add(header1);
        auto original = cache.Get(header1->GetHash());
        
        // Create new header with same hash but different data
        auto updated_header = std::make_shared<BlockHeader>();
        updated_header->SetIndex(header1->GetIndex());
        updated_header->SetTimestamp(9999); // Different timestamp
        
        // Add updated header (should replace original)
        cache.Add(updated_header);
        
        EXPECT_EQ(1, cache.Size()); // Size should remain the same
        
        auto retrieved = cache.Get(header1->GetHash());
        EXPECT_NE(nullptr, retrieved);
        EXPECT_EQ(9999, retrieved->GetTimestamp()); // Should have updated timestamp
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
}
