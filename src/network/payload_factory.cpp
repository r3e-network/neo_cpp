/**
 * @file payload_factory.cpp
 * @brief Factory pattern implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/block_payload.h>
#include <neo/network/p2p/payloads/filter_add_payload.h>
#include <neo/network/p2p/payloads/filter_clear_payload.h>
#include <neo/network/p2p/payloads/filter_load_payload.h>
#include <neo/network/p2p/payloads/get_addr_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_headers_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/mempool_payload.h>
#include <neo/network/p2p/payloads/not_found_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/reject_payload.h>
#include <neo/network/p2p/payloads/transaction_payload.h>
#include <neo/network/p2p/payloads/verack_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <neo/network/payload_factory.h>

namespace neo::network::p2p
{

std::shared_ptr<IPayload> PayloadFactory::Create(MessageCommand command)
{
    switch (command)
    {
        case MessageCommand::Version:
            return std::make_shared<payloads::VersionPayload>();
        case MessageCommand::Verack:
            return std::make_shared<payloads::VerAckPayload>();
        case MessageCommand::Addr:
            return std::make_shared<payloads::AddrPayload>();
        case MessageCommand::GetAddr:
            return std::make_shared<payloads::GetAddrPayload>();
        case MessageCommand::Ping:
        case MessageCommand::Pong:
            return std::make_shared<payloads::PingPayload>();
        case MessageCommand::Inv:
            return std::make_shared<payloads::InvPayload>();
        case MessageCommand::GetData:
            return std::make_shared<payloads::InvPayload>();
        case MessageCommand::GetBlocks:
            return std::make_shared<payloads::GetBlocksPayload>();
        case MessageCommand::GetHeaders:
            return std::make_shared<payloads::GetHeadersPayload>();
        case MessageCommand::Headers:
            return std::make_shared<payloads::HeadersPayload>();
        case MessageCommand::GetBlockByIndex:
            return std::make_shared<payloads::GetBlockByIndexPayload>();
        case MessageCommand::Block:
            return std::make_shared<payloads::BlockPayload>();
        case MessageCommand::Transaction:
            return std::make_shared<payloads::TransactionPayload>();
        case MessageCommand::Mempool:
            return std::make_shared<payloads::MempoolPayload>();
        case MessageCommand::NotFound:
            return std::make_shared<payloads::NotFoundPayload>();
        case MessageCommand::Reject:
            return std::make_shared<payloads::RejectPayload>();
        case MessageCommand::FilterLoad:
            return std::make_shared<payloads::FilterLoadPayload>();
        case MessageCommand::FilterAdd:
            return std::make_shared<payloads::FilterAddPayload>();
        case MessageCommand::FilterClear:
            return std::make_shared<payloads::FilterClearPayload>();
        default:
            LOG_WARNING("Unknown message command: {}", static_cast<uint8_t>(command));
            return nullptr;
    }
}

std::shared_ptr<IPayload> PayloadFactory::Create(PayloadType type)
{
    // Map payload type to message command and use the existing Create method
    MessageCommand command = MessageCommand::Version;  // Default

    switch (type)
    {
        case PayloadType::Version:
            command = MessageCommand::Version;
            break;
        case PayloadType::Verack:
            command = MessageCommand::Verack;
            break;
        case PayloadType::Addr:
            command = MessageCommand::Addr;
            break;
        case PayloadType::GetAddr:
            command = MessageCommand::GetAddr;
            break;
        case PayloadType::Ping:
            command = MessageCommand::Ping;
            break;
        case PayloadType::Pong:
            command = MessageCommand::Pong;
            break;
        case PayloadType::Inventory:
            command = MessageCommand::Inv;
            break;
        case PayloadType::GetData:
            command = MessageCommand::GetData;
            break;
        case PayloadType::GetBlocks:
            command = MessageCommand::GetBlocks;
            break;
        case PayloadType::GetHeaders:
            command = MessageCommand::GetHeaders;
            break;
        case PayloadType::Headers:
            command = MessageCommand::Headers;
            break;
        case PayloadType::GetBlockByIndex:
            command = MessageCommand::GetBlockByIndex;
            break;
        case PayloadType::Block:
            command = MessageCommand::Block;
            break;
        case PayloadType::Transaction:
            command = MessageCommand::Transaction;
            break;
        case PayloadType::Mempool:
            command = MessageCommand::Mempool;
            break;
        case PayloadType::NotFound:
            command = MessageCommand::NotFound;
            break;
        case PayloadType::Reject:
            command = MessageCommand::Reject;
            break;
        case PayloadType::FilterLoad:
            command = MessageCommand::FilterLoad;
            break;
        case PayloadType::FilterAdd:
            command = MessageCommand::FilterAdd;
            break;
        case PayloadType::FilterClear:
            command = MessageCommand::FilterClear;
            break;
        default:
            LOG_WARNING("Unknown payload type: {}", static_cast<uint8_t>(type));
            return nullptr;
    }

    return Create(command);
}

std::shared_ptr<IPayload> PayloadFactory::DeserializePayload(MessageCommand command, io::BinaryReader& reader)
{
    auto payload = Create(command);
    if (payload)
    {
        try
        {
            payload->Deserialize(reader);
            return payload;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to deserialize payload for command {}: {}", static_cast<uint8_t>(command), e.what());
            return nullptr;
        }
    }
    return nullptr;
}

std::shared_ptr<IPayload> PayloadFactory::DeserializePayload(PayloadType type, io::BinaryReader& reader)
{
    auto payload = Create(type);
    if (payload)
    {
        try
        {
            payload->Deserialize(reader);
            return payload;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to deserialize payload for type {}: {}", static_cast<uint8_t>(type), e.what());
            return nullptr;
        }
    }
    return nullptr;
}

}  // namespace neo::network::p2p