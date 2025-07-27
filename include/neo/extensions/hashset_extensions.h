#pragma once

#include <algorithm>
#include <functional>
#include <set>
#include <unordered_set>
#include <vector>

namespace neo::extensions
{
/**
 * @brief Extensions for hash set operations.
 *
 * ## Overview
 * Provides utility methods for set operations, conversions, and manipulations
 * that complement the standard library set functionality.
 *
 * ## API Reference
 * - **Set Operations**: Union, intersection, difference
 * - **Conversions**: To/from vectors, filtering
 * - **Utilities**: Subset checking, element operations
 *
 * ## Usage Examples
 * ```cpp
 * // Union of two sets
 * auto result = HashSetExtensions::Union(set1, set2);
 *
 * // Check if subset
 * bool isSubset = HashSetExtensions::IsSubsetOf(subset, superset);
 *
 * // Convert to vector
 * auto vec = HashSetExtensions::ToVector(hashSet);
 * ```
 */
class HashSetExtensions
{
  public:
    /**
     * @brief Compute union of two unordered sets
     * @tparam T Element type
     * @param left First set
     * @param right Second set
     * @return Union of both sets
     */
    template <typename T>
    static std::unordered_set<T> Union(const std::unordered_set<T>& left, const std::unordered_set<T>& right)
    {
        std::unordered_set<T> result = left;
        result.insert(right.begin(), right.end());
        return result;
    }

    /**
     * @brief Compute intersection of two unordered sets
     * @tparam T Element type
     * @param left First set
     * @param right Second set
     * @return Intersection of both sets
     */
    template <typename T>
    static std::unordered_set<T> Intersection(const std::unordered_set<T>& left, const std::unordered_set<T>& right)
    {
        std::unordered_set<T> result;
        for (const auto& item : left)
        {
            if (right.find(item) != right.end())
            {
                result.insert(item);
            }
        }
        return result;
    }

    /**
     * @brief Compute difference of two unordered sets (left - right)
     * @tparam T Element type
     * @param left First set
     * @param right Second set
     * @return Elements in left but not in right
     */
    template <typename T>
    static std::unordered_set<T> Difference(const std::unordered_set<T>& left, const std::unordered_set<T>& right)
    {
        std::unordered_set<T> result;
        for (const auto& item : left)
        {
            if (right.find(item) == right.end())
            {
                result.insert(item);
            }
        }
        return result;
    }

    /**
     * @brief Compute symmetric difference of two unordered sets
     * @tparam T Element type
     * @param left First set
     * @param right Second set
     * @return Elements in either set but not in both
     */
    template <typename T>
    static std::unordered_set<T> SymmetricDifference(const std::unordered_set<T>& left,
                                                     const std::unordered_set<T>& right)
    {
        auto leftDiff = Difference(left, right);
        auto rightDiff = Difference(right, left);
        return Union(leftDiff, rightDiff);
    }

    /**
     * @brief Check if left is subset of right
     * @tparam T Element type
     * @param left Potential subset
     * @param right Potential superset
     * @return True if left is subset of right
     */
    template <typename T>
    static bool IsSubsetOf(const std::unordered_set<T>& left, const std::unordered_set<T>& right)
    {
        if (left.size() > right.size())
            return false;

        for (const auto& item : left)
        {
            if (right.find(item) == right.end())
                return false;
        }
        return true;
    }

    /**
     * @brief Check if left is superset of right
     * @tparam T Element type
     * @param left Potential superset
     * @param right Potential subset
     * @return True if left is superset of right
     */
    template <typename T>
    static bool IsSupersetOf(const std::unordered_set<T>& left, const std::unordered_set<T>& right)
    {
        return IsSubsetOf(right, left);
    }

