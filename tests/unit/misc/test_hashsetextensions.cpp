#include <gtest/gtest.h>
#include <neo/extensions/hashset_extensions.h>
#include <string>
#include <unordered_set>
#include <vector>

using namespace neo::extensions;

/**
 * @brief Test fixture for HashSetExtensions
 */
class HashSetExtensionsTest : public testing::Test
{
  protected:
    std::unordered_set<int> set1;
    std::unordered_set<int> set2;
    std::unordered_set<int> empty_set;

    void SetUp() override
    {
        set1 = {1, 2, 3, 4, 5};
        set2 = {4, 5, 6, 7, 8};
        empty_set.clear();
    }
};

TEST_F(HashSetExtensionsTest, Union)
{
    auto result = HashSetExtensions::Union(set1, set2);

    EXPECT_EQ(8, result.size());
    for (int i = 1; i <= 8; ++i)
    {
        EXPECT_TRUE(result.find(i) != result.end());
    }

    // Union with empty set
    auto result2 = HashSetExtensions::Union(set1, empty_set);
    EXPECT_EQ(set1, result2);
}

TEST_F(HashSetExtensionsTest, Intersection)
{
    auto result = HashSetExtensions::Intersection(set1, set2);

    EXPECT_EQ(2, result.size());
    EXPECT_TRUE(result.find(4) != result.end());
    EXPECT_TRUE(result.find(5) != result.end());

    // Intersection with empty set
    auto result2 = HashSetExtensions::Intersection(set1, empty_set);
    EXPECT_TRUE(result2.empty());
}

TEST_F(HashSetExtensionsTest, Difference)
{
    auto result = HashSetExtensions::Difference(set1, set2);

    EXPECT_EQ(3, result.size());
    EXPECT_TRUE(result.find(1) != result.end());
    EXPECT_TRUE(result.find(2) != result.end());
    EXPECT_TRUE(result.find(3) != result.end());

    // Difference with empty set
    auto result2 = HashSetExtensions::Difference(set1, empty_set);
    EXPECT_EQ(set1, result2);
}

TEST_F(HashSetExtensionsTest, SymmetricDifference)
{
    auto result = HashSetExtensions::SymmetricDifference(set1, set2);

    EXPECT_EQ(6, result.size());
    EXPECT_TRUE(result.find(1) != result.end());
    EXPECT_TRUE(result.find(2) != result.end());
    EXPECT_TRUE(result.find(3) != result.end());
    EXPECT_TRUE(result.find(6) != result.end());
    EXPECT_TRUE(result.find(7) != result.end());
    EXPECT_TRUE(result.find(8) != result.end());
    EXPECT_FALSE(result.find(4) != result.end());
    EXPECT_FALSE(result.find(5) != result.end());
}

TEST_F(HashSetExtensionsTest, IsSubsetOf)
{
    std::unordered_set<int> subset = {2, 3, 4};

    EXPECT_TRUE(HashSetExtensions::IsSubsetOf(subset, set1));
    EXPECT_FALSE(HashSetExtensions::IsSubsetOf(set1, subset));
    EXPECT_TRUE(HashSetExtensions::IsSubsetOf(empty_set, set1));
    EXPECT_TRUE(HashSetExtensions::IsSubsetOf(set1, set1));
}

TEST_F(HashSetExtensionsTest, IsSupersetOf)
{
    std::unordered_set<int> subset = {2, 3, 4};

    EXPECT_TRUE(HashSetExtensions::IsSupersetOf(set1, subset));
    EXPECT_FALSE(HashSetExtensions::IsSupersetOf(subset, set1));
    EXPECT_TRUE(HashSetExtensions::IsSupersetOf(set1, empty_set));
    EXPECT_TRUE(HashSetExtensions::IsSupersetOf(set1, set1));
}

TEST_F(HashSetExtensionsTest, AreDisjoint)
{
    std::unordered_set<int> disjoint_set = {10, 11, 12};

    EXPECT_FALSE(HashSetExtensions::AreDisjoint(set1, set2));
    EXPECT_TRUE(HashSetExtensions::AreDisjoint(set1, disjoint_set));
    EXPECT_TRUE(HashSetExtensions::AreDisjoint(empty_set, set1));
}

TEST_F(HashSetExtensionsTest, ToVector)
{
    auto vec = HashSetExtensions::ToVector(set1);

    EXPECT_EQ(set1.size(), vec.size());
    for (const auto& elem : vec)
    {
        EXPECT_TRUE(set1.find(elem) != set1.end());
    }
}

TEST_F(HashSetExtensionsTest, FromVector)
{
    std::vector<int> vec = {1, 2, 3, 3, 4, 4, 5};
    auto result = HashSetExtensions::FromVector(vec);

    EXPECT_EQ(5, result.size());
    for (int i = 1; i <= 5; ++i)
    {
        EXPECT_TRUE(result.find(i) != result.end());
    }
}

TEST_F(HashSetExtensionsTest, Where)
{
    auto result = HashSetExtensions::Where(set1, [](int x) { return x % 2 == 0; });

    EXPECT_EQ(2, result.size());
    EXPECT_TRUE(result.find(2) != result.end());
    EXPECT_TRUE(result.find(4) != result.end());
}

TEST_F(HashSetExtensionsTest, Any)
{
    EXPECT_TRUE(HashSetExtensions::Any(set1, [](int x) { return x > 3; }));
    EXPECT_FALSE(HashSetExtensions::Any(set1, [](int x) { return x > 10; }));
    EXPECT_FALSE(HashSetExtensions::Any(empty_set, [](int x) { return true; }));
}

TEST_F(HashSetExtensionsTest, All)
{
    EXPECT_TRUE(HashSetExtensions::All(set1, [](int x) { return x > 0; }));
    EXPECT_FALSE(HashSetExtensions::All(set1, [](int x) { return x > 3; }));
    EXPECT_TRUE(HashSetExtensions::All(empty_set, [](int x) { return false; }));
}

TEST_F(HashSetExtensionsTest, RemoveWhere)
{
    auto copy_set = set1;
    size_t removed = HashSetExtensions::RemoveWhere(copy_set, [](int x) { return x % 2 == 0; });

    EXPECT_EQ(2, removed);
    EXPECT_EQ(3, copy_set.size());
    EXPECT_TRUE(copy_set.find(1) != copy_set.end());
    EXPECT_TRUE(copy_set.find(3) != copy_set.end());
    EXPECT_TRUE(copy_set.find(5) != copy_set.end());
}

TEST_F(HashSetExtensionsTest, AddRange)
{
    std::unordered_set<int> target;
    std::vector<int> elements = {1, 2, 3};

    HashSetExtensions::AddRange(target, elements);
    EXPECT_EQ(3, target.size());

    HashSetExtensions::AddRange(target, set2);
    EXPECT_EQ(8, target.size());
}
