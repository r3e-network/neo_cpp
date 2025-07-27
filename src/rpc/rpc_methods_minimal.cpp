#include <neo/cryptography/base64.h>
#include <neo/rpc/rpc_methods.h>

namespace neo::rpc
{
using json = nlohmann::json;

nlohmann::json RPCMethods::GetVersion(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result;
    result["port"] = 10333;
    result["nonce"] = 12345;
    result["useragent"] = "/NEO:3.0.0/";

    // Protocol information
    json protocol;
    protocol["addressversion"] = 53;
    protocol["network"] = 894710606;  // TestNet magic number
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

    // Return null for now - full implementation would require accessing blockchain
    return nullptr;
}

nlohmann::json RPCMethods::InvokeContract(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }

    // Return test response for now
    json result;
    result["script"] = "00";
    result["state"] = "HALT";
    result["gasconsumed"] = "0";
    result["stack"] = json::array();

    return result;
}

nlohmann::json RPCMethods::GetMemPoolDetails(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    auto memPool = neoSystem->GetMemPool();
    if (!memPool)
    {
        throw std::runtime_error("Memory pool not available");
    }

    json result;
    result["size"] = memPool->GetCount();
    result["capacity"] = memPool->GetCapacity();
    result["transactions"] = json::array();

    return result;
}

nlohmann::json RPCMethods::GetTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing transaction hash parameter");
    }

    // Return null for now - full implementation would require accessing mempool and blockchain
    return nullptr;
}

nlohmann::json RPCMethods::GetBalance(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing account parameter");
    }

    // Return empty balance for now
    json result;
    result["address"] = params[0].get<std::string>();
    result["balance"] = json::array();

    return result;
}
}  // namespace neo::rpc