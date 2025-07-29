#include <algorithm>
#include <gtest/gtest.h>
#include <list>
#include <memory>
#include <neo/extensions/collection_extensions.h>
#include <set>
#include <string>
#include <vector>

using namespace neo::extensions;

class CollectionExtensionsTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        // Initialize test data
        numbers_ = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        strings_ = {"apple", "banana", "cherry", "date", "elderberry"};
        emptyNumbers_ = {};
        duplicateNumbers_ = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    }

    void TearDown() override
    {
        // Clean up test environment
    }

    std::vector<int> numbers_;
    std::vector<std::string> strings_;
    std::vector<int> emptyNumbers_;
    std::vector<int> duplicateNumbers_;
};

TEST_F(CollectionExtensionsTest, Where_FilterNumbers)
{
    // Test filtering even numbers
    auto evens = CollectionExtensions::Where(numbers_, [](int x) { return x % 2 == 0; });
    std::vector<int> expectedEvens = {2, 4, 6, 8, 10};
    EXPECT_EQ(evens, expectedEvens);

    // Test filtering odd numbers
    auto odds = CollectionExtensions::Where(numbers_, [](int x) { return x % 2 == 1; });
    std::vector<int> expectedOdds = {1, 3, 5, 7, 9};
    EXPECT_EQ(odds, expectedOdds);

    // Test filtering with no matches
    auto negatives = CollectionExtensions::Where(numbers_, [](int x) { return x < 0; });
    EXPECT_TRUE(negatives.empty());
}

TEST_F(CollectionExtensionsTest, Where_FilterStrings)
{
    // Test filtering strings by length
    auto longStrings = CollectionExtensions::Where(strings_, [](const std::string& s) { return s.length() > 5; });
    std::vector<std::string> expectedLong = {"banana", "cherry", "elderberry"};
    EXPECT_EQ(longStrings, expectedLong);

    // Test empty container
    std::vector<std::string> emptyStrings;
    auto filtered = CollectionExtensions::Where(emptyStrings, [](const std::string& s) { return true; });
    EXPECT_TRUE(filtered.empty());
}

TEST_F(CollectionExtensionsTest, Select_TransformNumbers)
{
    // Test transforming numbers to their squares
    auto squares = CollectionExtensions::Select(numbers_, [](int x) { return x * x; });
    std::vector<int> expectedSquares = {1, 4, 9, 16, 25, 36, 49, 64, 81, 100};
    EXPECT_EQ(squares, expectedSquares);

    // Test transforming to different type
    auto numberStrings = CollectionExtensions::Select(numbers_, [](int x) { return std::to_string(x); });
    std::vector<std::string> expectedStrings = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
    EXPECT_EQ(numberStrings, expectedStrings);
}

TEST_F(CollectionExtensionsTest, Any_PredicateTests)
{
    // Test Any with various predicates
    EXPECT_TRUE(CollectionExtensions::Any(numbers_, [](int x) { return x > 5; }));
    EXPECT_FALSE(CollectionExtensions::Any(numbers_, [](int x) { return x > 15; }));
    EXPECT_TRUE(CollectionExtensions::Any(numbers_, [](int x) { return x == 1; }));

    // Test with empty container
    EXPECT_FALSE(CollectionExtensions::Any(emptyNumbers_, [](int x) { return true; }));

    // Test with strings
    EXPECT_TRUE(CollectionExtensions::Any(strings_, [](const std::string& s) { return s.length() > 8; }));
}

TEST_F(CollectionExtensionsTest, All_PredicateTests)
{
    // Test All with various predicates
    EXPECT_TRUE(CollectionExtensions::All(numbers_, [](int x) { return x > 0; }));
    EXPECT_FALSE(CollectionExtensions::All(numbers_, [](int x) { return x > 5; }));
    EXPECT_TRUE(CollectionExtensions::All(numbers_, [](int x) { return x <= 10; }));

    // Test with empty container (vacuous truth)
    EXPECT_TRUE(CollectionExtensions::All(emptyNumbers_, [](int x) { return false; }));

    // Test with strings
    EXPECT_TRUE(CollectionExtensions::All(strings_, [](const std::string& s) { return !s.empty(); }));
}

TEST_F(CollectionExtensionsTest, Count_PredicateTests)
{
    // Test Count with various predicates
    EXPECT_EQ(CollectionExtensions::Count(numbers_, [](int x) { return x % 2 == 0; }), 5u);
    EXPECT_EQ(CollectionExtensions::Count(numbers_, [](int x) { return x > 5; }), 5u);
    EXPECT_EQ(CollectionExtensions::Count(numbers_, [](int x) { return x > 15; }), 0u);

    // Test with empty container
    EXPECT_EQ(CollectionExtensions::Count(emptyNumbers_, [](int x) { return true; }), 0u);

    // Test with strings
    EXPECT_EQ(CollectionExtensions::Count(strings_, [](const std::string& s) { return s.length() == 5; }), 2u);
}

