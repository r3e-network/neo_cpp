#include <neo/cryptography/mpttrie/node.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <stdexcept>
#include <sstream>

namespace neo::cryptography::mpttrie
{
    Node::Node() : type_(NodeType::Empty), hash_dirty_(true), reference_(0)
    {
    }

    Node::Node(const Node& other)
        : type_(other.type_)
        , hash_(other.hash_)
        , hash_dirty_(other.hash_dirty_)
        , reference_(other.reference_)
        , key_(other.key_)
        , value_(other.value_)
        , stored_hash_(other.stored_hash_)
    {
        // Deep copy children
        if (!other.children_.empty())
        {
            children_.reserve(other.children_.size());
            for (const auto& child : other.children_)
            {
                if (child)
                    children_.push_back(child->Clone());
                else
                    children_.push_back(std::make_unique<Node>());
            }
        }

        // Deep copy next node
        if (other.next_)
        {
            next_ = other.next_->Clone();
        }
    }

    Node::Node(Node&& other) noexcept
        : type_(other.type_)
        , hash_(std::move(other.hash_))
        , hash_dirty_(other.hash_dirty_)
        , reference_(other.reference_)
        , children_(std::move(other.children_))
        , key_(std::move(other.key_))
        , next_(std::move(other.next_))
        , value_(std::move(other.value_))
        , stored_hash_(std::move(other.stored_hash_))
    {
        other.type_ = NodeType::Empty;
        other.hash_dirty_ = true;
        other.reference_ = 0;
    }

    Node& Node::operator=(const Node& other)
    {
        if (this != &other)
        {
            *this = Node(other);  // Copy and swap
        }
        return *this;
    }

    Node& Node::operator=(Node&& other) noexcept
    {
        if (this != &other)
        {
            type_ = other.type_;
            hash_ = std::move(other.hash_);
            hash_dirty_ = other.hash_dirty_;
            reference_ = other.reference_;
            children_ = std::move(other.children_);
            key_ = std::move(other.key_);
            next_ = std::move(other.next_);
            value_ = std::move(other.value_);
            stored_hash_ = std::move(other.stored_hash_);

            other.type_ = NodeType::Empty;
            other.hash_dirty_ = true;
            other.reference_ = 0;
        }
        return *this;
    }

    NodeType Node::GetType() const
    {
        return type_;
    }

    io::UInt256 Node::GetHash() const
    {
        if (hash_dirty_)
        {
            hash_ = CalculateHash();
            hash_dirty_ = false;
        }
        return hash_;
    }

    bool Node::IsEmpty() const
    {
        return type_ == NodeType::Empty;
    }

    int Node::GetReference() const
    {
        return reference_;
    }

    void Node::SetReference(int reference)
    {
        reference_ = reference;
    }

    void Node::SetDirty()
    {
        hash_dirty_ = true;
    }

    size_t Node::GetSize() const
    {
        size_t size = sizeof(NodeType);

        switch (type_)
        {
            case NodeType::BranchNode:
                return size + GetBranchSize() + sizeof(reference_);
            case NodeType::ExtensionNode:
                return size + GetExtensionSize() + sizeof(reference_);
            case NodeType::LeafNode:
                return size + GetLeafSize() + sizeof(reference_);
            case NodeType::HashNode:
                return size + GetHashSize();
            case NodeType::Empty:
                return size;
            default:
                throw std::invalid_argument("Unsupported node type");
        }
    }

    size_t Node::GetSizeAsChild() const
    {
        switch (type_)
        {
            case NodeType::BranchNode:
            case NodeType::ExtensionNode:
            case NodeType::LeafNode:
                return NewHash(GetHash())->GetSize();
            case NodeType::HashNode:
            case NodeType::Empty:
                return GetSize();
            default:
                throw std::invalid_argument("Unsupported node type");
        }
    }

    std::unique_ptr<Node> Node::Clone() const
    {
        return std::make_unique<Node>(*this);
    }

