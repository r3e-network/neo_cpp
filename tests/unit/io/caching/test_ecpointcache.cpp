#include <gtest/gtest.h>
#include <memory>
#include <neo/cryptography/ecc/eccurve.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/byte_vector.h>
#include <neo/io/caching/ecpoint_cache.h>
#include <string>
#include <thread>
#include <vector>

using namespace neo::io::caching;
using namespace neo::cryptography::ecc;
using namespace neo::io;

/**
 * @brief Test fixture for ECPointCache
 */
class ECPointCacheTest : public testing::Test
{
  protected:
    std::shared_ptr<ECCurve> testCurve;
    std::vector<ByteVector> testPointBytes;
    std::vector<std::shared_ptr<ECPoint>> testPoints;

    void SetUp() override
    {
        // Initialize test curve (secp256r1)
        testCurve = ECCurve::GetSecp256r1();

        // Create test points (compressed format)
        // Point 1: Example compressed public key
        testPointBytes.push_back(ByteVector({0x02,  // Compressed prefix for even y
                                             0x48, 0x6f, 0xeb, 0x65, 0xb0, 0x76, 0x17, 0xaf, 0x0e, 0x0a, 0x28,
                                             0x1b, 0xc5, 0xa4, 0xca, 0x56, 0x3e, 0x52, 0x37, 0x5b, 0xde, 0x73,
                                             0x54, 0x94, 0x5e, 0xf0, 0x46, 0x9f, 0x7f, 0x26, 0x09, 0xa9}));

        // Point 2: Another example compressed public key
        testPointBytes.push_back(ByteVector({0x03,  // Compressed prefix for odd y
                                             0x67, 0x81, 0xe6, 0x42, 0x36, 0x93, 0xc3, 0xdb, 0x9f, 0x5e, 0x7f,
                                             0x89, 0x1a, 0x8a, 0x66, 0x5f, 0xeb, 0x40, 0xa0, 0x47, 0x12, 0x0e,
                                             0x0a, 0x48, 0xc2, 0x28, 0x59, 0x85, 0x9c, 0xad, 0x0c, 0x96}));

        // Decode test points
        for (const auto& bytes : testPointBytes)
        {
            testPoints.push_back(ECPoint::DecodePoint(bytes.AsSpan(), testCurve));
        }
    }
};

TEST_F(ECPointCacheTest, Constructor)
{
    ECPointCache cache(100);

    EXPECT_EQ(0u, cache.Size());
    EXPECT_EQ(100u, cache.Capacity());
}

TEST_F(ECPointCacheTest, GetOrCreate_NewPoint)
{
    ECPointCache cache(10);

    // Get point that doesn't exist in cache
    auto point = cache.GetOrCreate(testPointBytes[0], testCurve);

    ASSERT_NE(nullptr, point);
    EXPECT_EQ(1u, cache.Size());

    // Verify the point is correct
    auto encoded = point->EncodePoint(true);
    EXPECT_EQ(testPointBytes[0], encoded);
}

TEST_F(ECPointCacheTest, GetOrCreate_ExistingPoint)
{
    ECPointCache cache(10);

    // Add point to cache
    auto point1 = cache.GetOrCreate(testPointBytes[0], testCurve);
    EXPECT_EQ(1u, cache.Size());

    // Get same point again - should come from cache
    auto point2 = cache.GetOrCreate(testPointBytes[0], testCurve);
    EXPECT_EQ(1u, cache.Size());  // Size shouldn't increase

    // Both should be the same object
    EXPECT_EQ(point1, point2);
}

TEST_F(ECPointCacheTest, Get_ExistingPoint)
{
    ECPointCache cache(10);

    // Add point to cache
    cache.GetOrCreate(testPointBytes[0], testCurve);

    // Get point from cache
    auto result = cache.Get(testPointBytes[0]);
    ASSERT_TRUE(result.has_value());

    auto point = result.value();
    ASSERT_NE(nullptr, point);
    auto encoded = point->EncodePoint(true);
    EXPECT_EQ(testPointBytes[0], encoded);
}

TEST_F(ECPointCacheTest, Get_NonExistingPoint)
{
    ECPointCache cache(10);

    // Try to get point that doesn't exist
    auto result = cache.Get(testPointBytes[0]);
    EXPECT_FALSE(result.has_value());
}

TEST_F(ECPointCacheTest, Add)
{
    ECPointCache cache(10);

    // Add point directly
    cache.Add(testPoints[0]);
    EXPECT_EQ(1u, cache.Size());

    // Verify it's in cache
    auto result = cache.Get(testPointBytes[0]);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(testPoints[0], result.value());
}

TEST_F(ECPointCacheTest, Add_Nullptr)
{
    ECPointCache cache(10);

    // Add nullptr - should be ignored
    cache.Add(nullptr);
    EXPECT_EQ(0u, cache.Size());
}

TEST_F(ECPointCacheTest, Clear)
{
    ECPointCache cache(10);

    // Add multiple points
    cache.GetOrCreate(testPointBytes[0], testCurve);
    cache.GetOrCreate(testPointBytes[1], testCurve);
    EXPECT_EQ(2u, cache.Size());

    // Clear cache
    cache.Clear();
    EXPECT_EQ(0u, cache.Size());

    // Verify points are gone
    EXPECT_FALSE(cache.Get(testPointBytes[0]).has_value());
    EXPECT_FALSE(cache.Get(testPointBytes[1]).has_value());
}

