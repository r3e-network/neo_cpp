#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>

namespace neo::vm
{
    // Forward declarations
    class StackItem;
    namespace types
    {
        class Buffer;
        class CompoundType;
    }

    /**
     * @brief Represents a reference counter for stack items.
     *
     * This class is used to track references between stack items to prevent memory leaks
     * and circular references.
     */
    class ReferenceCounter
    {
    public:
        /**
         * @brief Constructs a new ReferenceCounter.
         */
        ReferenceCounter() = default;

        /**
         * @brief Increments the reference count.
         */
        void AddReference();

        /**
         * @brief Decrements the reference count.
         */
        void RemoveReference();

        /**
         * @brief Adds a reference from a parent to a child.
         * @param child The child item.
         * @param parent The parent item.
         */
        void AddReference(std::shared_ptr<StackItem> child, std::shared_ptr<StackItem> parent);

        /**
         * @brief Removes a reference from a parent to a child.
         * @param child The child item.
         * @param parent The parent item.
         */
        void RemoveReference(std::shared_ptr<StackItem> child, std::shared_ptr<StackItem> parent);

        /**
         * @brief Adds a stack reference to an item.
         * @param item The item.
         * @param count The number of references to add.
         */
        void AddStackReference(std::shared_ptr<StackItem> item, size_t count = 1);

        /**
         * @brief Removes a stack reference from an item.
         * @param item The item.
         */
        void RemoveStackReference(std::shared_ptr<StackItem> item);

        /**
         * @brief Adds an item to the zero-referred list.
         * @param item The item.
         */
        void AddZeroReferred(std::shared_ptr<StackItem> item);

        /**
         * @brief Checks and processes items that have zero references.
         * @return The current reference count.
         */
        size_t CheckZeroReferred();

        /**
         * @brief Gets the number of references to an item.
         * @param item The item.
         * @return The number of references.
         */
        size_t GetReferenceCount(std::shared_ptr<StackItem> item) const;

        /**
         * @brief Gets the number of stack references to an item.
         * @param item The item.
         * @return The number of stack references.
         */
        size_t GetStackReferences(std::shared_ptr<StackItem> item) const;

        /**
         * @brief Gets the total number of references.
         * @return The total number of references.
         */
        size_t Count() const { return references_count_; }

        /**
         * @brief Checks if an item is referenced.
         * @param item The item.
         * @return True if the item is referenced, false otherwise.
         */
        bool IsReferenced(std::shared_ptr<StackItem> item) const;

        /**
         * @brief Checks if there is a circular reference.
         * @param root The root item.
         * @return True if there is a circular reference, false otherwise.
         */
        bool HasCircularReference(std::shared_ptr<StackItem> root) const;

    private:
        struct ReferenceEntry
        {
            std::shared_ptr<StackItem> Item;
            size_t Count = 0;
        };

        /**
         * @brief Determines if an item needs to be tracked based on its type.
         * @param item The item to check.
         * @return True if the item needs to be tracked, otherwise false.
         */
        static bool NeedTrack(std::shared_ptr<StackItem> item);

        /**
         * @brief Finds strongly connected components using Tarjan's algorithm.
         * @param components The vector to store the components.
         */
        void FindStronglyConnectedComponents(std::vector<std::unordered_set<std::shared_ptr<StackItem>>>& components);

        /**
         * @brief Helper method for Tarjan's algorithm to find strongly connected components.
         * @param v The current vertex (stack item).
         * @param indices Map of vertices to their indices.
         * @param lowlinks Map of vertices to their lowlink values.
         * @param stack Stack of vertices.
         * @param onStack Set of vertices on the stack.
         * @param index Current index.
         * @param components Vector to store the components.
         */
        void StrongConnect(
            std::shared_ptr<StackItem> v,
            std::unordered_map<std::shared_ptr<StackItem>, int>& indices,
            std::unordered_map<std::shared_ptr<StackItem>, int>& lowlinks,
            std::vector<std::shared_ptr<StackItem>>& stack,
            std::unordered_set<std::shared_ptr<StackItem>>& onStack,
            int& index,
            std::vector<std::unordered_set<std::shared_ptr<StackItem>>>& components);

        std::unordered_map<std::shared_ptr<StackItem>, std::unordered_map<std::shared_ptr<StackItem>, ReferenceEntry>> references_;
        std::unordered_map<std::shared_ptr<StackItem>, size_t> stack_references_;
        std::unordered_set<std::shared_ptr<StackItem>> tracked_items_;
        std::unordered_set<std::shared_ptr<StackItem>> zero_referred_;
        std::list<std::unordered_set<std::shared_ptr<StackItem>>> cached_components_;
        size_t references_count_ = 0;

        bool HasCircularReference(std::shared_ptr<StackItem> root, std::unordered_set<std::shared_ptr<StackItem>>& visited) const;
    };
}
