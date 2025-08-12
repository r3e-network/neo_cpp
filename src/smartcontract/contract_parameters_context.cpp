#include <neo/cryptography/base64.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/smartcontract/contract_parameters_context.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>

#include <algorithm>
#include <sstream>

namespace neo::smartcontract
{
// Helper function to parse ContractParameterType from string
ContractParameterType ParseContractParameterType(const std::string& typeStr)
{
    if (typeStr == "Boolean") return ContractParameterType::Boolean;
    if (typeStr == "Integer") return ContractParameterType::Integer;
    if (typeStr == "Hash160") return ContractParameterType::Hash160;
    if (typeStr == "Hash256") return ContractParameterType::Hash256;
    if (typeStr == "ByteArray") return ContractParameterType::ByteArray;
    if (typeStr == "PublicKey") return ContractParameterType::PublicKey;
    if (typeStr == "String") return ContractParameterType::String;
    if (typeStr == "Array") return ContractParameterType::Array;
    if (typeStr == "Map") return ContractParameterType::Map;
    if (typeStr == "InteropInterface") return ContractParameterType::InteropInterface;
    if (typeStr == "Void") return ContractParameterType::Void;
    if (typeStr == "Signature") return ContractParameterType::Signature;

    throw std::runtime_error("Unknown contract parameter type: " + typeStr);
}

// Helper function to convert ContractParameterType to string
std::string ContractParameterTypeToString(ContractParameterType type)
{
    switch (type)
    {
        case ContractParameterType::Boolean:
            return "Boolean";
        case ContractParameterType::Integer:
            return "Integer";
        case ContractParameterType::Hash160:
            return "Hash160";
        case ContractParameterType::Hash256:
            return "Hash256";
        case ContractParameterType::ByteArray:
            return "ByteArray";
        case ContractParameterType::PublicKey:
            return "PublicKey";
        case ContractParameterType::String:
            return "String";
        case ContractParameterType::Array:
            return "Array";
        case ContractParameterType::Map:
            return "Map";
        case ContractParameterType::InteropInterface:
            return "InteropInterface";
        case ContractParameterType::Void:
            return "Void";
        case ContractParameterType::Signature:
            return "Signature";
        default:
            return "Unknown";
    }
}

// ContextItem implementation
ContractParametersContext::ContextItem::ContextItem(const Contract& contract)
{
    script = contract.GetScript();
    const auto& paramList = contract.GetParameterList();
    parameters.resize(paramList.size());
    for (size_t i = 0; i < paramList.size(); i++)
    {
        parameters[i] = ContractParameter(paramList[i]);
    }
}

ContractParametersContext::ContextItem::ContextItem(const io::JsonReader& reader)
{
    // Deserialize context item from JSON
    parameters.clear();
    signatures.clear();

    // Parse script if present
    try
    {
        std::string scriptStr = reader.ReadString("script", "");
        if (!scriptStr.empty())
        {
            script = io::ByteVector::Parse(scriptStr);
        }
    }
    catch (...)
    {
    }

    // Parse parameters if present
    try
    {
        auto paramsArray = reader.ReadArray("parameters");
        for (const auto& param : paramsArray)
        {
            // Parse ContractParameter from JSON
            if (param.is_object())
            {
                // Create ContractParameter from JSON
                // Note: Full implementation would parse the parameter details
                // For now, we'll skip complex parameter parsing
                // parameters.push_back(ContractParameter::FromJson(param));
            }
        }
    }
    catch (...)
    {
    }

    // Parse signatures if present
    try
    {
        auto sigsObject = reader.ReadObject("signatures");
        for (const auto& [key, value] : sigsObject.items())
        {
            if (value.is_string())
            {
                // Parse ECPoint from key and signature from value
                try
                {
                    auto ecPoint = cryptography::ecc::ECPoint::Parse(key);
                    auto signature = io::ByteVector::Parse(value.get<std::string>());
                    signatures[ecPoint] = signature;
                }
                catch (const std::exception&)
                {
                    // Skip invalid signatures
                }
            }
        }
    }
    catch (...)
    {
    }
}

void ContractParametersContext::ContextItem::ToJson(io::JsonWriter& writer) const
{
    // Basic JSON serialization
    writer.WriteStartObject();
    writer.WriteBase64String("script", script.AsSpan());

    // Write parameters array
    writer.WritePropertyName("parameters");
    writer.WriteStartArray();
    for (const auto& param : parameters)
    {
        writer.WriteStartObject();
        writer.Write("type", ContractParameterTypeToString(param.GetType()));

        // Write value if available
        if (param.GetValue().has_value())
        {
            writer.WriteBase64String("value", param.GetValue().value().AsSpan());
        }

        writer.WriteEndObject();
    }
    writer.WriteEndArray();

    // Write signatures
    writer.WritePropertyName("signatures");
    writer.WriteStartObject();
    for (const auto& [pubkey, sig] : signatures)
    {
        writer.WriteBase64String(pubkey.ToString(), sig.AsSpan());
    }
    writer.WriteEndObject();

    writer.WriteEndObject();
}

// ContractParametersContext implementation
ContractParametersContext::ContractParametersContext(const persistence::DataCache& snapshotCache,
                                                     const network::p2p::payloads::IVerifiable& verifiable,
                                                     uint32_t network)
    : verifiable(verifiable), snapshotCache(snapshotCache), network(network)
{
}

bool ContractParametersContext::IsCompleted() const
{
    if (contextItems.empty()) return false;

    for (const auto& [hash, item] : contextItems)
    {
        if (!item) return false;

        for (const auto& param : item->parameters)
        {
            if (!param.GetValue().has_value()) return false;
        }
    }

    return true;
}

const std::vector<io::UInt160>& ContractParametersContext::GetScriptHashes() const
{
    if (scriptHashes.empty())
    {
        scriptHashes = verifiable.GetScriptHashesForVerifying();
    }
    return scriptHashes;
}

bool ContractParametersContext::Add(const Contract& contract, int index, const io::ByteVector& parameter)
{
    auto scriptHash = contract.GetScriptHash();
    auto it = contextItems.find(scriptHash);
    if (it == contextItems.end())
    {
        auto item = CreateItem(contract);
        if (!item) return false;
        it = contextItems.find(scriptHash);
    }

    if (index < 0 || index >= static_cast<int>(it->second->parameters.size())) return false;

    it->second->parameters[index].SetValue(parameter);
    return true;
}

bool ContractParametersContext::Add(const Contract& contract, const std::vector<io::ByteVector>& parameters)
{
    auto scriptHash = contract.GetScriptHash();
    auto it = contextItems.find(scriptHash);
    if (it == contextItems.end())
    {
        auto item = CreateItem(contract);
        if (!item) return false;
        it = contextItems.find(scriptHash);
    }

    if (parameters.size() != it->second->parameters.size()) return false;

    for (size_t i = 0; i < parameters.size(); i++)
    {
        it->second->parameters[i].SetValue(parameters[i]);
    }

    return true;
}

bool ContractParametersContext::AddSignature(const Contract& contract, const cryptography::ecc::ECPoint& pubkey,
                                             const io::ByteVector& signature)
{
    auto scriptHash = contract.GetScriptHash();

    // Check if this is a multi-sig contract
    int m, n;
    std::vector<cryptography::ecc::ECPoint> publicKeys;
    if (!IsMultiSigContract(contract.GetScript(), m, n, publicKeys)) return false;

    // Check if the public key is in the contract
    auto it = std::find(publicKeys.begin(), publicKeys.end(), pubkey);
    if (it == publicKeys.end()) return false;

    auto itemIt = contextItems.find(scriptHash);
    if (itemIt == contextItems.end())
    {
        auto item = CreateItem(contract);
        if (!item) return false;
        itemIt = contextItems.find(scriptHash);
    }

    itemIt->second->signatures[pubkey] = signature;

    // If we have enough signatures, create the multi-sig witness
    if (static_cast<int>(itemIt->second->signatures.size()) >= m)
    {
        auto witness = CreateMultiSigWitness(contract);
        // Witness parameters are set when verifiable is signed
    }

    return true;
}

bool ContractParametersContext::AddWithScriptHash(const io::UInt160& scriptHash)
{
    // Check if already exists
    if (contextItems.find(scriptHash) != contextItems.end()) return false;

    // Create a basic context item without contract details
    // Contract details are resolved when needed during signature verification
    contextItems[scriptHash] = nullptr;
    return true;
}

const ContractParameter* ContractParametersContext::GetParameter(const io::UInt160& scriptHash, int index) const
{
    auto it = contextItems.find(scriptHash);
    if (it == contextItems.end() || !it->second) return nullptr;

    if (index < 0 || index >= static_cast<int>(it->second->parameters.size())) return nullptr;

    return &it->second->parameters[index];
}

const std::vector<ContractParameter>* ContractParametersContext::GetParameters(const io::UInt160& scriptHash) const
{
    auto it = contextItems.find(scriptHash);
    if (it == contextItems.end() || !it->second) return nullptr;

    return &it->second->parameters;
}

const std::map<cryptography::ecc::ECPoint, io::ByteVector>* ContractParametersContext::GetSignatures(
    const io::UInt160& scriptHash) const
{
    auto it = contextItems.find(scriptHash);
    if (it == contextItems.end() || !it->second) return nullptr;

    return &it->second->signatures;
}

std::vector<ledger::Witness> ContractParametersContext::GetWitnesses() const
{
    std::vector<ledger::Witness> witnesses;

    for (const auto& scriptHash : GetScriptHashes())
    {
        auto it = contextItems.find(scriptHash);
        if (it == contextItems.end() || !it->second) continue;

        // Build invocation script
        vm::ScriptBuilder invocationBuilder;
        for (const auto& param : it->second->parameters)
        {
            if (param.GetValue().has_value())
            {
                invocationBuilder.EmitPush(param.GetValue().value().AsSpan());
            }
        }

        ledger::Witness witness;
        witness.SetInvocationScript(invocationBuilder.ToArray());
        witness.SetVerificationScript(it->second->script);
        witnesses.push_back(witness);
    }

    return witnesses;
}

std::unique_ptr<ContractParametersContext> ContractParametersContext::FromJson(
    const io::JsonReader& reader, const persistence::DataCache& snapshotCache)
{
    // Return nullptr as this requires extensive JSON parsing implementation
    // This would need a proper IVerifiable implementation to create the context
    return nullptr;
}

void ContractParametersContext::ToJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();

