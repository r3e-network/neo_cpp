#pragma once

#include <neo/cryptography/mpttrie/node.h>
#include <neo/io/uint256.h>
#include <neo/persistence/istore.h>
#include <neo/persistence/store_view.h>

#include <memory>
#include <unordered_map>

namespace neo::cryptography::mpttrie
{
/**
 * @brief Cache for MPT nodes.
 */
class Cache
{
   public:
    /**
     * @brief Constructor.
     * @param store The store snapshot.
     * @param prefix The storage prefix.
     */
    Cache(std::shared_ptr<persistence::IStoreSnapshot> store, uint8_t prefix);

    /**
     * @brief Destructor.
     */
    ~Cache() = default;

    /**
     * @brief Resolves a node by hash.
     * @param hash The node hash.
     * @return The resolved node, or nullptr if not found.
     */
    std::unique_ptr<Node> Resolve(const io::UInt256& hash);

    /**
     * @brief Puts a node in the cache.
     * @param node The node to put.
     */
    void PutNode(std::unique_ptr<Node> node);

    /**
     * @brief Deletes a node from the cache.
     * @param hash The node hash.
     */
    void DeleteNode(const io::UInt256& hash);

    /**
     * @brief Commits all changes to the store.
     */
    void Commit();

   private:
    enum class TrackState : uint8_t
    {
        None,
        Added,
        Changed,
        Deleted
    };

    struct Trackable
    {
        std::unique_ptr<Node> node;
        TrackState state;
    };

    std::shared_ptr<persistence::IStoreSnapshot> store_;
    uint8_t prefix_;
    std::unordered_map<io::UInt256, std::unique_ptr<Trackable>> cache_;

    /**
     * @brief Creates a storage key from hash.
     * @param hash The hash.
     * @return The storage key.
     */
    io::ByteVector CreateKey(const io::UInt256& hash) const;
};
}  // namespace neo::cryptography::mpttrie
