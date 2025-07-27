#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/block_payload.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/network/p2p/payloads/filter_add_payload.h>
#include <neo/network/p2p/payloads/filter_clear_payload.h>
#include <neo/network/p2p/payloads/filter_load_payload.h>
#include <neo/network/p2p/payloads/get_addr_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/payloads/get_headers_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/inventory_payload.h>
#include <neo/network/p2p/payloads/mempool_payload.h>
#include <neo/network/p2p/payloads/merkle_block_payload.h>
#include <neo/network/p2p/payloads/not_found_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/reject_payload.h>
#include <neo/network/p2p/payloads/transaction_payload.h>
#include <neo/network/p2p/payloads/verack_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/payload_factory.h>
#include <stdexcept>

namespace neo::network::p2p
{
std::shared_ptr<IPayload> PayloadFactory::Create(MessageCommand command)
{
    std::shared_ptr<IPayload> payload;

    switch (command)
    {
        case MessageCommand::Version:
            payload = std::make_shared<payloads::VersionPayload>();
            break;
        case MessageCommand::Verack:
            payload = std::make_shared<payloads::VerackPayload>();
            break;
        case MessageCommand::Inv:
            payload = std::make_shared<payloads::InvPayload>();
            break;
        case MessageCommand::GetData:
            payload = std::make_shared<payloads::GetDataPayload>();
            break;
        case MessageCommand::Addr:
            payload = std::make_shared<payloads::AddrPayload>();
            break;
        case MessageCommand::GetAddr:
            payload = std::make_shared<payloads::GetAddrPayload>();
            break;
        case MessageCommand::Ping:
        case MessageCommand::Pong:
            payload = std::make_shared<payloads::PingPayload>();
            break;
        case MessageCommand::GetHeaders:
            payload = std::make_shared<payloads::GetHeadersPayload>();
            break;
        case MessageCommand::Headers:
            payload = std::make_shared<payloads::HeadersPayload>();
            break;
        case MessageCommand::GetBlocks:
            payload = std::make_shared<payloads::GetBlocksPayload>();
            break;
        case MessageCommand::GetBlockByIndex:
            payload = std::make_shared<payloads::GetBlockByIndexPayload>();
            break;
        case MessageCommand::Mempool:
            payload = std::make_shared<payloads::MempoolPayload>();
            break;
        case MessageCommand::NotFound:
            payload = std::make_shared<payloads::NotFoundPayload>();
            break;
        case MessageCommand::Transaction:
            payload = std::make_shared<payloads::TransactionPayload>();
            break;
        case MessageCommand::Block:
            payload = std::make_shared<payloads::BlockPayload>();
            break;
        case MessageCommand::Extensible:
            payload = std::make_shared<payloads::ExtensiblePayload>();
            break;
        case MessageCommand::Reject:
            payload = std::make_shared<payloads::RejectPayload>();
            break;
        case MessageCommand::FilterLoad:
            payload = std::make_shared<payloads::FilterLoadPayload>();
            break;
        case MessageCommand::FilterAdd:
            payload = std::make_shared<payloads::FilterAddPayload>();
            break;
        case MessageCommand::FilterClear:
            payload = std::make_shared<payloads::FilterClearPayload>();
            break;
        case MessageCommand::MerkleBlock:
            payload = std::make_shared<payloads::MerkleBlockPayload>();
            break;
        default:
            return nullptr;
    }

    return payload;
}

std::shared_ptr<IPayload> PayloadFactory::Create(PayloadType type)
{
    std::shared_ptr<IPayload> payload;

    switch (type)
    {
        case PayloadType::Version:
            payload = std::make_shared<payloads::VersionPayload>();
            break;
        case PayloadType::Verack:
            payload = std::make_shared<payloads::VerackPayload>();
            break;
        case PayloadType::Inventory:
            payload = std::make_shared<payloads::InvPayload>();
            break;
        case PayloadType::GetData:
            payload = std::make_shared<payloads::GetDataPayload>();
            break;
        case PayloadType::Addr:
            payload = std::make_shared<payloads::AddrPayload>();
            break;
        case PayloadType::GetAddr:
            payload = std::make_shared<payloads::GetAddrPayload>();
            break;
        case PayloadType::Ping:
        case PayloadType::Pong:
            payload = std::make_shared<payloads::PingPayload>();
            break;
        case PayloadType::GetHeaders:
            payload = std::make_shared<payloads::GetHeadersPayload>();
            break;
        case PayloadType::Headers:
            payload = std::make_shared<payloads::HeadersPayload>();
            break;
        case PayloadType::GetBlocks:
            payload = std::make_shared<payloads::GetBlocksPayload>();
            break;
        case PayloadType::GetBlockByIndex:
            payload = std::make_shared<payloads::GetBlockByIndexPayload>();
            break;
        case PayloadType::Transaction:
            payload = std::make_shared<payloads::TransactionPayload>();
            break;
        case PayloadType::Block:
            payload = std::make_shared<payloads::BlockPayload>();
            break;
        case PayloadType::Extensible:
            payload = std::make_shared<payloads::ExtensiblePayload>();
            break;
        case PayloadType::FilterLoad:
            payload = std::make_shared<payloads::FilterLoadPayload>();
            break;
        case PayloadType::FilterAdd:
            payload = std::make_shared<payloads::FilterAddPayload>();
            break;
        case PayloadType::FilterClear:
            payload = std::make_shared<payloads::FilterClearPayload>();
            break;
        case PayloadType::MerkleBlock:
            payload = std::make_shared<payloads::MerkleBlockPayload>();
            break;
        case PayloadType::Mempool:
            payload = std::make_shared<payloads::MempoolPayload>();
            break;
        default:
            throw std::invalid_argument("Invalid payload type");
    }

    return payload;
}

std::shared_ptr<IPayload> PayloadFactory::DeserializePayload(MessageCommand command, io::BinaryReader& reader)
{
    auto payload = Create(command);
    if (payload)
    {
        payload->Deserialize(reader);
    }
    return payload;
}

std::shared_ptr<IPayload> PayloadFactory::DeserializePayload(PayloadType type, io::BinaryReader& reader)
{
    auto payload = Create(type);
    if (payload)
    {
        payload->Deserialize(reader);
    }
    return payload;
}
}  // namespace neo::network::p2p
