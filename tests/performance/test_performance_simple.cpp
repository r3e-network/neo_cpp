#include <gtest/gtest.h>
#include <chrono>

TEST(PerformanceTest, BasicBenchmark) {
    auto start = std::chrono::high_resolution_clock::now();
    int sum = 0;
    for (int i = 0; i < 1000000; ++i) {
        sum += i;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 100); // Should complete in less than 100ms
}

TEST(PerformanceTest, MemoryAllocation) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<int> vec;
    for (int i = 0; i < 10000; ++i) {
        vec.push_back(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 50);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
