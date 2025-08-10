#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/witness.h>
#include <neo/ledger/block.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/persistence/data_cache.h>

#include <cstdint>
#include <memory>

namespace neo::sign
{
/**
 * @brief Interface for signing operations in Neo.
 *
 * This interface provides methods for signing blocks and
 * extensible payloads, which is required for consensus operations.
 */
class ISigner
{
   public:
    virtual ~ISigner() = default;

    /**
     * @brief Checks if the signer contains a signable key for the given public key.
     * @param publicKey The public key to check.
     * @return True if the signer can sign for this public key.
     */
    virtual bool ContainsSignable(const cryptography::ecc::ECPoint& publicKey) const = 0;

    /**
     * @brief Signs a block.
     * @param block The block to sign.
     * @param publicKey The public key to sign with.
     * @param network The network magic.
     * @return The signature.
     */
    virtual io::ByteVector SignBlock(std::shared_ptr<ledger::Block> block, const cryptography::ecc::ECPoint& publicKey,
                                     uint32_t network) const = 0;

    /**
     * @brief Signs an extensible payload.
     * @param payload The payload to sign.
     * @param snapshot The data cache snapshot.
     * @param network The network magic.
     * @return The witness containing the signature.
     */
    virtual cryptography::Witness SignExtensiblePayload(
        std::shared_ptr<network::p2p::payloads::ExtensiblePayload> payload,
        std::shared_ptr<persistence::DataCache> snapshot, uint32_t network) const = 0;
};
}  // namespace neo::sign