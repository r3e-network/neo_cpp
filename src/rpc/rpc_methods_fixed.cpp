#include <neo/cryptography/base64.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/signer.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/persistence/storage_key.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/stack_item_types.h>
#include <neo/vm/vm_state.h>
#include <sstream>

namespace neo::rpc
{
using json = nlohmann::json;

nlohmann::json RPCMethods::GetVersion(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    json result;
    auto localNode = neoSystem->GetLocalNode();
    result["port"] = localNode ? localNode->GetPort() : 10333;
    result["nonce"] = localNode ? localNode->GetNonce() : 12345;
    result["useragent"] = localNode ? localNode->GetUserAgent() : "/NEO:3.0.0/";

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

    auto blockchain = neoSystem->GetBlockchain();
    if (!blockchain)
    {
        throw std::runtime_error("Blockchain not available");
    }

    bool verbose = params.size() > 1 && params[1].get<bool>();
    std::shared_ptr<ledger::Block> block;

    // Check if parameter is hash or index
    if (params[0].is_string())
    {
        auto hashStr = params[0].get<std::string>();
        io::UInt256 hash = io::UInt256::Parse(hashStr);
        block = blockchain->GetBlock(hash);
    }
    else if (params[0].is_number())
    {
        uint32_t index = params[0].get<uint32_t>();
        auto hash = blockchain->GetBlockHash(index);
        block = blockchain->GetBlock(hash);
    }
    else
    {
        throw std::runtime_error("Invalid block identifier");
    }

    if (!block)
    {
        return nullptr;
    }

    if (verbose)
    {
        // Return full block details
        json blockJson;
        blockJson["hash"] = block->GetHash().ToString();
        blockJson["size"] = block->GetSize();
        blockJson["version"] = block->GetVersion();
        blockJson["previousblockhash"] = block->GetPrevHash().ToString();
        blockJson["merkleroot"] = block->GetMerkleRoot().ToString();
        blockJson["time"] = block->GetTimestamp();
        blockJson["index"] = block->GetIndex();
        blockJson["primary"] = block->GetPrimaryIndex();
        blockJson["nextconsensus"] = block->GetNextConsensus().ToString();

        // Add transactions
        json txArray = json::array();
        for (const auto& tx : block->GetTransactions())
        {
            json txJson;
            txJson["hash"] = tx.GetHash().ToString();
            txJson["size"] = tx.GetSize();
            txJson["version"] = tx.GetVersion();
            txArray.push_back(txJson);
        }
        blockJson["tx"] = txArray;

        return blockJson;
    }
    else
    {
        // Return block as base64 string
        std::stringstream ss;
        io::BinaryWriter writer(ss);
        block->Serialize(writer);
        std::string blockData = ss.str();
        return Base64::Encode(
            std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(blockData.data()), blockData.size()));
    }
}

nlohmann::json RPCMethods::GetContract(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing contract hash parameter");
    }

    auto hashStr = params[0].get<std::string>();
    io::UInt160 contractHash = io::UInt160::Parse(hashStr);

    auto snapshot = neoSystem->GetSnapshot();
    auto engine = std::make_shared<smartcontract::ApplicationEngine>(smartcontract::TriggerType::Application, nullptr,
                                                                     snapshot, nullptr, 0, true);

    auto contractManagement = smartcontract::native::ContractManagement::GetInstance();
    auto contract = contractManagement->GetContract(engine, contractHash);

    if (!contract)
    {
        return nullptr;
    }

    json contractJson;
    contractJson["id"] = contract->GetId();
    contractJson["updatecounter"] = contract->GetUpdateCounter();
    contractJson["hash"] = contract->GetHash().ToString();
    contractJson["nef"] = Base64::Encode(contract->GetNef().ToArray().AsSpan());

    // Add manifest
    json manifestJson;
    manifestJson["name"] = contract->GetManifest().GetName();
    manifestJson["abi"] = json::object();  // Simplified for now
    manifestJson["permissions"] = json::array();
    manifestJson["trusts"] = json::array();
    manifestJson["extra"] = nullptr;

    contractJson["manifest"] = manifestJson;

    return contractJson;
}

