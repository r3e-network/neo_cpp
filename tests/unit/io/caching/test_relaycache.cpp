#include <gtest/gtest.h>
#include <neo/io/caching/relay_cache.h>
#include <neo/io/uint256.h>

using namespace neo::io::caching;

class RelayCacheTest : public ::testing::Test
{
  protected:
    RelayCache cache_{3};
};

TEST_F(RelayCacheTest, AddAndContains)
{
    io::UInt256 hash1 = io::UInt256::Parse("0x01");
    io::UInt256 hash2 = io::UInt256::Parse("0x02");

    EXPECT_FALSE(cache_.Contains(hash1));
    cache_.Add(hash1);
    EXPECT_TRUE(cache_.Contains(hash1));

    cache_.Add(hash2);
    EXPECT_TRUE(cache_.Contains(hash2));
}

TEST_F(RelayCacheTest, CapacityEviction)
{
    cache_ = RelayCache(2);
    auto h1 = io::UInt256::Parse("0x01");
    auto h2 = io::UInt256::Parse("0x02");
    auto h3 = io::UInt256::Parse("0x03");

    cache_.Add(h1);
    cache_.Add(h2);
    cache_.Add(h3);

    EXPECT_FALSE(cache_.Contains(h1));
    EXPECT_TRUE(cache_.Contains(h2));
    EXPECT_TRUE(cache_.Contains(h3));
}

TEST_F(RelayCacheTest, Clear)
{
    cache_.Add(io::UInt256::Parse("0x01"));
    cache_.Add(io::UInt256::Parse("0x02"));
    cache_.Clear();
    EXPECT_FALSE(cache_.Contains(io::UInt256::Parse("0x01")));
}

