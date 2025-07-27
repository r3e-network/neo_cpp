#pragma once

#include <memory>
#include <neo/cryptography/mpttrie/node_type.h>
#include <neo/io/byte_vector.h>
#include <neo/io/serializable.h>
#include <neo/io/uint256.h>
#include <span>
#include <vector>

namespace neo::cryptography::mpttrie
{
/**
 * @brief Node in the Merkle Patricia Trie.
 */
class Node : public io::ISerializable
{
  public:
    static constexpr int BranchChildCount = 17;
    static constexpr int MaxKeyLength = 1024;
    static constexpr int MaxValueLength = 1024 * 1024;

    /**
     * @brief Default constructor creates an empty node.
     */
    Node();

    /**
     * @brief Copy constructor.
     */
    Node(const Node& other);

    /**
     * @brief Move constructor.
     */
    Node(Node&& other) noexcept;

    /**
     * @brief Assignment operator.
     */
    Node& operator=(const Node& other);

    /**
     * @brief Move assignment operator.
     */
    Node& operator=(Node&& other) noexcept;

    /**
     * @brief Destructor.
     */
    ~Node() = default;

    /**
     * @brief Gets the node type.
     * @return The node type.
     */
    NodeType GetType() const;

    /**
     * @brief Gets the hash of this node.
     * @return The hash.
     */
    io::UInt256 GetHash() const;

    /**
     * @brief Checks if this is an empty node.
     * @return True if empty, false otherwise.
     */
    bool IsEmpty() const;

    /**
     * @brief Gets the reference count.
     * @return The reference count.
     */
    int GetReference() const;

    /**
     * @brief Sets the reference count.
     * @param reference The reference count.
     */
    void SetReference(int reference);

    /**
     * @brief Marks the node as dirty (hash needs recalculation).
     */
    void SetDirty();

    /**
     * @brief Gets the size of this node when serialized.
     * @return The size in bytes.
     */
    size_t GetSize() const;

    /**
     * @brief Gets the size of this node when used as a child.
     * @return The size in bytes.
     */
    size_t GetSizeAsChild() const;

    /**
     * @brief Clones this node.
     * @return A cloned node.
     */
    std::unique_ptr<Node> Clone() const;

    /**
     * @brief Clones this node as a child reference.
     * @return A cloned node.
     */
    std::unique_ptr<Node> CloneAsChild() const;

    /**
     * @brief Serializes the node to a byte array.
     * @return The serialized data.
     */
    io::ByteVector ToArray() const;

    /**
     * @brief Serializes the node to a byte array without reference count.
     * @return The serialized data.
     */
    io::ByteVector ToArrayWithoutReference() const;

    /**
     * @brief Serializes this node as a child.
     * @param writer The binary writer.
     */
    void SerializeAsChild(io::BinaryWriter& writer) const;

    // ISerializable implementation
    void Serialize(io::BinaryWriter& writer) const override;
    void Deserialize(io::BinaryReader& reader) override;

    // Static factory methods
    /**
     * @brief Creates a new branch node.
     * @return A new branch node.
     */
    static std::unique_ptr<Node> NewBranch();

    /**
     * @brief Creates a new extension node.
     * @param key The key.
     * @param next The next node.
     * @return A new extension node.
     */
    static std::unique_ptr<Node> NewExtension(std::span<const uint8_t> key, std::unique_ptr<Node> next);

    /**
     * @brief Creates a new leaf node.
     * @param value The value.
     * @return A new leaf node.
     */
    static std::unique_ptr<Node> NewLeaf(std::span<const uint8_t> value);

    /**
     * @brief Creates a new hash node.
     * @param hash The hash.
     * @return A new hash node.
     */
    static std::unique_ptr<Node> NewHash(const io::UInt256& hash);

    // Branch node accessors
    /**
     * @brief Gets the children array (for branch nodes).
     * @return The children array.
     */
    std::vector<std::unique_ptr<Node>>& GetChildren();

    /**
     * @brief Gets the children array (for branch nodes, const version).
     * @return The children array.
     */
    const std::vector<std::unique_ptr<Node>>& GetChildren() const;

    // Extension node accessors
    /**
     * @brief Gets the key (for extension nodes).
     * @return The key.
     */
    std::span<const uint8_t> GetKey() const;

    /**
     * @brief Sets the key (for extension nodes).
     * @param key The key.
     */
    void SetKey(std::span<const uint8_t> key);

    /**
     * @brief Gets the next node (for extension nodes).
     * @return The next node.
     */
    Node& GetNext();

    /**
     * @brief Gets the next node (for extension nodes, const version).
     * @return The next node.
     */
    const Node& GetNext() const;

    /**
     * @brief Gets the next node pointer (for extension nodes).
     * @return The next node pointer.
     */
    std::unique_ptr<Node>& GetNextPtr();

    /**
     * @brief Gets the next node pointer (for extension nodes, const version).
     * @return The next node pointer.
     */
    const std::unique_ptr<Node>& GetNextPtr() const;

    /**
     * @brief Sets the next node (for extension nodes).
     * @param next The next node.
     */
    void SetNext(std::unique_ptr<Node> next);

    // Leaf node accessors
    /**
     * @brief Gets the value (for leaf nodes).
     * @return The value.
     */
    std::span<const uint8_t> GetValue() const;

    /**
     * @brief Sets the value (for leaf nodes).
     * @param value The value.
     */
    void SetValue(std::span<const uint8_t> value);

    // Hash node accessors
    /**
     * @brief Gets the stored hash (for hash nodes).
     * @return The stored hash.
     */
    const io::UInt256& GetStoredHash() const;

  private:
    NodeType type_;
    mutable io::UInt256 hash_;
    mutable bool hash_dirty_;
    int reference_;

    // Branch node data
    std::vector<std::unique_ptr<Node>> children_;

    // Extension node data
    io::ByteVector key_;
    std::unique_ptr<Node> next_;

    // Leaf node data
    io::ByteVector value_;

    // Hash node data
    io::UInt256 stored_hash_;

    /**
     * @brief Calculates the hash of this node.
     * @return The calculated hash.
     */
    io::UInt256 CalculateHash() const;

    /**
     * @brief Serializes branch node data.
     * @param writer The binary writer.
     */
    void SerializeBranch(io::BinaryWriter& writer) const;

    /**
     * @brief Deserializes branch node data.
     * @param reader The binary reader.
     */
    void DeserializeBranch(io::BinaryReader& reader);

    /**
     * @brief Serializes extension node data.
     * @param writer The binary writer.
     */
    void SerializeExtension(io::BinaryWriter& writer) const;

    /**
     * @brief Deserializes extension node data.
     * @param reader The binary reader.
     */
    void DeserializeExtension(io::BinaryReader& reader);

    /**
     * @brief Serializes leaf node data.
     * @param writer The binary writer.
     */
    void SerializeLeaf(io::BinaryWriter& writer) const;

    /**
     * @brief Deserializes leaf node data.
     * @param reader The binary reader.
     */
    void DeserializeLeaf(io::BinaryReader& reader);

    /**
     * @brief Gets the size of branch node data.
     * @return The size in bytes.
     */
    size_t GetBranchSize() const;

    /**
     * @brief Gets the size of extension node data.
     * @return The size in bytes.
     */
    size_t GetExtensionSize() const;

    /**
     * @brief Gets the size of leaf node data.
     * @return The size in bytes.
     */
    size_t GetLeafSize() const;

    /**
     * @brief Gets the size of hash node data.
     * @return The size in bytes.
     */
    size_t GetHashSize() const;
};
}  // namespace neo::cryptography::mpttrie
