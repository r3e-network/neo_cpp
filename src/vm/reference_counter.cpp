#include <neo/vm/reference_counter.h>
#include <neo/vm/stack_item.h>

namespace neo::vm
{
void ReferenceCounter::AddReference() { references_count_++; }

void ReferenceCounter::RemoveReference() { references_count_--; }

void ReferenceCounter::AddReference(std::shared_ptr<StackItem> child, std::shared_ptr<StackItem> parent)
{
    if (!child || !parent) return;

    references_count_++;

    // Only track CompoundType and Buffer items
    if (!NeedTrack(child)) return;

    // Clear the cached components
    cached_components_.clear();
    tracked_items_.insert(child);

    auto& parentReferences = references_[parent];
    auto it = parentReferences.find(child);
    if (it == parentReferences.end())
    {
        ReferenceEntry entry;
        entry.Item = child;
        entry.Count = 1;
        parentReferences[child] = entry;
    }
    else
    {
        it->second.Count++;
    }
}

void ReferenceCounter::RemoveReference(std::shared_ptr<StackItem> child, std::shared_ptr<StackItem> parent)
{
    if (!child || !parent) return;

    references_count_--;

    // Only track CompoundType and Buffer items
    if (!NeedTrack(child)) return;

    // Clear the cached components
    cached_components_.clear();

    auto parentIt = references_.find(parent);
    if (parentIt == references_.end()) return;

    auto& parentReferences = parentIt->second;
    auto childIt = parentReferences.find(child);
    if (childIt == parentReferences.end()) return;

    if (--childIt->second.Count == 0) parentReferences.erase(childIt);

    if (parentReferences.empty()) references_.erase(parentIt);

    // If the item has no stack references, add it to the zero_referred set
    if (GetStackReferences(child) == 0) zero_referred_.insert(child);
}

bool ReferenceCounter::NeedTrack(std::shared_ptr<StackItem> item)
{
    // Track all items if TrackAllItems is true
    constexpr bool TrackAllItems = false;
    if (TrackAllItems) return true;

    // Track the item if it is a CompoundType or Buffer
    if (item->GetType() == StackItemType::Array || item->GetType() == StackItemType::Struct ||
        item->GetType() == StackItemType::Map || item->GetType() == StackItemType::Buffer)
        return true;

    return false;
}

void ReferenceCounter::AddStackReference(std::shared_ptr<StackItem> item, size_t count)
{
    // Increment the reference count by the specified count
    references_count_ += count;

    // If the item doesn't need to be tracked, return early
    if (!NeedTrack(item)) return;

    // Add the item to the set of tracked items and to the cached components if needed
    if (tracked_items_.insert(item).second && !cached_components_.empty())
    {
        std::unordered_set<std::shared_ptr<StackItem>> component;
        component.insert(item);
        cached_components_.push_back(component);
    }

    // Increment the item's stack references by the specified count
    stack_references_[item] += count;

    // Remove the item from the zero_referred set since it now has references
    zero_referred_.erase(item);
}

void ReferenceCounter::RemoveStackReference(std::shared_ptr<StackItem> item)
{
    // Decrement the reference count
    references_count_--;

    // If the item doesn't need to be tracked, return early
    if (!NeedTrack(item)) return;

    // Decrement the item's stack references and add it to the zero_referred set if it has no references
    auto it = stack_references_.find(item);
    if (it != stack_references_.end() && --it->second == 0)
    {
        stack_references_.erase(it);
        zero_referred_.insert(item);
    }
}

void ReferenceCounter::AddZeroReferred(std::shared_ptr<StackItem> item)
{
    // Add the item to the zero_referred set
    zero_referred_.insert(item);

    // If the item doesn't need to be tracked, return early
    if (!NeedTrack(item)) return;

    // Add the item to the cached components and the set of tracked items
    if (!cached_components_.empty())
    {
        std::unordered_set<std::shared_ptr<StackItem>> component;
        component.insert(item);
        cached_components_.push_back(component);
    }

    tracked_items_.insert(item);
}

size_t ReferenceCounter::CheckZeroReferred()
{
    // If there are items with zero references, process them
    if (!zero_referred_.empty())
    {
        // Clear the zero_referred set since we are going to process all of them
        zero_referred_.clear();

        // If cached components are empty, we need to recompute the strongly connected components (SCCs)
        if (cached_components_.empty())
        {
            // Find strongly connected components using Tarjan's algorithm
            std::vector<std::unordered_set<std::shared_ptr<StackItem>>> components;
            FindStronglyConnectedComponents(components);
        }

        // Reset all tracked items' Tarjan algorithm-related fields
        for (const auto& item : tracked_items_)
        {
            item->Reset();
        }

        // Process each SCC in the cached_components list
        for (auto it = cached_components_.begin(); it != cached_components_.end();)
        {
            auto& component = *it;
            bool on_stack = false;

            // Check if any item in the SCC is still on the stack
            for (const auto& item : component)
            {
                // An item is considered 'on stack' if it has stack references
                if (GetStackReferences(item) > 0)
                {
                    on_stack = true;
                    break;
                }

                // Or if its parent items are still on stack
                for (const auto& [parent, children] : references_)
                {
                    auto childIt = children.find(item);
                    if (childIt != children.end() && childIt->second.Count > 0 && parent->IsOnStack())
                    {
                        on_stack = true;
                        break;
                    }
                }

                if (on_stack) break;
            }

            // If any item in the component is on stack, mark all items in the component as on stack
            if (on_stack)
            {
                for (const auto& item : component)
                {
                    item->SetOnStack(true);
                }
                ++it;
            }
            else
            {
                // If no item in the component is on stack, clean up the component
                for (const auto& item : component)
                {
                    // Remove the item from tracked_items_
                    tracked_items_.erase(item);

                    // Remove all references from this item to other items
                    auto refIt = references_.find(item);
                    if (refIt != references_.end())
                    {
                        references_.erase(refIt);
                    }

                    // Remove all stack references for this item
                    stack_references_.erase(item);
                }

                // Remove the component from cached_components_
                it = cached_components_.erase(it);
            }
        }
    }

    return references_count_;
}

size_t ReferenceCounter::GetReferenceCount(std::shared_ptr<StackItem> item) const
{
    size_t count = 0;
    for (const auto& [parent, children] : references_)
    {
        auto it = children.find(item);
        if (it != children.end()) count += it->second.Count;
    }
    return count;
}

size_t ReferenceCounter::GetStackReferences(std::shared_ptr<StackItem> item) const
{
    auto it = stack_references_.find(item);
    return it != stack_references_.end() ? it->second : 0;
}

bool ReferenceCounter::IsReferenced(std::shared_ptr<StackItem> item) const
{
    // Check if the item has stack references
    if (GetStackReferences(item) > 0) return true;

    // Check if the item has object references
    for (const auto& [parent, children] : references_)
    {
        if (children.find(item) != children.end()) return true;
    }

    return false;
}

bool ReferenceCounter::HasCircularReference(std::shared_ptr<StackItem> root) const
{
    std::unordered_set<std::shared_ptr<StackItem>> visited;
    return HasCircularReference(root, visited);
}

void ReferenceCounter::FindStronglyConnectedComponents(
    std::vector<std::unordered_set<std::shared_ptr<StackItem>>>& components)
{
    // If we have cached components, use them
    if (!cached_components_.empty())
    {
        components.clear();
        components.reserve(cached_components_.size());
        for (const auto& component : cached_components_)
        {
            components.push_back(component);
        }
        return;
    }

    // Initialize data structures for Tarjan's algorithm
    std::unordered_map<std::shared_ptr<StackItem>, int> indices;
    std::unordered_map<std::shared_ptr<StackItem>, int> lowlinks;
    std::vector<std::shared_ptr<StackItem>> stack;
    std::unordered_set<std::shared_ptr<StackItem>> onStack;
    int index = 0;

    // Run Tarjan's algorithm on each tracked item
    for (const auto& v : tracked_items_)
    {
        if (indices.find(v) == indices.end())
        {
            StrongConnect(v, indices, lowlinks, stack, onStack, index, components);
        }
    }
}

void ReferenceCounter::StrongConnect(std::shared_ptr<StackItem> v,
                                     std::unordered_map<std::shared_ptr<StackItem>, int>& indices,
                                     std::unordered_map<std::shared_ptr<StackItem>, int>& lowlinks,
                                     std::vector<std::shared_ptr<StackItem>>& stack,
                                     std::unordered_set<std::shared_ptr<StackItem>>& onStack, int& index,
                                     std::vector<std::unordered_set<std::shared_ptr<StackItem>>>& components)
{
    // Set the depth index for v to the smallest unused index
    indices[v] = index;
    lowlinks[v] = index;
    v->SetDFN(index);
    v->SetLowLink(index);
    index++;
    stack.push_back(v);
    onStack.insert(v);
    v->SetOnStack(true);

    // Consider successors of v
    auto it = references_.find(v);
    if (it != references_.end())
    {
        for (const auto& [w, entry] : it->second)
        {
            if (indices.find(w) == indices.end())
            {
                // Successor w has not yet been visited; recurse on it
                StrongConnect(w, indices, lowlinks, stack, onStack, index, components);
                lowlinks[v] = std::min(lowlinks[v], lowlinks[w]);
                v->SetLowLink(std::min(v->GetLowLink(), w->GetLowLink()));
            }
            else if (onStack.find(w) != onStack.end())
            {
                // Successor w is in stack and hence in the current SCC
                lowlinks[v] = std::min(lowlinks[v], indices[w]);
                v->SetLowLink(std::min(v->GetLowLink(), w->GetDFN()));
            }
        }
    }

    // If v is a root node, pop the stack and generate an SCC
    if (lowlinks[v] == indices[v])
    {
        std::unordered_set<std::shared_ptr<StackItem>> component;
        std::shared_ptr<StackItem> w;
        do
        {
            w = stack.back();
            stack.pop_back();
            onStack.erase(w);
            w->SetOnStack(false);
            component.insert(w);
        } while (w != v);

        components.push_back(component);
        cached_components_.push_back(component);
    }
}

bool ReferenceCounter::HasCircularReference(std::shared_ptr<StackItem> root,
                                            std::unordered_set<std::shared_ptr<StackItem>>& visited) const
{
    if (!root) return false;

    if (visited.find(root) != visited.end()) return true;

    visited.insert(root);

    auto it = references_.find(root);
    if (it != references_.end())
    {
        for (const auto& [child, entry] : it->second)
        {
            if (HasCircularReference(child, visited)) return true;
        }
    }

    visited.erase(root);
    return false;
}
}  // namespace neo::vm