    /**
     * @brief Check if sets are disjoint (no common elements)
     * @tparam T Element type
     * @param left First set
     * @param right Second set
     * @return True if sets have no common elements
     */
    template <typename T>
    static bool AreDisjoint(const std::unordered_set<T>& left, const std::unordered_set<T>& right)
    {
        const auto& smaller = (left.size() <= right.size()) ? left : right;
        const auto& larger = (left.size() <= right.size()) ? right : left;

        for (const auto& item : smaller)
        {
            if (larger.find(item) != larger.end())
                return false;
        }
        return true;
    }

    /**
     * @brief Convert unordered set to vector
     * @tparam T Element type
     * @param hashSet Set to convert
     * @return Vector containing all elements
     */
    template <typename T>
    static std::vector<T> ToVector(const std::unordered_set<T>& hashSet)
    {
        return std::vector<T>(hashSet.begin(), hashSet.end());
    }

    /**
     * @brief Convert vector to unordered set
     * @tparam T Element type
     * @param vec Vector to convert
     * @return Unordered set containing unique elements
     */
    template <typename T>
    static std::unordered_set<T> FromVector(const std::vector<T>& vec)
    {
        return std::unordered_set<T>(vec.begin(), vec.end());
    }

    /**
     * @brief Filter set by predicate
     * @tparam T Element type
     * @tparam Predicate Predicate function type
     * @param hashSet Set to filter
     * @param predicate Function to test elements
     * @return New set with elements matching predicate
     */
    template <typename T, typename Predicate>
    static std::unordered_set<T> Where(const std::unordered_set<T>& hashSet, Predicate predicate)
    {
        std::unordered_set<T> result;
        for (const auto& item : hashSet)
        {
            if (predicate(item))
            {
                result.insert(item);
            }
        }
        return result;
    }

    /**
     * @brief Check if any element satisfies predicate
     * @tparam T Element type
     * @tparam Predicate Predicate function type
     * @param hashSet Set to check
     * @param predicate Function to test elements
     * @return True if any element matches predicate
     */
    template <typename T, typename Predicate>
    static bool Any(const std::unordered_set<T>& hashSet, Predicate predicate)
    {
        return std::any_of(hashSet.begin(), hashSet.end(), predicate);
    }

    /**
     * @brief Check if all elements satisfy predicate
     * @tparam T Element type
     * @tparam Predicate Predicate function type
     * @param hashSet Set to check
     * @param predicate Function to test elements
     * @return True if all elements match predicate
     */
    template <typename T, typename Predicate>
    static bool All(const std::unordered_set<T>& hashSet, Predicate predicate)
    {
        return std::all_of(hashSet.begin(), hashSet.end(), predicate);
    }

    /**
     * @brief Remove elements matching predicate
     * @tparam T Element type
     * @tparam Predicate Predicate function type
     * @param hashSet Set to modify
     * @param predicate Function to test elements for removal
     * @return Number of elements removed
     */
    template <typename T, typename Predicate>
    static size_t RemoveWhere(std::unordered_set<T>& hashSet, Predicate predicate)
    {
        size_t removed = 0;
        auto it = hashSet.begin();
        while (it != hashSet.end())
        {
            if (predicate(*it))
            {
                it = hashSet.erase(it);
                ++removed;
            }
            else
            {
                ++it;
            }
        }
        return removed;
    }

    /**
     * @brief Add multiple elements to set
     * @tparam T Element type
     * @param hashSet Set to modify
     * @param elements Elements to add
     */
    template <typename T>
    static void AddRange(std::unordered_set<T>& hashSet, const std::vector<T>& elements)
    {
        hashSet.insert(elements.begin(), elements.end());
    }

    /**
     * @brief Add multiple elements from another set
     * @tparam T Element type
     * @param hashSet Set to modify
     * @param other Set to add elements from
     */
    template <typename T>
    static void AddRange(std::unordered_set<T>& hashSet, const std::unordered_set<T>& other)
    {
        hashSet.insert(other.begin(), other.end());
    }
};
}  // namespace neo::extensions