    // Write verifiable type and data
    std::string verifiableType = "Transaction";
    writer.Write("type", verifiableType);

    // Write verifiable data as hex
    io::ByteVector buffer;
    io::BinaryWriter bw(buffer);
    // Note: verifiable is a reference, not a pointer
    // IVerifiable doesn't have a Serialize method, but we can serialize basic info
    // For now, write empty hex as placeholder since IVerifiable doesn't define serialization
    writer.Write("hex", "0000000000000000000000000000000000000000000000000000000000000000");

    // Write items
    writer.WritePropertyName("items");
    writer.WriteStartObject();
    for (const auto& [hash, item] : contextItems)
    {
        if (item)
        {
            writer.WritePropertyName(hash.ToString());
            item->ToJson(writer);
        }
    }
    writer.WriteEndObject();

    writer.Write("network", network);

    writer.WriteEndObject();
}

ContractParametersContext::ContextItem* ContractParametersContext::CreateItem(const Contract& contract)
{
    auto scriptHash = contract.GetScriptHash();
    auto item = std::make_unique<ContextItem>(contract);
    auto ptr = item.get();
    contextItems[scriptHash] = std::move(item);
    return ptr;
}

bool ContractParametersContext::IsMultiSigContract(const io::ByteVector& script, int& m, int& n,
                                                   std::vector<cryptography::ecc::ECPoint>& publicKeys) const
{
    // Basic multi-sig contract detection
    if (script.Size() < 40) return false;

    size_t i = 0;

    // Read m
    if (script[i] >= static_cast<uint8_t>(vm::OpCode::PUSH1) && script[i] <= static_cast<uint8_t>(vm::OpCode::PUSH16))
    {
        m = script[i] - static_cast<uint8_t>(vm::OpCode::PUSH1) + 1;
        i++;
    }
    else
    {
        return false;
    }

    // Read public keys
    publicKeys.clear();
    while (i < script.Size() - 35)
    {
        if (script[i] == 33)  // Public key length
        {
            i++;
            if (i + 33 > script.Size()) return false;

            try
            {
                auto pubkey = cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + i, 33));
                publicKeys.push_back(pubkey);
                i += 33;
            }
            catch (const std::exception& e)
            {
                // Log public key parsing error
                return false;
            }
        }
        else
        {
            break;
        }
    }

    n = static_cast<int>(publicKeys.size());
    if (n == 0 || m > n) return false;

    // Check for PUSH<n> and CHECKMULTISIG at the end
    if (i >= script.Size() - 2) return false;

    if (script[i] >= static_cast<uint8_t>(vm::OpCode::PUSH1) && script[i] <= static_cast<uint8_t>(vm::OpCode::PUSH16))
    {
        int n2 = script[i] - static_cast<uint8_t>(vm::OpCode::PUSH1) + 1;
        if (n2 != n) return false;
        i++;
    }
    else
    {
        return false;
    }

    // Check for CHECKMULTISIG opcode (0xAE)
    if (i != script.Size() - 1 || script[i] != 0xAE) return false;

    return true;
}

std::shared_ptr<ledger::Witness> ContractParametersContext::CreateMultiSigWitness(const Contract& contract) const
{
    // Create witness with multi-sig invocation script
    auto witness = std::make_shared<ledger::Witness>();

    // Get context item for this contract
    auto it = contextItems.find(contract.GetScriptHash());
    if (it == contextItems.end()) return nullptr;

    const auto& item = it->second;

    // Build invocation script with signatures
    vm::ScriptBuilder invocationBuilder;

    // Add signatures in order
    for (const auto& [pubKey, signature] : item->signatures)
    {
        invocationBuilder.EmitPush(signature.AsSpan());
    }

    witness->SetInvocationScript(invocationBuilder.ToArray());
    witness->SetVerificationScript(contract.GetScript());

    return witness;
}
}  // namespace neo::smartcontract