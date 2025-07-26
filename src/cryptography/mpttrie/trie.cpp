#include <neo/cryptography/mpttrie/trie.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_span.h>
#include <algorithm>
#include <vector>
#include <memory>
#include <cstddef>
#include <cstdint>
#include <span>
#include <spdlog/spdlog.h>

#define LOG_ERROR(msg, ...) spdlog::error(msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) spdlog::debug(msg, ##__VA_ARGS__)

namespace neo::cryptography::mpttrie
{
    // Complete MPTTrie implementation for Neo blockchain
    // This implements the Merkle Patricia Trie data structure used for
    // efficient and cryptographically secure state storage.

    Trie::Trie(std::shared_ptr<persistence::IStoreSnapshot> store, io::UInt256 root, bool full_state)
        : store_(store), cache_(std::make_unique<Cache>(store, 0xf0)), full_state_(full_state)
    {
        if (!root.IsZero())
        {
            SetRoot(root);
        }
        else
        {
            root_ = Node::NewHash(io::UInt256::Zero());
        }
    }

    void Trie::SetRoot(const io::UInt256& root_hash)
    {
        if (root_hash.IsZero())
        {
            root_ = Node::NewHash(io::UInt256::Zero());
        }
        else
        {
            root_ = Node::NewHash(root_hash);
        }
    }

    io::UInt256 Trie::GetRootHash() const
    {
        if (!root_)
            return io::UInt256::Zero();

        return root_->GetHash();
    }

    io::ByteVector Trie::Get(io::ByteSpan key) const
    {
        io::ByteVector value;
        if (TryGet(key, value))
            return value;

        throw std::out_of_range("Key not found");
    }

    bool Trie::TryGet(io::ByteSpan key, io::ByteVector& value) const
    {
        auto path = ToNibbles(key);
        if (path.empty() || path.size() > Node::MaxKeyLength)
        {
            return false;
        }
        return TryGetInternal(*root_, io::ByteSpan(path.data(), path.size()), value);
    }

    void Trie::Put(io::ByteSpan key, io::ByteSpan value)
    {
        auto path = ToNibbles(key);
        if (path.empty() || path.size() > Node::MaxKeyLength)
        {
            throw std::invalid_argument("Invalid key");
        }
        if (value.size() > Node::MaxValueLength)
        {
            throw std::invalid_argument("Value too large");
        }

        auto value_node = Node::NewLeaf(value);
        PutInternal(root_, io::ByteSpan(path.data(), path.size()), std::move(value_node));
    }

    bool Trie::Delete(io::ByteSpan key)
    {
        auto path = ToNibbles(key);
        if (path.empty() || path.size() > Node::MaxKeyLength)
        {
            return false;
        }

        return TryDeleteInternal(root_, io::ByteSpan(path.data(), path.size()));
    }

    void Trie::Commit()
    {
        if (cache_)
        {
            cache_->Commit();
        }
    }

    std::vector<io::ByteVector> Trie::GetProof(io::ByteSpan key) const
    {
        std::vector<io::ByteVector> proof;
        auto path = ToNibbles(key);

        if (path.empty() || path.size() > Node::MaxKeyLength)
        {
            return proof;
        }

        GetProofInternal(*root_, io::ByteSpan(path.data(), path.size()), proof);
        return proof;
    }

    std::vector<uint8_t> Trie::ToNibbles(io::ByteSpan key)
    {
        std::vector<uint8_t> result;
        result.reserve(key.Size() * 2);

        for (uint8_t byte : key)
        {
            result.push_back(byte >> 4);
            result.push_back(byte & 0x0F);
        }

        return result;
    }

    io::ByteVector Trie::CreateKey(const io::UInt256& hash)
    {
        io::ByteVector key;
        key.Reserve(1 + io::UInt256::Size);
        key.Push(0xf0);  // MPT prefix
        key.Append(hash.AsSpan());
        return key;
    }

