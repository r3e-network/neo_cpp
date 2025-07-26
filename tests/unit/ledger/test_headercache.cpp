// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/ledger/test_headercache.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_LEDGER_TEST_HEADERCACHE_CPP_H
#define TESTS_UNIT_LEDGER_TEST_HEADERCACHE_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/ledger/header_cache.h>

namespace neo {
namespace test {

class HeaderCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for HeaderCache testing
        cache_capacity = 100;
        header_cache = std::make_shared<ledger::HeaderCache>(cache_capacity);
        
        // Create test headers
        header1 = CreateTestHeader(
            io::UInt256::Parse("1111111111111111111111111111111111111111111111111111111111111111"),
            io::UInt256::Parse("0000000000000000000000000000000000000000000000000000000000000000"),
            0
        );
        
        header2 = CreateTestHeader(
            io::UInt256::Parse("2222222222222222222222222222222222222222222222222222222222222222"),
            header1->GetHash(),
            1
        );
        
        header3 = CreateTestHeader(
            io::UInt256::Parse("3333333333333333333333333333333333333333333333333333333333333333"),
            header2->GetHash(),
            2
        );
        
        header4 = CreateTestHeader(
            io::UInt256::Parse("4444444444444444444444444444444444444444444444444444444444444444"),
            header3->GetHash(),
            3
        );
    }

    void TearDown() override {
        // Clean up test fixtures
        header_cache.reset();
        header1.reset();
        header2.reset();
        header3.reset();
        header4.reset();
    }

    // Helper methods and test data for HeaderCache testing
    size_t cache_capacity;
    std::shared_ptr<ledger::HeaderCache> header_cache;
    std::shared_ptr<ledger::Header> header1;
    std::shared_ptr<ledger::Header> header2;
    std::shared_ptr<ledger::Header> header3;
    std::shared_ptr<ledger::Header> header4;
    
    // Helper to create test headers
    std::shared_ptr<ledger::Header> CreateTestHeader(const io::UInt256& hash, const io::UInt256& prev_hash, uint32_t index) {
        auto header = std::make_shared<ledger::Header>();
        header->SetHash(hash);
        header->SetPreviousHash(prev_hash);
        header->SetIndex(index);
        header->SetTimestamp(1640995200 + index * 15000); // 15 seconds apart
        header->SetNonce(12345 + index);
        return header;
    }
};

// HeaderCache test methods converted from C# UT_HeaderCache.cs functionality

TEST_F(HeaderCacheTest, ConstructorCreatesEmptyCache) {
    EXPECT_EQ(header_cache->GetCount(), 0);
    EXPECT_TRUE(header_cache->IsEmpty());
    EXPECT_EQ(header_cache->GetCapacity(), cache_capacity);
}

TEST_F(HeaderCacheTest, AddHeaderToCache) {
    bool added = header_cache->Add(header1);
    EXPECT_TRUE(added);
    EXPECT_EQ(header_cache->GetCount(), 1);
    EXPECT_FALSE(header_cache->IsEmpty());
}

TEST_F(HeaderCacheTest, AddDuplicateHeader) {
    EXPECT_TRUE(header_cache->Add(header1));
    EXPECT_EQ(header_cache->GetCount(), 1);
    
    // Adding same header again should fail or replace
    bool added_again = header_cache->Add(header1);
    EXPECT_EQ(header_cache->GetCount(), 1); // Count should remain same
}

TEST_F(HeaderCacheTest, AddMultipleHeaders) {
    EXPECT_TRUE(header_cache->Add(header1));
    EXPECT_TRUE(header_cache->Add(header2));
    EXPECT_TRUE(header_cache->Add(header3));
    
    EXPECT_EQ(header_cache->GetCount(), 3);
}

TEST_F(HeaderCacheTest, GetHeaderByHash) {
    header_cache->Add(header1);
    header_cache->Add(header2);
    
    auto retrieved1 = header_cache->Get(header1->GetHash());
    EXPECT_NE(retrieved1, nullptr);
    EXPECT_EQ(retrieved1->GetHash(), header1->GetHash());
    EXPECT_EQ(retrieved1->GetIndex(), header1->GetIndex());
    
    auto retrieved2 = header_cache->Get(header2->GetHash());
    EXPECT_NE(retrieved2, nullptr);
    EXPECT_EQ(retrieved2->GetHash(), header2->GetHash());
}

TEST_F(HeaderCacheTest, GetHeaderByIndex) {
    header_cache->Add(header1);
    header_cache->Add(header2);
    
    auto retrieved1 = header_cache->Get(0);
    EXPECT_NE(retrieved1, nullptr);
    EXPECT_EQ(retrieved1->GetIndex(), 0);
    
    auto retrieved2 = header_cache->Get(1);
    EXPECT_NE(retrieved2, nullptr);
    EXPECT_EQ(retrieved2->GetIndex(), 1);
}

