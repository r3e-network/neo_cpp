#include <neo/cryptography/base64.h>
#include <neo/io/binary_writer.h>
#include <neo/rpc/rpc_methods.h>

namespace neo::rpc
{
using json = nlohmann::json;

nlohmann::json RPCMethods::GetVersion(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result;
    result["tcpport"] = 10333;
    result["wsport"] = 10334;
    result["nonce"] = 12345;
    result["useragent"] = "/NEO:3.0.0/";

    json protocol;
    protocol["addressversion"] = 53;
    protocol["network"] = 894710606;
    protocol["validatorscount"] = 7;
    protocol["msperblock"] = 15000;
    protocol["maxtraceableblocks"] = 2102400;
    protocol["maxvaliduntilblockincrement"] = 86400;
    protocol["maxtransactionsperblock"] = 512;
    protocol["memorypoolmaxtransactions"] = 50000;
    protocol["initialgasdistribution"] = 5200000000000000;

    result["protocol"] = protocol;
    return result;
}

nlohmann::json RPCMethods::GetBlockCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }
    return blockchain->GetCurrentBlockIndex() + 1;
}

nlohmann::json RPCMethods::GetBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing block identifier parameter");
    }
    return nullptr;
}

nlohmann::json RPCMethods::GetBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing block index parameter");
    }

    uint32_t index = params[0].get<uint32_t>();
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }

    auto hash = blockchain->GetBlockHash(index);
    return hash.ToString();
}

nlohmann::json RPCMethods::GetBlockHeader(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing block identifier parameter");
    }
    return nullptr;
}

nlohmann::json RPCMethods::GetRawMemPool(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto memPool = neoSystem->GetMemPool();
    if (!memPool)
    {
        throw std::runtime_error("Memory pool not available");
    }

    json result = json::array();
    // MemoryPool would need to expose transaction hashes
    return result;
}

nlohmann::json RPCMethods::GetRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing transaction hash parameter");
    }
    return nullptr;
}

nlohmann::json RPCMethods::GetTransactionHeight(std::shared_ptr<node::NeoSystem> neoSystem,
                                                const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing transaction hash parameter");
    }
    return nullptr;
}

nlohmann::json RPCMethods::SendRawTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing transaction data parameter");
    }
    return false;
}

nlohmann::json RPCMethods::InvokeFunction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }

    json result;
    result["script"] = "00";
    result["state"] = "HALT";
    result["gasconsumed"] = "0";
    result["stack"] = json::array();

    return result;
}

nlohmann::json RPCMethods::InvokeScript(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing script parameter");
    }

    json result;
    result["script"] = params[0];
    result["state"] = "HALT";
    result["gasconsumed"] = "0";
    result["stack"] = json::array();

    return result;
}

nlohmann::json RPCMethods::GetContractState(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing contract hash parameter");
    }
    return nullptr;
}

nlohmann::json RPCMethods::GetUnclaimedGas(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing account parameter");
    }
    return "0";
}

nlohmann::json RPCMethods::GetConnectionCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto localNode = neoSystem->GetLocalNode();
    if (!localNode)
    {
        return 0;
    }
    return localNode->GetConnectedPeersCount();
}

nlohmann::json RPCMethods::GetPeers(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result;
    result["unconnected"] = json::array();
    result["bad"] = json::array();
    result["connected"] = json::array();

    auto localNode = neoSystem->GetLocalNode();
    if (localNode)
    {
        // LocalNode would need to expose peer information
    }

    return result;
}

nlohmann::json RPCMethods::GetCommittee(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result = json::array();
    // Would need to query NeoToken native contract
    return result;
}

nlohmann::json RPCMethods::GetValidators(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result = json::array();
    // Would need to query NeoToken native contract
    return result;
}

nlohmann::json RPCMethods::GetNextBlockValidators(std::shared_ptr<node::NeoSystem> neoSystem,
                                                  const nlohmann::json& params)
{
    json result = json::array();
    // Would need to query NeoToken native contract
    return result;
}

nlohmann::json RPCMethods::GetBestBlockHash(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }

    auto currentIndex = blockchain->GetCurrentBlockIndex();
    auto hash = blockchain->GetBlockHash(currentIndex);
    return hash.ToString();
}

nlohmann::json RPCMethods::GetBlockHeaderCount(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }
    return blockchain->GetCurrentBlockIndex() + 1;
}

nlohmann::json RPCMethods::GetStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }
    return nullptr;
}

