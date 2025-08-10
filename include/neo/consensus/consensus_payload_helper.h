#pragma once

#include <neo/consensus/consensus_message.h>
#include <neo/cryptography/witness.h>
#include <neo/io/uint160.h>
#include <neo/network/p2p/payloads/extensible_payload.h>

#include <memory>
#include <string>

namespace neo::consensus
{
/**
 * @brief Helper class for working with consensus messages in ExtensiblePayload.
 *
 * In Neo N3, consensus messages are wrapped in ExtensiblePayload with
 * category "dBFT" for transmission over the network.
 */
class ConsensusPayloadHelper
{
   public:
    /**
     * @brief The category name for consensus messages.
     */
    static constexpr const char* CONSENSUS_CATEGORY = "dBFT";

    /**
     * @brief Creates an ExtensiblePayload containing a consensus message.
     * @param message The consensus message to wrap.
     * @param sender The sender's script hash.
     * @param validBlockStart The starting block for validity.
     * @param validBlockEnd The ending block for validity.
     * @return The created ExtensiblePayload.
     */
    static std::shared_ptr<network::p2p::payloads::ExtensiblePayload> CreatePayload(
        std::shared_ptr<ConsensusMessage> message, const io::UInt160& sender, uint32_t validBlockStart,
        uint32_t validBlockEnd);

    /**
     * @brief Extracts a consensus message from an ExtensiblePayload.
     * @param payload The ExtensiblePayload to extract from.
     * @return The extracted consensus message, or nullptr if not a consensus payload.
     */
    static std::shared_ptr<ConsensusMessage> GetMessage(const network::p2p::payloads::ExtensiblePayload& payload);

    /**
     * @brief Checks if an ExtensiblePayload contains a consensus message.
     * @param payload The ExtensiblePayload to check.
     * @return True if it contains a consensus message, false otherwise.
     */
    static bool IsConsensusPayload(const network::p2p::payloads::ExtensiblePayload& payload);

    /**
     * @brief Signs an ExtensiblePayload containing a consensus message.
     * @param payload The payload to sign.
     * @param witness The witness containing the signature.
     */
    static void SignPayload(network::p2p::payloads::ExtensiblePayload& payload, const cryptography::Witness& witness);
};
}  // namespace neo::consensus