    bool Trie::TryGetInternal(Node& node, io::ByteSpan path, io::ByteVector& value) const
    {
        try
        {
            // Resolve the node if it's a hash node
            auto resolved_node = &node;
            std::unique_ptr<Node> cache_node;

            if (node.GetType() == NodeType::HashNode)
            {
                auto hash_data = node.GetStoredHash();
                if (hash_data.IsZero())
                    return false;

                // Load node from cache
                cache_node = cache_->Resolve(hash_data);
                if (!cache_node)
                    return false;

                resolved_node = cache_node.get();
            }

            switch (resolved_node->GetType())
            {
                case NodeType::BranchNode:
                    if (path.empty())
                    {
                        // Check if branch has a value (index 16)
                        const auto& children = resolved_node->GetChildren();
                        if (children.size() > 16 && children[16])
                        {
                            return TryGetInternal(*children[16], io::ByteSpan(), value);
                        }
                        return false;
                    }
                    else
                    {
                        // Follow the branch
                        uint8_t index = path[0];
                        if (index >= 16) return false;

                        const auto& children = resolved_node->GetChildren();
                        if (children.size() <= index || !children[index])
                            return false;

                        return TryGetInternal(*children[index], path.subspan(1), value);
                    }

                case NodeType::ExtensionNode:
                {
                    auto node_key = resolved_node->GetKey();
                    if (path.size() < node_key.size())
                        return false;

                    // Check if path starts with node key
                    if (!std::equal(node_key.begin(), node_key.end(), path.begin()))
                        return false;

                    // Continue with remaining path
                    return TryGetInternal(resolved_node->GetNext(), path.subspan(node_key.size()), value);
                }

                case NodeType::LeafNode:
                    if (path.empty())
                    {
                        auto leaf_value = resolved_node->GetValue();
                        value = io::ByteVector(leaf_value);
                        return true;
                    }
                    return false;

                default:
                    return false;
            }
        }
        catch (const std::bad_alloc& e)
        {
            LOG_ERROR("Memory allocation failed in Trie::TryGetInternal: {}", e.what());
            return false;
        }
        catch (const std::runtime_error& e)
        {
            LOG_DEBUG("Runtime error in Trie::TryGetInternal: {}", e.what());
            return false;
        }
        catch (const std::exception& e)
        {
            LOG_DEBUG("Exception in Trie::TryGetInternal: {}", e.what());
            return false;
        }
    }

    void Trie::PutInternal(std::unique_ptr<Node>& node, io::ByteSpan path, std::unique_ptr<Node> value_node)
    {
        if (!node)
        {
            if (path.empty())
            {
                node = std::move(value_node);
            }
            else
            {
                // Create extension node leading to leaf
                node = Node::NewExtension(path, std::move(value_node));
            }
            return;
        }

        // Resolve hash nodes
        if (node->GetType() == NodeType::HashNode)
        {
            auto hash_data = node->GetStoredHash();
            if (!hash_data.IsZero())
            {
                auto resolved_node = cache_->Resolve(hash_data);
                if (resolved_node)
                {
                    node = std::move(resolved_node);
                }
                else
                {
                    // Hash node points to non-existent data, replace with new node
                    if (path.empty())
                    {
                        node = std::move(value_node);
                    }
                    else
                    {
                        node = Node::NewExtension(path, std::move(value_node));
                    }
                    return;
                }
            }
            else
            {
                // Empty hash node, replace
                if (path.empty())
                {
                    node = std::move(value_node);
                }
                else
                {
                    node = Node::NewExtension(path, std::move(value_node));
                }
                return;
            }
        }

        switch (node->GetType())
        {
            case NodeType::BranchNode:
                PutBranch(node, path, std::move(value_node));
                break;

            case NodeType::ExtensionNode:
                PutExtension(node, path, std::move(value_node));
                break;

            case NodeType::LeafNode:
                PutLeaf(node, path, std::move(value_node));
                break;

            default:
                throw std::runtime_error("Invalid node type for put operation");
        }
    }

    void Trie::PutBranch(std::unique_ptr<Node>& node, io::ByteSpan path, std::unique_ptr<Node> value_node)
    {
        auto& children = node->GetChildren();

        if (path.empty())
        {
            // Set value at branch node (index 16)
            if (children.size() <= 16)
                children.resize(17);
            children[16] = std::move(value_node);
        }
        else
        {
            // Follow branch
            uint8_t index = path[0];
            if (index >= 16)
                throw std::invalid_argument("Invalid path nibble");

            if (children.size() <= index)
                children.resize(std::max(static_cast<size_t>(index + 1), static_cast<size_t>(17)));

            PutInternal(children[index], path.subspan(1), std::move(value_node));
        }

        node->SetDirty();
    }