nlohmann::json RPCMethods::FindStorage(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }

    json result;
    result["results"] = json::array();
    result["truncated"] = false;

    return result;
}

nlohmann::json RPCMethods::GetCandidates(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result = json::array();
    // Would need to query NeoToken native contract
    return result;
}

nlohmann::json RPCMethods::GetNativeContracts(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result = json::array();

    // Add hardcoded native contracts
    json neoContract;
    neoContract["id"] = 2;
    neoContract["hash"] = "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5";
    neoContract["nef"] = json::object();
    neoContract["manifest"] = json::object();
    neoContract["manifest"]["name"] = "NeoToken";
    result.push_back(neoContract);

    json gasContract;
    gasContract["id"] = -6;
    gasContract["hash"] = "0xd2a4cff31913016155e38e474a2c06d08be276cf";
    gasContract["nef"] = json::object();
    gasContract["manifest"] = json::object();
    gasContract["manifest"]["name"] = "GasToken";
    result.push_back(gasContract);

    return result;
}

nlohmann::json RPCMethods::SubmitBlock(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing block data parameter");
    }
    return false;
}

nlohmann::json RPCMethods::ValidateAddress(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing address parameter");
    }

    json result;
    result["address"] = params[0];
    result["isvalid"] = false;

    try
    {
        auto address = params[0].get<std::string>();
        // Basic validation - check if it can be parsed
        if (address.length() == 34 && address[0] == 'N')
        {
            result["isvalid"] = true;
        }
    }
    catch (...)
    {
        // Invalid address
    }

    return result;
}

nlohmann::json RPCMethods::TraverseIterator(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }

    json result;
    result["values"] = json::array();
    result["truncated"] = false;

    return result;
}

nlohmann::json RPCMethods::TerminateSession(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing session ID parameter");
    }
    return true;
}

nlohmann::json RPCMethods::InvokeContractVerify(std::shared_ptr<node::NeoSystem> neoSystem,
                                                const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing contract hash parameter");
    }

    json result;
    result["script"] = "00";
    result["state"] = "HALT";
    result["gasconsumed"] = "0";
    result["stack"] = json::array();

    return result;
}

// Private helper methods
nlohmann::json RPCMethods::BlockToJson(std::shared_ptr<ledger::Block> block, bool verbose)
{
    if (!block)
        return nullptr;

    if (verbose)
    {
        json result;
        result["hash"] = block->GetHash().ToString();
        result["size"] = block->GetSize();
        result["version"] = block->GetVersion();
        result["previousblockhash"] = block->GetPreviousHash().ToString();
        result["merkleroot"] = block->GetMerkleRoot().ToString();
        result["time"] = block->GetTimestamp().time_since_epoch().count();
        result["index"] = block->GetIndex();
        result["primary"] = block->GetPrimaryIndex();
        result["nextconsensus"] = block->GetNextConsensus().ToString();

        json tx = json::array();
        for (const auto& transaction : block->GetTransactions())
        {
            tx.push_back(transaction.GetHash().ToString());
        }
        result["tx"] = tx;

        return result;
    }
    else
    {
        std::stringstream ss;
        io::BinaryWriter writer(ss);
        block->Serialize(writer);
        std::string blockData = ss.str();

        return cryptography::Base64::Encode(
            io::ByteSpan(reinterpret_cast<const uint8_t*>(blockData.data()), blockData.size()));
    }
}

nlohmann::json RPCMethods::TransactionToJson(std::shared_ptr<ledger::Transaction> tx, bool verbose)
{
    if (!tx)
        return nullptr;

    if (verbose)
    {
        json result;
        result["hash"] = tx->GetHash().ToString();
        result["size"] = tx->GetSize();
        result["version"] = tx->GetVersion();
        return result;
    }
    else
    {
        std::stringstream ss;
        io::BinaryWriter writer(ss);
        tx->Serialize(writer);
        std::string txData = ss.str();

        return cryptography::Base64::Encode(
            io::ByteSpan(reinterpret_cast<const uint8_t*>(txData.data()), txData.size()));
    }
}

nlohmann::json RPCMethods::ContractToJson(std::shared_ptr<smartcontract::ContractState> contract)
{
    if (!contract)
        return nullptr;

    json result;
    result["id"] = contract->GetId();
    result["updatecounter"] = contract->GetUpdateCounter();
    result["hash"] = contract->GetScriptHash().ToString();

    // NEF and manifest would need proper serialization
    result["nef"] = json::object();
    result["manifest"] = json::object();

    return result;
}
}  // namespace neo::rpc