TEST_F(HeaderCacheTest, GetNonExistentHeader) {
    header_cache->Add(header1);
    
    io::UInt256 non_existent = io::UInt256::Parse("9999999999999999999999999999999999999999999999999999999999999999");
    auto result = header_cache->Get(non_existent);
    EXPECT_EQ(result, nullptr);
    
    auto result_by_index = header_cache->Get(999);
    EXPECT_EQ(result_by_index, nullptr);
}

TEST_F(HeaderCacheTest, ContainsHeader) {
    header_cache->Add(header1);
    header_cache->Add(header2);
    
    EXPECT_TRUE(header_cache->Contains(header1->GetHash()));
    EXPECT_TRUE(header_cache->Contains(header2->GetHash()));
    EXPECT_FALSE(header_cache->Contains(header3->GetHash()));
}

TEST_F(HeaderCacheTest, RemoveHeader) {
    header_cache->Add(header1);
    header_cache->Add(header2);
    EXPECT_EQ(header_cache->GetCount(), 2);
    
    bool removed = header_cache->Remove(header1->GetHash());
    EXPECT_TRUE(removed);
    EXPECT_EQ(header_cache->GetCount(), 1);
    EXPECT_FALSE(header_cache->Contains(header1->GetHash()));
    EXPECT_TRUE(header_cache->Contains(header2->GetHash()));
}

TEST_F(HeaderCacheTest, RemoveNonExistentHeader) {
    header_cache->Add(header1);
    
    io::UInt256 non_existent = io::UInt256::Parse("9999999999999999999999999999999999999999999999999999999999999999");
    bool removed = header_cache->Remove(non_existent);
    EXPECT_FALSE(removed);
    EXPECT_EQ(header_cache->GetCount(), 1); // Count unchanged
}

TEST_F(HeaderCacheTest, ClearCache) {
    header_cache->Add(header1);
    header_cache->Add(header2);
    header_cache->Add(header3);
    EXPECT_EQ(header_cache->GetCount(), 3);
    
    header_cache->Clear();
    EXPECT_EQ(header_cache->GetCount(), 0);
    EXPECT_TRUE(header_cache->IsEmpty());
}

TEST_F(HeaderCacheTest, GetLatestHeader) {
    header_cache->Add(header1); // index 0
    header_cache->Add(header2); // index 1
    header_cache->Add(header3); // index 2
    
    auto latest = header_cache->GetLatest();
    EXPECT_NE(latest, nullptr);
    EXPECT_EQ(latest->GetIndex(), 2); // Should be highest index
}

TEST_F(HeaderCacheTest, GetHeaderRange) {
    header_cache->Add(header1);
    header_cache->Add(header2);
    header_cache->Add(header3);
    header_cache->Add(header4);
    
    auto range = header_cache->GetRange(1, 2); // Get headers with index 1 and 2
    EXPECT_EQ(range.size(), 2);
    EXPECT_EQ(range[0]->GetIndex(), 1);
    EXPECT_EQ(range[1]->GetIndex(), 2);
}

TEST_F(HeaderCacheTest, CacheCapacityLimit) {
    // Create a small cache for testing capacity
    auto small_cache = std::make_shared<ledger::HeaderCache>(2);
    
    // Add headers up to capacity
    EXPECT_TRUE(small_cache->Add(header1));
    EXPECT_TRUE(small_cache->Add(header2));
    EXPECT_EQ(small_cache->GetCount(), 2);
    
    // Adding beyond capacity should evict oldest
    EXPECT_TRUE(small_cache->Add(header3));
    EXPECT_EQ(small_cache->GetCount(), 2); // Still at capacity
    
    // header1 should be evicted, header2 and header3 should remain
    EXPECT_FALSE(small_cache->Contains(header1->GetHash()));
    EXPECT_TRUE(small_cache->Contains(header2->GetHash()));
    EXPECT_TRUE(small_cache->Contains(header3->GetHash()));
}

TEST_F(HeaderCacheTest, GetAllHeaders) {
    header_cache->Add(header1);
    header_cache->Add(header2);
    header_cache->Add(header3);
    
    auto all_headers = header_cache->GetAll();
    EXPECT_EQ(all_headers.size(), 3);
    
    // Check that all headers are present
    bool found1 = false, found2 = false, found3 = false;
    for (const auto& header : all_headers) {
        if (header->GetHash() == header1->GetHash()) found1 = true;
        if (header->GetHash() == header2->GetHash()) found2 = true;
        if (header->GetHash() == header3->GetHash()) found3 = true;
    }
    EXPECT_TRUE(found1 && found2 && found3);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_LEDGER_TEST_HEADERCACHE_CPP_H
