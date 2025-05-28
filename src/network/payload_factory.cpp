#include <neo/network/payload_factory.h>
#include <neo/network/payloads/version_payload.h>
#include <neo/network/payloads/inventory_payload.h>
#include <neo/network/payloads/addr_payload.h>
#include <neo/network/payloads/ping_payload.h>
#include <neo/network/payloads/headers_payload.h>
#include <neo/network/payloads/get_blocks_payload.h>
#include <neo/network/payloads/get_block_by_index_payload.h>
#include <neo/network/payloads/transaction_payload.h>
#include <neo/network/payloads/block_payload.h>
#include <neo/network/payloads/consensus_payload.h>
#include <neo/network/payloads/filter_load_payload.h>
#include <neo/network/payloads/filter_add_payload.h>
#include <neo/network/payloads/merkle_block_payload.h>
#include <neo/network/payloads/get_addr_payload.h>
#include <neo/network/p2p/payloads/extensible_payload.h>
#include <neo/network/p2p/payloads/merkle_block_payload.h>
#include <neo/network/p2p/payloads/oracle_response.h>
#include <stdexcept>

namespace neo::network::p2p
{
    std::shared_ptr<IPayload> PayloadFactory::Create(MessageCommand command)
    {
        std::shared_ptr<IPayload> payload;

        switch (command)
        {
            case MessageCommand::Version:
                payload = std::make_shared<network::payloads::VersionPayload>();
                break;
            case MessageCommand::Inv:
            case MessageCommand::GetData:
            case MessageCommand::NotFound:
                payload = std::make_shared<network::payloads::InventoryPayload>();
                break;
            case MessageCommand::Addr:
                payload = std::make_shared<network::payloads::AddrPayload>();
                break;
            case MessageCommand::Ping:
            case MessageCommand::Pong:
                payload = std::make_shared<network::payloads::PingPayload>();
                break;
            case MessageCommand::Headers:
                payload = std::make_shared<network::payloads::HeadersPayload>();
                break;
            case MessageCommand::GetBlocks:
                payload = std::make_shared<network::payloads::GetBlocksPayload>();
                break;
            case MessageCommand::GetBlockByIndex:
                payload = std::make_shared<network::payloads::GetBlockByIndexPayload>();
                break;
            case MessageCommand::Transaction:
                payload = std::make_shared<network::payloads::TransactionPayload>();
                break;
            case MessageCommand::Block:
                payload = std::make_shared<network::payloads::BlockPayload>();
                break;
            case MessageCommand::Consensus:
                payload = std::make_shared<network::payloads::ConsensusPayload>();
                break;
            case MessageCommand::FilterLoad:
                payload = std::make_shared<network::payloads::FilterLoadPayload>();
                break;
            case MessageCommand::FilterAdd:
                payload = std::make_shared<network::payloads::FilterAddPayload>();
                break;
            case MessageCommand::MerkleBlock:
                payload = std::make_shared<payloads::MerkleBlockPayload>();
                break;
            case MessageCommand::GetAddr:
                payload = std::make_shared<network::payloads::GetAddrPayload>();
                break;
            case MessageCommand::Extensible:
                payload = std::make_shared<payloads::ExtensiblePayload>();
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
                payload = std::make_shared<network::payloads::VersionPayload>();
                break;
            case PayloadType::Inventory:
            case PayloadType::GetData:
            case PayloadType::NotFound:
                payload = std::make_shared<network::payloads::InventoryPayload>();
                break;
            case PayloadType::Addr:
                payload = std::make_shared<network::payloads::AddrPayload>();
                break;
            case PayloadType::Ping:
            case PayloadType::Pong:
                payload = std::make_shared<network::payloads::PingPayload>();
                break;
            case PayloadType::Headers:
                payload = std::make_shared<network::payloads::HeadersPayload>();
                break;
            case PayloadType::GetBlocks:
                payload = std::make_shared<network::payloads::GetBlocksPayload>();
                break;
            case PayloadType::GetBlockByIndex:
                payload = std::make_shared<network::payloads::GetBlockByIndexPayload>();
                break;
            case PayloadType::Transaction:
                payload = std::make_shared<network::payloads::TransactionPayload>();
                break;
            case PayloadType::Block:
                payload = std::make_shared<network::payloads::BlockPayload>();
                break;
            case PayloadType::Consensus:
                payload = std::make_shared<network::payloads::ConsensusPayload>();
                break;
            case PayloadType::FilterLoad:
                payload = std::make_shared<network::payloads::FilterLoadPayload>();
                break;
            case PayloadType::FilterAdd:
                payload = std::make_shared<network::payloads::FilterAddPayload>();
                break;
            case PayloadType::MerkleBlock:
                payload = std::make_shared<payloads::MerkleBlockPayload>();
                break;
            case PayloadType::GetAddr:
                payload = std::make_shared<network::payloads::GetAddrPayload>();
                break;
            case PayloadType::Extensible:
                payload = std::make_shared<payloads::ExtensiblePayload>();
                break;
            case PayloadType::FilterClear:
                // FilterClear payload has no data, so we can use a simple IPayload implementation
                payload = std::make_shared<network::payloads::FilterAddPayload>();
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
}