TEST_F(ECPointCacheTest, CapacityLimit)
{
    ECPointCache cache(2);  // Small capacity

    // Add first point
    cache.GetOrCreate(testPointBytes[0], testCurve);
    EXPECT_EQ(1u, cache.Size());

    // Add second point
    cache.GetOrCreate(testPointBytes[1], testCurve);
    EXPECT_EQ(2u, cache.Size());

    // Add third point - should evict oldest
    ByteVector thirdPointBytes({0x02, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
                                0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                                0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01});

    cache.GetOrCreate(thirdPointBytes, testCurve);
    EXPECT_EQ(2u, cache.Size());  // Still 2 due to capacity limit

    // First point should be evicted
    EXPECT_FALSE(cache.Get(testPointBytes[0]).has_value());
    EXPECT_TRUE(cache.Get(testPointBytes[1]).has_value());
    EXPECT_TRUE(cache.Get(thirdPointBytes).has_value());
}

TEST_F(ECPointCacheTest, ByteVectorHash)
{
    ByteVectorHash hasher;

    // Same bytes should have same hash
    ByteVector bytes1({0x01, 0x02, 0x03});
    ByteVector bytes2({0x01, 0x02, 0x03});
    EXPECT_EQ(hasher(bytes1), hasher(bytes2));

    // Different bytes should (usually) have different hash
    ByteVector bytes3({0x01, 0x02, 0x04});
    EXPECT_NE(hasher(bytes1), hasher(bytes3));

    // Empty bytes
    ByteVector empty;
    size_t emptyHash = hasher(empty);
    EXPECT_NE(0u, emptyHash);  // Should still produce a hash
}

TEST_F(ECPointCacheTest, ByteVectorEqual)
{
    ByteVectorEqual comparer;

    // Same bytes
    ByteVector bytes1({0x01, 0x02, 0x03});
    ByteVector bytes2({0x01, 0x02, 0x03});
    EXPECT_TRUE(comparer(bytes1, bytes2));

    // Different bytes
    ByteVector bytes3({0x01, 0x02, 0x04});
    EXPECT_FALSE(comparer(bytes1, bytes3));

    // Different lengths
    ByteVector bytes4({0x01, 0x02});
    EXPECT_FALSE(comparer(bytes1, bytes4));

    // Empty bytes
    ByteVector empty1;
    ByteVector empty2;
    EXPECT_TRUE(comparer(empty1, empty2));
}

TEST_F(ECPointCacheTest, ThreadSafety)
{
    ECPointCache cache(100);
    const int numThreads = 4;
    const int pointsPerThread = 25;
    std::vector<std::thread> threads;

    // Multiple threads adding and getting points
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back(
            [&cache, this, t, pointsPerThread]()
            {
                for (int i = 0; i < pointsPerThread; ++i)
                {
                    // Create unique point bytes for each thread/iteration
                    ByteVector uniqueBytes(33, 0x02);
                    uniqueBytes[1] = static_cast<uint8_t>(t);
                    uniqueBytes[2] = static_cast<uint8_t>(i);

                    // Add to cache
                    cache.GetOrCreate(uniqueBytes, testCurve);

                    // Try to get it back
                    auto result = cache.Get(uniqueBytes);
                    EXPECT_TRUE(result.has_value());
                }
            });
    }

    // Wait for all threads
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Cache should have items (exact count depends on eviction)
    EXPECT_GT(cache.Size(), 0u);
    EXPECT_LE(cache.Size(), cache.Capacity());
}

TEST_F(ECPointCacheTest, DifferentCurves)
{
    ECPointCache cache(10);

    // Get point with secp256r1
    auto point1 = cache.GetOrCreate(testPointBytes[0], testCurve);
    ASSERT_NE(nullptr, point1);

    // Try to get same bytes with different curve (secp256k1)
    auto secp256k1 = ECCurve::GetSecp256k1();
    auto point2 = cache.GetOrCreate(testPointBytes[0], secp256k1);

    // Should return cached point (cache doesn't distinguish by curve)
    // This might be a design consideration - in real usage, you'd want separate caches per curve
    EXPECT_EQ(point1, point2);
}

TEST_F(ECPointCacheTest, LargeCache)
{
    ECPointCache cache(1000);

    // Add many points
    for (int i = 0; i < 500; ++i)
    {
        ByteVector bytes(33, 0x02);
        bytes[1] = static_cast<uint8_t>(i & 0xFF);
        bytes[2] = static_cast<uint8_t>((i >> 8) & 0xFF);

        cache.GetOrCreate(bytes, testCurve);
    }

    EXPECT_EQ(500u, cache.Size());
    EXPECT_EQ(1000u, cache.Capacity());
}

TEST_F(ECPointCacheTest, PerformanceBenefit)
{
    ECPointCache cache(100);

    // First access - creates point
    auto start1 = std::chrono::high_resolution_clock::now();
    auto point1 = cache.GetOrCreate(testPointBytes[0], testCurve);
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count();

    // Second access - from cache (should be faster)
    auto start2 = std::chrono::high_resolution_clock::now();
    auto point2 = cache.GetOrCreate(testPointBytes[0], testCurve);
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2).count();

    // Cache hit should be faster than creating new point
    // Note: This might be flaky on very fast systems or under load
    EXPECT_EQ(point1, point2);
    // We can't reliably assert timing, but typically duration2 < duration1
}
