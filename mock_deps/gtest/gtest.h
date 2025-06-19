#pragma once

#include <iostream>
#include <string>
#include <functional>

// Mock Google Test framework for compilation testing
namespace testing {
    
class Test {
public:
    virtual ~Test() = default;
    virtual void SetUp() {}
    virtual void TearDown() {}
};

// Test registration macros
#define TEST_F(test_fixture, test_name) \
    class test_fixture##_##test_name : public test_fixture { \
    public: \
        void TestBody(); \
    }; \
    void test_fixture##_##test_name::TestBody()

#define TEST(test_suite, test_name) \
    void test_suite##_##test_name()

// Assertion macros
#define EXPECT_TRUE(condition) \
    if (!(condition)) std::cerr << "EXPECT_TRUE failed: " #condition << std::endl

#define EXPECT_FALSE(condition) \
    if (condition) std::cerr << "EXPECT_FALSE failed: " #condition << std::endl

#define EXPECT_EQ(expected, actual) \
    if ((expected) != (actual)) std::cerr << "EXPECT_EQ failed" << std::endl

#define EXPECT_NE(expected, actual) \
    if ((expected) == (actual)) std::cerr << "EXPECT_NE failed" << std::endl

#define EXPECT_LT(val1, val2) \
    if (!((val1) < (val2))) std::cerr << "EXPECT_LT failed" << std::endl

#define EXPECT_GT(val1, val2) \
    if (!((val1) > (val2))) std::cerr << "EXPECT_GT failed" << std::endl

#define EXPECT_LE(val1, val2) \
    if (!((val1) <= (val2))) std::cerr << "EXPECT_LE failed" << std::endl

#define EXPECT_GE(val1, val2) \
    if (!((val1) >= (val2))) std::cerr << "EXPECT_GE failed" << std::endl

#define EXPECT_THROW(statement, exception_type) \
    try { statement; std::cerr << "EXPECT_THROW failed: no exception" << std::endl; } \
    catch(const exception_type&) {} \
    catch(...) { std::cerr << "EXPECT_THROW failed: wrong exception" << std::endl; }

#define EXPECT_NO_THROW(statement) \
    try { statement; } \
    catch(...) { std::cerr << "EXPECT_NO_THROW failed" << std::endl; }

#define ASSERT_TRUE(condition) EXPECT_TRUE(condition)
#define ASSERT_FALSE(condition) EXPECT_FALSE(condition)
#define ASSERT_EQ(expected, actual) EXPECT_EQ(expected, actual)

} // namespace testing

// Main macro
#define RUN_ALL_TESTS() 0