    void Trie::PutExtension(std::unique_ptr<Node>& node, io::ByteSpan path, std::unique_ptr<Node> value_node)
    {
        auto node_key = node->GetKey();
        auto node_key_span = io::ByteSpan(node_key.data(), node_key.size());
        auto common_len = CommonPrefixLength(node_key_span, path);

        if (common_len == node_key.size())
        {
            // Path continues through extension
            auto remaining_path = path.subspan(common_len);
            auto& next_node = node->GetNextPtr();
            PutInternal(next_node, remaining_path, std::move(value_node));
        }
        else
        {
            // Split extension node
            auto branch = Node::NewBranch();
            auto& children = branch->GetChildren();
            children.resize(17);

            if (common_len < node_key.size())
            {
                // Create new extension for remaining original key
                auto remaining_original = io::ByteSpan(node_key.data() + common_len + 1, node_key.size() - common_len - 1);
                if (remaining_original.empty())
                {
                    auto& next_node = node->GetNextPtr();
                    children[node_key[common_len]] = std::move(next_node);
                }
                else
                {
                    auto& next_node = node->GetNextPtr();
                    children[node_key[common_len]] = Node::NewExtension(remaining_original, std::move(next_node));
                }
            }

            if (common_len < path.size())
            {
                // Add new value
                auto remaining_new = path.subspan(common_len + 1);
                if (remaining_new.empty())
                {
                    children[path[common_len]] = std::move(value_node);
                }
                else
                {
                    children[path[common_len]] = Node::NewExtension(remaining_new, std::move(value_node));
                }
            }
            else
            {
                // New value goes at branch
                children[16] = std::move(value_node);
            }

            if (common_len == 0)
            {
                node = std::move(branch);
            }
            else
            {
                auto common_key = io::ByteSpan(node_key.data(), common_len);
                node = Node::NewExtension(common_key, std::move(branch));
            }
        }

        node->SetDirty();
    }

    void Trie::PutLeaf(std::unique_ptr<Node>& node, io::ByteSpan path, std::unique_ptr<Node> value_node)
    {
        if (path.empty())
        {
            // Replace leaf value
            node = std::move(value_node);
        }
        else
        {
            // Convert leaf to branch and add new path
            auto branch = Node::NewBranch();
            auto& children = branch->GetChildren();
            children.resize(17);

            // Original leaf goes to branch value
            children[16] = std::move(node);

            // New value
            if (path.size() == 1)
            {
                children[path[0]] = std::move(value_node);
            }
            else
            {
                children[path[0]] = Node::NewExtension(path.subspan(1), std::move(value_node));
            }

            node = std::move(branch);
        }

        node->SetDirty();
    }

    bool Trie::TryDeleteInternal(std::unique_ptr<Node>& node, io::ByteSpan path)
    {
        if (!node)
            return false;

        // Resolve hash nodes
        if (node->GetType() == NodeType::HashNode)
        {
            auto hash_data = node->GetStoredHash();
            if (hash_data.IsZero())
                return false;

            auto resolved_node = cache_->Resolve(hash_data);
            if (!resolved_node)
                return false;

            node = std::move(resolved_node);
        }

        bool deleted = false;

        switch (node->GetType())
        {
            case NodeType::BranchNode:
                deleted = DeleteBranch(node, path);
                break;

            case NodeType::ExtensionNode:
                deleted = DeleteExtension(node, path);
                break;

            case NodeType::LeafNode:
                if (path.empty())
                {
                    node.reset();
                    deleted = true;
                }
                break;

            default:
                break;
        }

        if (deleted && node)
        {
            node->SetDirty();
        }

        return deleted;
    }

