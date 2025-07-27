#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <neo/cryptography/mpttrie/cache.h>
#include <neo/cryptography/mpttrie/node.h>
#include <neo/io/byte_span.h>
#include <neo/io/uint256.h>
#include <neo/persistence/istore.h>
#include <vector>

namespace neo::cryptography::mpttrie
{
/**
 * @brief Merkle Patricia Trie implementation.
 *
 * This class provides a production-ready implementation of the Merkle Patricia Trie
 * data structure used in Neo blockchain for state storage. The MPT combines the
 * efficiency of Patricia tries with the cryptographic integrity of Merkle trees.
 *
 * Key features:
 * - Efficient key-value storage with cryptographic proofs
 * - Reference counting to avoid duplicate node storage
 * - Cache layer for optimal performance
 * - Support for full or latest state modes
 * - Merkle proof generation for light client verification
 */
class Trie
{
  public:
    /**
     * @brief Constructs a Trie with the given store and root.
     * @param store The store snapshot for persistence.
     * @param root The root hash of the trie.
     * @param full_state If true, keeps all historical state. If false, only latest state.
     */
    Trie(std::shared_ptr<persistence::IStoreSnapshot> store, io::UInt256 root, bool full_state = false);

    /**
     * @brief Sets the root hash of the trie.
     * @param root_hash The new root hash.
     */
    void SetRoot(const io::UInt256& root_hash);

    /**
     * @brief Gets the root hash of the trie.
     * @return The root hash.
     */
    io::UInt256 GetRootHash() const;

    /**
     * @brief Gets a value by key.
     * @param key The key.
     * @return The value.
     * @throws std::out_of_range if key not found.
     */
    io::ByteVector Get(io::ByteSpan key) const;

    /**
     * @brief Tries to get a value by key.
     * @param key The key.
     * @param value Output parameter for the value.
     * @return True if found, false otherwise.
     */
    bool TryGet(io::ByteSpan key, io::ByteVector& value) const;

    /**
     * @brief Puts a key-value pair.
     * @param key The key.
     * @param value The value.
     */
    void Put(io::ByteSpan key, io::ByteSpan value);

    /**
     * @brief Deletes a key.
     * @param key The key.
     * @return True if deleted, false if not found.
     */
    bool Delete(io::ByteSpan key);

    /**
     * @brief Commits all pending changes to the store.
     */
    void Commit();

    /**
     * @brief Generates a Merkle proof for the given key.
     * @param key The key to generate proof for.
     * @return Vector of proof data (serialized nodes).
     */
    std::vector<io::ByteVector> GetProof(io::ByteSpan key) const;

  private:
    std::shared_ptr<persistence::IStoreSnapshot> store_;
    std::unique_ptr<Cache> cache_;
    std::unique_ptr<Node> root_;
    bool full_state_;

    /**
     * @brief Converts a key to nibbles (4-bit values).
     * @param key The key.
     * @return The nibbles.
     */
    static std::vector<uint8_t> ToNibbles(io::ByteSpan key);

    /**
     * @brief Creates a storage key from hash.
     * @param hash The hash.
     * @return The storage key.
     */
    static io::ByteVector CreateKey(const io::UInt256& hash);

    /**
     * @brief Internal get implementation.
     * @param node The current node.
     * @param path The remaining path.
     * @param value Output parameter for the value.
     * @return True if found, false otherwise.
     */
    bool TryGetInternal(Node& node, io::ByteSpan path, io::ByteVector& value) const;

    /**
     * @brief Internal put implementation.
     * @param node The current node.
     * @param path The remaining path.
     * @param value_node The node containing the value to insert.
     */
    void PutInternal(std::unique_ptr<Node>& node, io::ByteSpan path, std::unique_ptr<Node> value_node);

    /**
     * @brief Put implementation for branch nodes.
     * @param node The branch node.
     * @param path The remaining path.
     * @param value_node The value node to insert.
     */
    void PutBranch(std::unique_ptr<Node>& node, io::ByteSpan path, std::unique_ptr<Node> value_node);

    /**
     * @brief Put implementation for extension nodes.
     * @param node The extension node.
     * @param path The remaining path.
     * @param value_node The value node to insert.
     */
    void PutExtension(std::unique_ptr<Node>& node, io::ByteSpan path, std::unique_ptr<Node> value_node);

    /**
     * @brief Put implementation for leaf nodes.
     * @param node The leaf node.
     * @param path The remaining path.
     * @param value_node The value node to insert.
     */
    void PutLeaf(std::unique_ptr<Node>& node, io::ByteSpan path, std::unique_ptr<Node> value_node);

    /**
     * @brief Internal delete implementation.
     * @param node The current node.
     * @param path The remaining path.
     * @return True if deleted, false otherwise.
     */
    bool TryDeleteInternal(std::unique_ptr<Node>& node, io::ByteSpan path);

    /**
     * @brief Delete implementation for branch nodes.
     * @param node The branch node.
     * @param path The remaining path.
     * @return True if deleted, false otherwise.
     */
    bool DeleteBranch(std::unique_ptr<Node>& node, io::ByteSpan path);

    /**
     * @brief Delete implementation for extension nodes.
     * @param node The extension node.
     * @param path The remaining path.
     * @return True if deleted, false otherwise.
     */
    bool DeleteExtension(std::unique_ptr<Node>& node, io::ByteSpan path);

    /**
     * @brief Simplifies a branch node if it has only one child.
     * @param node The branch node to simplify.
     */
    void SimplifyBranch(std::unique_ptr<Node>& node);

    /**
     * @brief Internal proof generation implementation.
     * @param node The current node.
     * @param path The remaining path.
     * @param proof The proof accumulator.
     * @return True if path exists, false otherwise.
     */
    bool GetProofInternal(Node& node, io::ByteSpan path, std::vector<io::ByteVector>& proof) const;

    /**
     * @brief Calculates the common prefix length between two byte spans.
     * @param a First byte span.
     * @param b Second byte span.
     * @return Length of common prefix.
     */
    static size_t CommonPrefixLength(io::ByteSpan a, io::ByteSpan b);
};
}  // namespace neo::cryptography::mpttrie