nlohmann::json RPCMethods::InvokeContract(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.size() < 2)
    {
        throw std::runtime_error("Missing required parameters");
    }

    auto scriptHashStr = params[0].get<std::string>();
    auto method = params[1].get<std::string>();

    io::UInt160 scriptHash = io::UInt160::Parse(scriptHashStr);

    // Prepare parameters
    std::vector<std::shared_ptr<vm::StackItem>> methodParams;
    if (params.size() > 2 && params[2].is_array())
    {
        for (const auto& param : params[2])
        {
            if (param.is_string())
            {
                methodParams.push_back(vm::StackItem::Create(param.get<std::string>()));
            }
            else if (param.is_number())
            {
                methodParams.push_back(vm::StackItem::Create(param.get<int64_t>()));
            }
            else if (param.is_boolean())
            {
                methodParams.push_back(vm::StackItem::Create(param.get<bool>()));
            }
        }
    }

    // Create script
    std::stringstream scriptStream;
    io::BinaryWriter scriptWriter(scriptStream);

    // Build invocation script
    for (auto it = methodParams.rbegin(); it != methodParams.rend(); ++it)
    {
        // Push parameters onto stack
        if (auto str = (*it)->GetString(); !str.empty())
        {
            scriptWriter.Write(static_cast<uint8_t>(0x0C));  // PUSHDATA1
            scriptWriter.WriteVarString(str);
        }
        else if (auto intVal = (*it)->GetInteger(); intVal != nullptr)
        {
            scriptWriter.Write(static_cast<uint8_t>(0x00));  // PUSH0 (simplified)
        }
    }

    // Push method name
    scriptWriter.Write(static_cast<uint8_t>(0x0C));  // PUSHDATA1
    scriptWriter.WriteVarString(method);

    // Push script hash
    scriptWriter.Write(static_cast<uint8_t>(0x14));  // PUSH20
    scriptWriter.Write(scriptHash);

    // Call contract
    scriptWriter.Write(static_cast<uint8_t>(0x41));  // SYSCALL
    scriptWriter.WriteVarString("System.Contract.Call");

    std::string scriptData = scriptStream.str();
    io::ByteVector script(std::vector<uint8_t>(scriptData.begin(), scriptData.end()));

    // Execute script
    auto snapshot = neoSystem->GetSnapshot();
    auto engine = std::make_shared<smartcontract::ApplicationEngine>(smartcontract::TriggerType::Application, nullptr,
                                                                     snapshot, nullptr,
                                                                     20000000,  // 0.2 GAS
                                                                     true);

    engine->LoadScript(script);
    auto state = engine->Execute();

    json result;
    result["script"] = Base64::Encode(script.AsSpan());
    result["state"] = vm::ToString(state);
    result["gasconsumed"] = std::to_string(engine->GetGasConsumed());

    if (state == vm::VMState::HALT)
    {
        json stack = json::array();
        auto& evalStack = engine->GetEvaluationStack();
        while (evalStack.GetCount() > 0)
        {
            auto item = evalStack.Pop();
            json stackItem;
            stackItem["type"] = vm::GetStackItemTypeName(item->GetType());

            if (item->GetType() == vm::StackItemType::ByteString)
            {
                stackItem["value"] = Base64::Encode(item->GetByteArray().AsSpan());
            }
            else if (item->GetType() == vm::StackItemType::Integer)
            {
                stackItem["value"] = item->GetInteger()->ToString();
            }
            else if (item->GetType() == vm::StackItemType::Boolean)
            {
                stackItem["value"] = item->GetBoolean();
            }

            stack.push_back(stackItem);
        }
        result["stack"] = stack;
    }
    else
    {
        result["exception"] = engine->GetFaultException();
    }

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

    json transactions = json::array();
    // Memory pool doesn't expose GetTransactions() in current interface
    // This would need to be added to the MemoryPool class
    result["transactions"] = transactions;

    return result;
}

nlohmann::json RPCMethods::GetTransaction(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing transaction hash parameter");
    }

    auto hashStr = params[0].get<std::string>();
    io::UInt256 hash = io::UInt256::Parse(hashStr);
    bool verbose = params.size() > 1 && params[1].get<bool>();

    // Try memory pool first
    auto memPool = neoSystem->GetMemPool();
    if (memPool)
    {
        auto tx = memPool->TryGetValue(hash);
        if (tx)
        {
            if (verbose)
            {
                json txJson;
                txJson["hash"] = tx->GetHash().ToString();
                txJson["size"] = tx->GetSize();
                txJson["version"] = tx->GetVersion();
                txJson["sys_fee"] = tx->GetSystemFee();
                txJson["net_fee"] = tx->GetNetworkFee();
                txJson["valid_until_block"] = tx->GetValidUntilBlock();
                txJson["script"] = Base64::Encode(tx->GetScript().AsSpan());

                json signers = json::array();
                for (const auto& signer : tx->GetSigners())
                {
                    json signerJson;
                    signerJson["account"] = signer.GetAccount().ToString();
                    signerJson["scopes"] = static_cast<uint8_t>(signer.GetScopes());
                    signers.push_back(signerJson);
                }
                txJson["signers"] = signers;

                return txJson;
            }
            else
            {
                std::stringstream ss;
                io::BinaryWriter writer(ss);
                tx->Serialize(writer);
                std::string txData = ss.str();
                return Base64::Encode(
                    std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(txData.data()), txData.size()));
            }
        }
    }

    // Try blockchain
    auto blockchain = neoSystem->GetBlockchain();
    if (blockchain)
    {
        // Blockchain would need to expose GetTransaction method
        // For now, return null
    }

    return nullptr;
}

nlohmann::json RPCMethods::GetBalance(std::shared_ptr<node::NeoSystem> neoSystem, const nlohmann::json& params)
{
    if (params.empty())
    {
        throw std::runtime_error("Missing account parameter");
    }

    auto accountStr = params[0].get<std::string>();
    io::UInt160 account = io::UInt160::Parse(accountStr);

    auto snapshot = neoSystem->GetSnapshot();
    auto neoToken = smartcontract::native::NeoToken::GetInstance();
    auto gasToken = smartcontract::native::GasToken::GetInstance();

    json result;
    json balances = json::array();

    // NEO balance
    json neoBalance;
    neoBalance["asset"] = neoToken->GetHash().ToString();
    neoBalance["amount"] = std::to_string(neoToken->GetBalance(snapshot, account));
    balances.push_back(neoBalance);

    // GAS balance
    json gasBalance;
    gasBalance["asset"] = gasToken->GetHash().ToString();
    gasBalance["amount"] = std::to_string(gasToken->GetBalance(snapshot, account));
    balances.push_back(gasBalance);

    result["address"] = account.ToString();
    result["balance"] = balances;

    return result;
}
}  // namespace neo::rpc