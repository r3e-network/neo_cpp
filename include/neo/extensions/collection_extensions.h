#pragma once

#include <vector>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <functional>

namespace neo::extensions 
{
    /**
     * @brief Extensions for STL collections.
     * 
     * ## Overview
     * Provides utility methods for common collection operations that extend
     * the functionality of standard STL containers.
     * 
     * ## API Reference
     * - **Filtering**: Where, Select operations
     * - **Aggregation**: Any, All, Count operations
     * - **Conversion**: ToVector, ToSet operations
     * - **Manipulation**: AddRange, RemoveWhere operations
     * 
     * ## Usage Examples
     * ```cpp
     * std::vector<int> numbers = {1, 2, 3, 4, 5};
     * auto evens = CollectionExtensions::Where(numbers, [](int x) { return x % 2 == 0; });
     * bool hasPositive = CollectionExtensions::Any(numbers, [](int x) { return x > 0; });
     * ```
     */
    class CollectionExtensions 
    { 
    public:
        /**
         * @brief Filter elements by predicate
         * @tparam Container The container type
         * @tparam Predicate The predicate function type
         * @param container The input container
         * @param predicate The filtering predicate
         * @return Container with filtered elements
         */
        template<typename Container, typename Predicate>
        static Container Where(const Container& container, Predicate predicate)
        {
            Container result;
            std::copy_if(container.begin(), container.end(), 
                        std::back_inserter(result), predicate);
            return result;
        }

        /**
         * @brief Transform elements using selector function
         * @tparam Container The input container type
         * @tparam Selector The selector function type
         * @tparam ResultType The result element type
         * @param container The input container
         * @param selector The transformation function
         * @return Vector with transformed elements
         */
        template<typename Container, typename Selector, typename ResultType = std::invoke_result_t<Selector, typename Container::value_type>>
        static std::vector<ResultType> Select(const Container& container, Selector selector)
        {
            std::vector<ResultType> result;
            result.reserve(container.size());
            std::transform(container.begin(), container.end(), 
                          std::back_inserter(result), selector);
            return result;
        }

        /**
         * @brief Check if any element satisfies predicate
         * @tparam Container The container type
         * @tparam Predicate The predicate function type
         * @param container The input container
         * @param predicate The test predicate
         * @return True if any element satisfies predicate
         */
        template<typename Container, typename Predicate>
        static bool Any(const Container& container, Predicate predicate)
        {
            return std::any_of(container.begin(), container.end(), predicate);
        }

        /**
         * @brief Check if all elements satisfy predicate
         * @tparam Container The container type
         * @tparam Predicate The predicate function type
         * @param container The input container
         * @param predicate The test predicate
         * @return True if all elements satisfy predicate
         */
        template<typename Container, typename Predicate>
        static bool All(const Container& container, Predicate predicate)
        {
            return std::all_of(container.begin(), container.end(), predicate);
        }

        /**
         * @brief Count elements that satisfy predicate
         * @tparam Container The container type
         * @tparam Predicate The predicate function type
         * @param container The input container
         * @param predicate The counting predicate
         * @return Number of elements satisfying predicate
         */
        template<typename Container, typename Predicate>
        static size_t Count(const Container& container, Predicate predicate)
        {
            return std::count_if(container.begin(), container.end(), predicate);
        }

        /**
         * @brief Convert container to vector
         * @tparam Container The input container type
         * @param container The input container
         * @return Vector with same elements
         */
        template<typename Container>
        static std::vector<typename Container::value_type> ToVector(const Container& container)
        {
            return std::vector<typename Container::value_type>(container.begin(), container.end());
        }

        /**
         * @brief Convert container to set
         * @tparam Container The input container type
         * @param container The input container
         * @return Set with unique elements
         */
        template<typename Container>
        static std::set<typename Container::value_type> ToSet(const Container& container)
        {
            return std::set<typename Container::value_type>(container.begin(), container.end());
        }

        /**
         * @brief Add range of elements to container
         * @tparam Container The target container type
         * @tparam Range The source range type
         * @param container The target container
         * @param range The source range
         */
        template<typename Container, typename Range>
        static void AddRange(Container& container, const Range& range)
        {
            container.insert(container.end(), range.begin(), range.end());
        }

        /**
         * @brief Remove elements that satisfy predicate
         * @tparam Container The container type
         * @tparam Predicate The predicate function type
         * @param container The container to modify
         * @param predicate The removal predicate
         * @return Number of elements removed
         */
        template<typename Container, typename Predicate>
        static size_t RemoveWhere(Container& container, Predicate predicate)
        {
            auto originalSize = container.size();
            container.erase(
                std::remove_if(container.begin(), container.end(), predicate),
                container.end()
            );
            return originalSize - container.size();
        }

        /**
         * @brief Find first element that satisfies predicate
         * @tparam Container The container type
         * @tparam Predicate The predicate function type
         * @param container The input container
         * @param predicate The search predicate
         * @return Iterator to found element or end()
         */
        template<typename Container, typename Predicate>
        static typename Container::const_iterator FirstOrDefault(const Container& container, Predicate predicate)
        {
            return std::find_if(container.begin(), container.end(), predicate);
        }

        /**
         * @brief Check if container contains element
         * @tparam Container The container type
         * @tparam T The element type
         * @param container The input container
         * @param element The element to find
         * @return True if element is found
         */
        template<typename Container, typename T>
        static bool Contains(const Container& container, const T& element)
        {
            return std::find(container.begin(), container.end(), element) != container.end();
        }

        /**
         * @brief Get distinct elements from container
         * @tparam Container The container type
         * @param container The input container
         * @return Vector with unique elements
         */
        template<typename Container>
        static std::vector<typename Container::value_type> Distinct(const Container& container)
        {
            std::set<typename Container::value_type> uniqueSet(container.begin(), container.end());
            return std::vector<typename Container::value_type>(uniqueSet.begin(), uniqueSet.end());
        }
    };
}
