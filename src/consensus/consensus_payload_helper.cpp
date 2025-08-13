/**
 * @file consensus_payload_helper.cpp
 * @brief Helper utilities
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/consensus_payload_helper.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>

namespace neo::consensus
{
std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusPayloadHelper::CreatePayload(
    std::shared_ptr<ConsensusMessage> message, const io::UInt160& sender, uint32_t validBlockStart,
    uint32_t validBlockEnd)
{
    // Serialize the consensus message
    io::MemoryStream stream;
    io::BinaryWriter writer(stream);
    message->Serialize(writer);

    // Create ExtensiblePayload with consensus message data
    auto payload = std::make_shared<network::p2p::payloads::ExtensiblePayload>(
        CONSENSUS_CATEGORY, validBlockStart, validBlockEnd, sender, stream.ToArray(),
        cryptography::Witness()  // Witness will be set later
    );

    return payload;
}

std::shared_ptr<ConsensusMessage> ConsensusPayloadHelper::GetMessage(
    const network::p2p::payloads::ExtensiblePayload& payload)
{
    if (!IsConsensusPayload(payload)) return nullptr;

    return ConsensusMessage::DeserializeFrom(payload.GetData());
}

bool ConsensusPayloadHelper::IsConsensusPayload(const network::p2p::payloads::ExtensiblePayload& payload)
{
    return payload.GetCategory() == CONSENSUS_CATEGORY;
}

void ConsensusPayloadHelper::SignPayload(network::p2p::payloads::ExtensiblePayload& payload,
                                         const cryptography::Witness& witness)
{
    payload.SetWitness(witness);
}
}  // namespace neo::consensus