TEST_F(CollectionExtensionsTest, ToVector_Conversion)
{
    // Test converting set to vector
    std::set<int> numberSet = {3, 1, 4, 1, 5, 9, 2, 6};
    auto vectorFromSet = CollectionExtensions::ToVector(numberSet);
    std::vector<int> expected = {1, 2, 3, 4, 5, 6, 9};  // Set is sorted and unique
    EXPECT_EQ(vectorFromSet, expected);

    // Test converting list to vector
    std::list<std::string> stringList = {"a", "b", "c"};
    auto vectorFromList = CollectionExtensions::ToVector(stringList);
    std::vector<std::string> expectedStrings = {"a", "b", "c"};
    EXPECT_EQ(vectorFromList, expectedStrings);
}

TEST_F(CollectionExtensionsTest, ToSet_Conversion)
{
    // Test converting vector to set (removes duplicates and sorts)
    auto setFromVector = CollectionExtensions::ToSet(duplicateNumbers_);
    std::set<int> expected = {1, 2, 3, 4};
    EXPECT_EQ(setFromVector, expected);

    // Test converting strings
    auto stringSet = CollectionExtensions::ToSet(strings_);
    std::set<std::string> expectedSet = {"apple", "banana", "cherry", "date", "elderberry"};
    EXPECT_EQ(stringSet, expectedSet);
}

TEST_F(CollectionExtensionsTest, AddRange_Functionality)
{
    // Test adding range to vector
    std::vector<int> target = {1, 2, 3};
    std::vector<int> source = {4, 5, 6};
    CollectionExtensions::AddRange(target, source);
    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    EXPECT_EQ(target, expected);

    // Test adding empty range
    std::vector<int> emptySource;
    size_t originalSize = target.size();
    CollectionExtensions::AddRange(target, emptySource);
    EXPECT_EQ(target.size(), originalSize);
}

TEST_F(CollectionExtensionsTest, RemoveWhere_Functionality)
{
    // Test removing even numbers
    auto testNumbers = numbers_;
    size_t removed = CollectionExtensions::RemoveWhere(testNumbers, [](int x) { return x % 2 == 0; });
    EXPECT_EQ(removed, 5u);
    std::vector<int> expectedOdds = {1, 3, 5, 7, 9};
    EXPECT_EQ(testNumbers, expectedOdds);

    // Test removing all elements
    testNumbers = numbers_;
    removed = CollectionExtensions::RemoveWhere(testNumbers, [](int x) { return true; });
    EXPECT_EQ(removed, 10u);
    EXPECT_TRUE(testNumbers.empty());

    // Test removing no elements
    testNumbers = numbers_;
    removed = CollectionExtensions::RemoveWhere(testNumbers, [](int x) { return x > 15; });
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(testNumbers, numbers_);
}

TEST_F(CollectionExtensionsTest, FirstOrDefault_Search)
{
    // Test finding first element that satisfies predicate
    auto it = CollectionExtensions::FirstOrDefault(numbers_, [](int x) { return x > 5; });
    EXPECT_NE(it, numbers_.end());
    EXPECT_EQ(*it, 6);

    // Test finding no element
    auto notFound = CollectionExtensions::FirstOrDefault(numbers_, [](int x) { return x > 15; });
    EXPECT_EQ(notFound, numbers_.end());

    // Test with strings
    auto stringIt = CollectionExtensions::FirstOrDefault(strings_, [](const std::string& s) { return s.length() > 6; });
    EXPECT_NE(stringIt, strings_.end());
    EXPECT_EQ(*stringIt, "elderberry");
}

TEST_F(CollectionExtensionsTest, Contains_Search)
{
    // Test contains functionality
    EXPECT_TRUE(CollectionExtensions::Contains(numbers_, 5));
    EXPECT_FALSE(CollectionExtensions::Contains(numbers_, 15));

    // Test with strings
    EXPECT_TRUE(CollectionExtensions::Contains(strings_, "apple"));
    EXPECT_FALSE(CollectionExtensions::Contains(strings_, "grape"));

    // Test with empty container
    EXPECT_FALSE(CollectionExtensions::Contains(emptyNumbers_, 1));
}

TEST_F(CollectionExtensionsTest, WorksWithDifferentContainerTypes)
{
    // Test that methods work with different STL containers
    std::list<int> numberList = {1, 2, 3, 4, 5};

    // Test Any with list
    EXPECT_TRUE(CollectionExtensions::Any(numberList, [](int x) { return x > 3; }));

    // Test Count with list
    EXPECT_EQ(CollectionExtensions::Count(numberList, [](int x) { return x % 2 == 0; }), 2u);

    // Test Contains with list
    EXPECT_TRUE(CollectionExtensions::Contains(numberList, 3));

    // Test Where with list
    auto filteredList = CollectionExtensions::Where(numberList, [](int x) { return x > 3; });
    EXPECT_EQ(filteredList.size(), 2u);
}