    std::unique_ptr<Node> Node::CloneAsChild() const
    {
        switch (type_)
        {
            case NodeType::BranchNode:
            case NodeType::ExtensionNode:
            case NodeType::LeafNode:
                return NewHash(GetHash());
            case NodeType::HashNode:
            case NodeType::Empty:
                return Clone();
            default:
                throw std::invalid_argument("Unsupported node type");
        }
    }

    io::ByteVector Node::ToArray() const
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        Serialize(writer);

        std::string str = stream.str();
        return io::ByteVector(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }

    io::ByteVector Node::ToArrayWithoutReference() const
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);

        writer.Write(static_cast<uint8_t>(type_));

        switch (type_)
        {
            case NodeType::BranchNode:
                SerializeBranch(writer);
                break;
            case NodeType::ExtensionNode:
                SerializeExtension(writer);
                break;
            case NodeType::LeafNode:
                SerializeLeaf(writer);
                break;
            case NodeType::HashNode:
                writer.Write(stored_hash_);
                break;
            case NodeType::Empty:
                break;
            default:
                throw std::invalid_argument("Unsupported node type");
        }

        std::string str = stream.str();
        return io::ByteVector(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }

    void Node::SerializeAsChild(io::BinaryWriter& writer) const
    {
        switch (type_)
        {
            case NodeType::BranchNode:
            case NodeType::ExtensionNode:
            case NodeType::LeafNode:
                NewHash(GetHash())->Serialize(writer);
                break;
            case NodeType::HashNode:
            case NodeType::Empty:
                Serialize(writer);
                break;
            default:
                throw std::invalid_argument("Unsupported node type");
        }
    }

    void Node::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(static_cast<uint8_t>(type_));

        switch (type_)
        {
            case NodeType::BranchNode:
                SerializeBranch(writer);
                writer.WriteVarInt(reference_);
                break;
            case NodeType::ExtensionNode:
                SerializeExtension(writer);
                writer.WriteVarInt(reference_);
                break;
            case NodeType::LeafNode:
                SerializeLeaf(writer);
                writer.WriteVarInt(reference_);
                break;
            case NodeType::HashNode:
                writer.Write(stored_hash_);
                break;
            case NodeType::Empty:
                break;
            default:
                throw std::invalid_argument("Unsupported node type");
        }
    }

    void Node::Deserialize(io::BinaryReader& reader)
    {
        type_ = static_cast<NodeType>(reader.ReadUInt8());
        hash_dirty_ = true;

        switch (type_)
        {
            case NodeType::BranchNode:
                DeserializeBranch(reader);
                reference_ = reader.ReadVarInt();
                break;
            case NodeType::ExtensionNode:
                DeserializeExtension(reader);
                reference_ = reader.ReadVarInt();
                break;
            case NodeType::LeafNode:
                DeserializeLeaf(reader);
                reference_ = reader.ReadVarInt();
                break;
            case NodeType::HashNode:
                stored_hash_ = reader.ReadUInt256();
                hash_ = stored_hash_;
                hash_dirty_ = false;
                break;
            case NodeType::Empty:
                reference_ = 0;
                break;
            default:
                throw std::invalid_argument("Unsupported node type");
        }
    }

    // Static factory methods
    std::unique_ptr<Node> Node::NewBranch()
    {
        auto node = std::make_unique<Node>();
        node->type_ = NodeType::BranchNode;
        node->reference_ = 1;
        node->children_.resize(BranchChildCount);
        for (int i = 0; i < BranchChildCount; ++i)
        {
            node->children_[i] = std::make_unique<Node>();
        }
        return node;
    }

    std::unique_ptr<Node> Node::NewExtension(std::span<const uint8_t> key, std::unique_ptr<Node> next)
    {
        auto node = std::make_unique<Node>();
        node->type_ = NodeType::ExtensionNode;
        node->reference_ = 1;
        node->key_ = io::ByteVector(key);
        node->next_ = std::move(next);
        return node;
    }

    std::unique_ptr<Node> Node::NewLeaf(std::span<const uint8_t> value)
    {
        auto node = std::make_unique<Node>();
        node->type_ = NodeType::LeafNode;
        node->reference_ = 1;
        node->value_ = io::ByteVector(value);
        return node;
    }

    std::unique_ptr<Node> Node::NewHash(const io::UInt256& hash)
    {
        auto node = std::make_unique<Node>();
        node->type_ = NodeType::HashNode;
        node->stored_hash_ = hash;
        node->hash_ = hash;
        node->hash_dirty_ = false;
        return node;
    }

    // Branch node accessors
    std::vector<std::unique_ptr<Node>>& Node::GetChildren()
    {
        return children_;
    }

    const std::vector<std::unique_ptr<Node>>& Node::GetChildren() const
    {
        return children_;
    }

    // Extension node accessors
    std::span<const uint8_t> Node::GetKey() const
    {
        return std::span<const uint8_t>(key_.Data(), key_.Size());
    }

    void Node::SetKey(std::span<const uint8_t> key)
    {
        key_ = io::ByteVector(key);
        SetDirty();
    }

    Node& Node::GetNext()
    {
        if (!next_)
            throw std::runtime_error("Next node is null");
        return *next_;
    }

    const Node& Node::GetNext() const
    {
        if (!next_)
            throw std::runtime_error("Next node is null");
        return *next_;
    }

    std::unique_ptr<Node>& Node::GetNextPtr()
    {
        return next_;
    }

    const std::unique_ptr<Node>& Node::GetNextPtr() const
    {
        return next_;
    }

    void Node::SetNext(std::unique_ptr<Node> next)
    {
        next_ = std::move(next);
        SetDirty();
    }

    // Leaf node accessors
    std::span<const uint8_t> Node::GetValue() const
    {
        return std::span<const uint8_t>(value_.Data(), value_.Size());
    }

    void Node::SetValue(std::span<const uint8_t> value)
    {
        value_ = io::ByteVector(value);
        SetDirty();
    }

    // Hash node accessors
    const io::UInt256& Node::GetStoredHash() const
    {
        return stored_hash_;
    }

    io::UInt256 Node::CalculateHash() const
    {
        auto data = ToArrayWithoutReference();
        return Hash::Sha256(data.AsSpan());
    }

    // Simplified implementations for serialization methods
    void Node::SerializeBranch(io::BinaryWriter& writer) const
    {
        for (const auto& child : children_)
        {
            child->SerializeAsChild(writer);
        }
    }

    void Node::DeserializeBranch(io::BinaryReader& reader)
    {
        children_.resize(BranchChildCount);
        for (int i = 0; i < BranchChildCount; ++i)
        {
            children_[i] = std::make_unique<Node>();
            children_[i]->Deserialize(reader);
        }
    }

    void Node::SerializeExtension(io::BinaryWriter& writer) const
    {
        writer.WriteVarBytes(key_.AsSpan());
        next_->SerializeAsChild(writer);
    }

    void Node::DeserializeExtension(io::BinaryReader& reader)
    {
        key_ = reader.ReadVarBytes();
        next_ = std::make_unique<Node>();
        next_->Deserialize(reader);
    }

    void Node::SerializeLeaf(io::BinaryWriter& writer) const
    {
        writer.WriteVarBytes(value_.AsSpan());
    }

    void Node::DeserializeLeaf(io::BinaryReader& reader)
    {
        value_ = reader.ReadVarBytes();
    }

    size_t Node::GetBranchSize() const
    {
        size_t size = 0;
        for (const auto& child : children_)
        {
            size += child->GetSizeAsChild();
        }
        return size;
    }

    size_t Node::GetExtensionSize() const
    {
        return key_.GetVarSize() + next_->GetSizeAsChild();
    }

    size_t Node::GetLeafSize() const
    {
        return value_.GetVarSize();
    }

    size_t Node::GetHashSize() const
    {
        return io::UInt256::Size;
    }
}
