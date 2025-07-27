#include <neo/core/logging.h>
#include <neo/network/p2p/protocol_handler.h>

namespace neo::network::p2p
{
ProtocolHandler::ProtocolHandler(const Config& config, std::shared_ptr<persistence::DataCache> blockchain,
                                 std::shared_ptr<ledger::MemoryPool> mempool)
    : config_(config), blockchain_(blockchain), mempool_(mempool)
{
}

// Destructor is implicitly defined

void ProtocolHandler::OnPeerConnected(const io::UInt256& peer_id, bool is_outbound)
{
    LOG_DEBUG("OnPeerConnected stub: peer={} outbound={}", peer_id.ToString(), is_outbound);
}

void ProtocolHandler::OnPeerDisconnected(const io::UInt256& peer_id)
{
    LOG_DEBUG("OnPeerDisconnected stub: peer={}", peer_id.ToString());
}

void ProtocolHandler::HandleMessage(const io::UInt256& peer_id, const Message& message)
{
    LOG_DEBUG("HandleMessage stub: peer={} command={}", peer_id.ToString(), static_cast<uint8_t>(message.GetCommand()));
}

void ProtocolHandler::SendHandshake(const io::UInt256& peer_id)
{
    LOG_DEBUG("SendHandshake stub: peer={}", peer_id.ToString());
}

void ProtocolHandler::RequestBlocks(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes)
{
    LOG_DEBUG("RequestBlocks stub: peer={} count={}", peer_id.ToString(), hashes.size());
}

void ProtocolHandler::RequestTransactions(const io::UInt256& peer_id, const std::vector<io::UInt256>& hashes)
{
    LOG_DEBUG("RequestTransactions stub: peer={} count={}", peer_id.ToString(), hashes.size());
}

void ProtocolHandler::BroadcastBlock(const ledger::Block& block)
{
    LOG_DEBUG("BroadcastBlock stub");
}

void ProtocolHandler::BroadcastTransaction(const ledger::Transaction& transaction)
{
    LOG_DEBUG("BroadcastTransaction stub");
}

bool ProtocolHandler::IsSynchronized() const
{
    return true;  // Stub - always synchronized
}

size_t ProtocolHandler::GetHandshakedPeerCount() const
{
    return 0;  // Stub
}

// Private method stubs
void ProtocolHandler::HandleVersion(const io::UInt256& peer_id, const payloads::VersionPayload& payload)
{
    LOG_DEBUG("HandleVersion stub");
}

void ProtocolHandler::HandleVerack(const io::UInt256& peer_id)
{
    LOG_DEBUG("HandleVerack stub");
}

void ProtocolHandler::HandleGetAddr(const io::UInt256& peer_id)
{
    LOG_DEBUG("HandleGetAddr stub");
}

void ProtocolHandler::HandleAddr(const io::UInt256& peer_id, const payloads::AddrPayload& payload)
{
    LOG_DEBUG("HandleAddr stub");
}

void ProtocolHandler::HandlePing(const io::UInt256& peer_id, const payloads::PingPayload& payload)
{
    LOG_DEBUG("HandlePing stub");
}

void ProtocolHandler::HandlePong(const io::UInt256& peer_id, const payloads::PingPayload& payload)
{
    LOG_DEBUG("HandlePong stub");
}

void ProtocolHandler::HandleGetHeaders(const io::UInt256& peer_id, const payloads::GetHeadersPayload& payload)
{
    LOG_DEBUG("HandleGetHeaders stub");
}

void ProtocolHandler::HandleHeaders(const io::UInt256& peer_id, const payloads::HeadersPayload& payload)
{
    LOG_DEBUG("HandleHeaders stub");
}

void ProtocolHandler::HandleGetBlocks(const io::UInt256& peer_id, const payloads::GetBlocksPayload& payload)
{
    LOG_DEBUG("HandleGetBlocks stub");
}

void ProtocolHandler::HandleGetData(const io::UInt256& peer_id, const payloads::InvPayload& payload)
{
    LOG_DEBUG("HandleGetData stub");
}

void ProtocolHandler::HandleGetBlockByIndex(const io::UInt256& peer_id, const payloads::GetBlockByIndexPayload& payload)
{
    LOG_DEBUG("HandleGetBlockByIndex stub");
}

void ProtocolHandler::HandleInv(const io::UInt256& peer_id, const payloads::InvPayload& payload)
{
    LOG_DEBUG("HandleInv stub");
}

void ProtocolHandler::HandleBlock(const io::UInt256& peer_id, const payloads::BlockPayload& payload)
{
    LOG_DEBUG("HandleBlock stub");
}

void ProtocolHandler::HandleTransaction(const io::UInt256& peer_id, const payloads::TransactionPayload& payload)
{
    LOG_DEBUG("HandleTransaction stub");
}

void ProtocolHandler::HandleMempool(const io::UInt256& peer_id)
{
    LOG_DEBUG("HandleMempool stub");
}

void ProtocolHandler::HandleNotFound(const io::UInt256& peer_id, const payloads::InvPayload& payload)
{
    LOG_DEBUG("HandleNotFound stub");
}

std::vector<io::UInt256> ProtocolHandler::GetRelayPeers(const io::UInt256& exclude_peer) const
{
    return std::vector<io::UInt256>();  // Stub - no peers
}

void ProtocolHandler::RelayInventory(payloads::InventoryType type, const io::UInt256& hash,
                                     const io::UInt256& source_peer)
{
    LOG_DEBUG("RelayInventory stub: type={} hash={}", static_cast<uint8_t>(type), hash.ToString());
}

// SendReject is not in the public interface

}  // namespace neo::network::p2p