    bool Trie::DeleteBranch(std::unique_ptr<Node>& node, io::ByteSpan path)
    {
        auto& children = node->GetChildren();

        if (path.empty())
        {
            // Delete value at branch
            if (children.size() > 16 && children[16])
            {
                children[16].reset();
                return true;
            }
            return false;
        }
        else
        {
            uint8_t index = path[0];
            if (index >= 16 || children.size() <= index || !children[index])
                return false;

            bool deleted = TryDeleteInternal(children[index], path.subspan(1));

            if (deleted)
            {
                // Optimize branch structure after deletion by merging nodes
                SimplifyBranch(node);
            }

            return deleted;
        }
    }

    bool Trie::DeleteExtension(std::unique_ptr<Node>& node, io::ByteSpan path)
    {
        auto node_key = node->GetKey();

        if (path.size() < node_key.size())
            return false;

        if (!std::equal(node_key.begin(), node_key.end(), path.begin()))
            return false;

        return TryDeleteInternal(node->GetNextPtr(), path.subspan(node_key.size()));
    }

    void Trie::SimplifyBranch(std::unique_ptr<Node>& node)
    {
        auto& children = node->GetChildren();
        int non_null_count = 0;
        int last_index = -1;

        for (int i = 0; i < static_cast<int>(children.size()); ++i)
        {
            if (children[i])
            {
                non_null_count++;
                last_index = i;
            }
        }

        if (non_null_count == 1)
        {
            if (last_index == 16)
            {
                // Only value exists, replace with leaf
                node = std::move(children[16]);
            }
            else
            {
                // Only one child, merge with extension
                auto child = std::move(children[last_index]);
                if (child->GetType() == NodeType::ExtensionNode)
                {
                    // Merge extensions
                    std::vector<uint8_t> new_key;
                    new_key.push_back(static_cast<uint8_t>(last_index));
                    auto child_key = child->GetKey();
                    new_key.insert(new_key.end(), child_key.begin(), child_key.end());

                    node = Node::NewExtension(io::ByteSpan(new_key.data(), new_key.size()), std::move(child->GetNextPtr()));
                }
                else
                {
                    std::vector<uint8_t> new_key = { static_cast<uint8_t>(last_index) };
                    node = Node::NewExtension(io::ByteSpan(new_key.data(), new_key.size()), std::move(child));
                }
            }
        }
    }

    bool Trie::GetProofInternal(Node& node, io::ByteSpan path, std::vector<io::ByteVector>& proof) const
    {
        // Add current node to proof
        proof.push_back(node.ToArrayWithoutReference());

        // Resolve hash nodes
        std::unique_ptr<Node> cache_node;
        auto resolved_node = &node;

        if (node.GetType() == NodeType::HashNode)
        {
            auto hash_data = node.GetStoredHash();
            if (hash_data.IsZero())
                return false;

            cache_node = cache_->Resolve(hash_data);
            if (!cache_node)
                return false;

            resolved_node = cache_node.get();
            proof.push_back(resolved_node->ToArrayWithoutReference());
        }

        switch (resolved_node->GetType())
        {
            case NodeType::BranchNode:
                if (path.empty())
                {
                    const auto& children = resolved_node->GetChildren();
                    if (children.size() > 16 && children[16])
                    {
                        return GetProofInternal(*children[16], io::ByteSpan(), proof);
                    }
                    return true; // Found at branch
                }
                else
                {
                    uint8_t index = path[0];
                    if (index >= 16) return false;

                    const auto& children = resolved_node->GetChildren();
                    if (children.size() <= index || !children[index])
                        return false;

                    return GetProofInternal(*children[index], path.subspan(1), proof);
                }

            case NodeType::ExtensionNode:
            {
                auto node_key = resolved_node->GetKey();
                if (path.size() < node_key.size())
                    return false;

                if (!std::equal(node_key.begin(), node_key.end(), path.begin()))
                    return false;

                return GetProofInternal(resolved_node->GetNext(), path.subspan(node_key.size()), proof);
            }

            case NodeType::LeafNode:
                return path.empty();

            default:
                return false;
        }
    }

    size_t Trie::CommonPrefixLength(io::ByteSpan a, io::ByteSpan b)
    {
        size_t min_len = std::min(a.size(), b.size());
        size_t i = 0;

        while (i < min_len && a[i] == b[i])
        {
            ++i;
        }

        return i;
    }
}
