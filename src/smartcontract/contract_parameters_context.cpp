/**
 * @file contract_parameters_context.cpp
 * @brief Contract Parameters Context
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/base64.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/smartcontract/contract_parameters_context.h>
#include <neo/smartcontract/interop_service.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script.h>
#include <neo/vm/script_builder.h>

#include <algorithm>
#include <limits>
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
    if (IsMultiSigContract(contract.GetScript(), m, n, publicKeys))
    {
        auto pubKeyIt = std::find(publicKeys.begin(), publicKeys.end(), pubkey);
        if (pubKeyIt == publicKeys.end()) return false;

        auto itemIt = contextItems.find(scriptHash);
        if (itemIt == contextItems.end())
        {
            auto item = CreateItem(contract);
            if (!item) return false;
            itemIt = contextItems.find(scriptHash);
        }

        auto& signatures = itemIt->second->signatures;
        if (!signatures.emplace(pubkey, signature).second) return false;

        if (static_cast<int>(signatures.size()) == m)
        {
            std::vector<std::pair<int, io::ByteVector>> ordered;
            ordered.reserve(signatures.size());
            for (const auto& entry : signatures)
            {
                auto indexIt = std::find(publicKeys.begin(), publicKeys.end(), entry.first);
                if (indexIt == publicKeys.end()) continue;
                int idx = static_cast<int>(std::distance(publicKeys.begin(), indexIt));
                ordered.emplace_back(idx, entry.second);
            }
            std::sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.first > rhs.first;
            });

            size_t limit = std::min(ordered.size(), itemIt->second->parameters.size());
            for (size_t i = 0; i < limit; ++i)
            {
                if (!Add(contract, static_cast<int>(i), ordered[i].second))
                {
                    return false;
                }
            }
        }

        return true;
    }

    int signatureIndex = -1;
    const auto& parameterList = contract.GetParameterList();
    for (size_t i = 0; i < parameterList.size(); ++i)
    {
        if (parameterList[i] == ContractParameterType::Signature)
        {
            if (signatureIndex >= 0)
            {
                throw std::runtime_error("Multiple signature parameters are not supported");
            }
            signatureIndex = static_cast<int>(i);
        }
    }

    if (signatureIndex < 0) return false;

    auto itemIt = contextItems.find(scriptHash);
    if (itemIt == contextItems.end())
    {
        auto item = CreateItem(contract);
        if (!item) return false;
        itemIt = contextItems.find(scriptHash);
    }

    auto& signatures = itemIt->second->signatures;
    if (!signatures.emplace(pubkey, signature).second) return false;

    itemIt->second->parameters[signatureIndex].SetValue(signature);
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
    if (script.Size() < 5) return false;

    auto readInteger = [&](size_t& offset, int64_t& value) -> bool {
        if (offset >= script.Size()) return false;
        uint8_t opcode = script[offset++];
        auto op = static_cast<vm::OpCode>(opcode);
        switch (op)
        {
            case vm::OpCode::PUSH0:
                value = 0;
                return true;
            default:
                break;
        }

        if (opcode >= static_cast<uint8_t>(vm::OpCode::PUSH1) && opcode <= static_cast<uint8_t>(vm::OpCode::PUSH16))
        {
            value = static_cast<int64_t>(opcode - static_cast<uint8_t>(vm::OpCode::PUSH1) + 1);
            return true;
        }

        auto readSigned = [&](size_t byteCount, int64_t& result) -> bool {
            if (offset + byteCount > script.Size()) return false;
            if (byteCount == 1)
            {
                result = static_cast<int8_t>(script[offset]);
            }
            else if (byteCount == 2)
            {
                uint16_t raw = static_cast<uint16_t>(script[offset]) |
                               (static_cast<uint16_t>(script[offset + 1]) << 8);
                result = static_cast<int16_t>(raw);
            }
            else if (byteCount == 4)
            {
                uint32_t raw = static_cast<uint32_t>(script[offset]) |
                               (static_cast<uint32_t>(script[offset + 1]) << 8) |
                               (static_cast<uint32_t>(script[offset + 2]) << 16) |
                               (static_cast<uint32_t>(script[offset + 3]) << 24);
                result = static_cast<int32_t>(raw);
            }
            else if (byteCount == 8)
            {
                uint64_t raw = 0;
                for (size_t i = 0; i < byteCount; ++i)
                {
                    raw |= static_cast<uint64_t>(script[offset + i]) << (8 * i);
                }
                result = static_cast<int64_t>(raw);
            }
            else
            {
                return false;
            }
            offset += byteCount;
            return true;
        };

        switch (op)
        {
            case vm::OpCode::PUSHINT8:
                return readSigned(1, value);
            case vm::OpCode::PUSHINT16:
                return readSigned(2, value);
            case vm::OpCode::PUSHINT32:
                return readSigned(4, value);
            case vm::OpCode::PUSHINT64:
                return readSigned(8, value);
            default:
                return false;
        }
    };

    size_t offset = 0;
    int64_t mValue = 0;
    if (!readInteger(offset, mValue) || mValue <= 0) return false;
    if (mValue > std::numeric_limits<int>::max()) return false;
    m = static_cast<int>(mValue);

    publicKeys.clear();
    while (offset < script.Size())
    {
        if (offset + 2 >= script.Size()) break;
        if (script[offset] != static_cast<uint8_t>(vm::OpCode::PUSHDATA1)) break;
        uint8_t length = script[offset + 1];
        if (length != 33) return false;
        offset += 2;
        if (offset + length > script.Size()) return false;
        try
        {
            auto pubkey = cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + offset, length));
            publicKeys.push_back(pubkey);
        }
        catch (const std::exception&)
        {
            return false;
        }
        offset += length;
    }

    if (publicKeys.empty()) return false;

    int64_t nValue = 0;
    if (!readInteger(offset, nValue) || nValue <= 0) return false;
    if (nValue > std::numeric_limits<int>::max()) return false;
    n = static_cast<int>(nValue);
    if (n != static_cast<int>(publicKeys.size()) || m > n) return false;

    if (script.Size() < offset + 5) return false;
    if (script[offset] != static_cast<uint8_t>(vm::OpCode::SYSCALL)) return false;
    ++offset;

    uint32_t expected = calculate_interop_hash("System.Crypto.CheckMultisig");
    for (int i = 0; i < 4; ++i)
    {
        if (script[offset + i] != static_cast<uint8_t>((expected >> (8 * i)) & 0xFF)) return false;
    }
    offset += 4;

    return offset == script.Size();
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
