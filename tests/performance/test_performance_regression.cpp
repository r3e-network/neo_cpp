#include <gtest/gtest.h>
#include <chrono>
#include <vector>

class PerformanceTest : public ::testing::Test {
protected:
    template<typename Func>
    double MeasureTime(Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> diff = end - start;
        return diff.count();
    }
};

TEST_F(PerformanceTest, VectorOperations) {
    std::vector<int> vec;
    double time = MeasureTime([&vec]() {
        for (int i = 0; i < 10000; ++i) {
            vec.push_back(i);
        }
    });
    
    EXPECT_LT(time, 100.0); // Should complete in less than 100ms
}

TEST_F(PerformanceTest, MemoryAllocation) {
    double time = MeasureTime([]() {
        for (int i = 0; i < 1000; ++i) {
            auto* ptr = new int[100];
            delete[] ptr;
        }
    });
    
    EXPECT_LT(time, 50.0); // Should complete in less than 50ms
}

TEST_F(PerformanceTest, StringOperations) {
    std::string str;
    double time = MeasureTime([&str]() {
        for (int i = 0; i < 1000; ++i) {
            str += "test";
        }
    });
    
    EXPECT_LT(time, 10.0); // Should complete in less than 10ms
}
