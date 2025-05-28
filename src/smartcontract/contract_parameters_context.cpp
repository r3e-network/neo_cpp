#include <neo/smartcontract/contract_parameters_context.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/script.h>
#include <neo/vm/op_code.h>
#include <neo/cryptography/crypto.h>
#include <algorithm>
#include <sstream>

namespace neo::smartcontract
{
    ContractParametersContext::ContextItem::ContextItem(const Contract& contract)
        : script(contract.GetScript())
    {
        // Create parameters based on the contract's parameter list
        for (auto paramType : contract.GetParameterList())
        {
            parameters.emplace_back(paramType);
        }
    }

    ContractParametersContext::ContextItem::ContextItem(const io::JsonReader& reader)
    {
        // Read script
        auto scriptBase64 = reader.ReadString("script");
        if (!scriptBase64.empty())
        {
            script = io::ByteVector::FromBase64(scriptBase64);
        }

        // Read parameters
        auto parametersArray = reader.ReadArray("parameters");
        for (size_t i = 0; i < parametersArray.size(); i++)
        {
            ContractParameter parameter;
            // TODO: Implement parameter deserialization from JSON
            parameters.push_back(parameter);
        }

        // Read signatures
        auto signaturesObj = reader.ReadObject("signatures");
        for (const auto& property : signaturesObj.GetProperties())
        {
            auto pubkey = cryptography::ecc::ECPoint::Parse(property.first);
            auto signature = io::ByteVector::FromBase64(property.second.GetString());
            signatures[pubkey] = signature;
        }
    }

    void ContractParametersContext::ContextItem::ToJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();

        // Write script
        writer.WritePropertyName("script");
        if (script.IsEmpty())
        {
            writer.WriteNull();
        }
        else
        {
            writer.WriteString(script.ToBase64());
        }

        // Write parameters
        writer.WritePropertyName("parameters");
        writer.WriteStartArray();
        for (const auto& parameter : parameters)
        {
            // TODO: Implement parameter serialization to JSON
            writer.WriteStartObject();
            writer.WritePropertyName("type");
            writer.WriteString(std::to_string(static_cast<uint8_t>(parameter.GetType())));
            writer.WritePropertyName("value");
            if (parameter.GetValue().has_value())
            {
                writer.WriteString(parameter.GetValue()->ToHexString());
            }
            else
            {
                writer.WriteNull();
            }
            writer.WriteEndObject();
        }
        writer.WriteEndArray();

        // Write signatures
        writer.WritePropertyName("signatures");
        writer.WriteStartObject();
        for (const auto& signature : signatures)
        {
            writer.WritePropertyName(signature.first.ToString());
            writer.WriteString(signature.second.ToBase64());
        }
        writer.WriteEndObject();

        writer.WriteEndObject();
    }

    ContractParametersContext::ContractParametersContext(const persistence::DataCache& snapshotCache, const network::p2p::payloads::IVerifiable& verifiable, uint32_t network)
        : verifiable(verifiable), snapshotCache(snapshotCache), network(network)
    {
    }

    bool ContractParametersContext::IsCompleted() const
    {
        if (contextItems.size() < GetScriptHashes().size())
            return false;

        for (const auto& item : contextItems)
        {
            if (!item.second)
                return false;

            for (const auto& parameter : item.second->parameters)
            {
                if (!parameter.GetValue().has_value())
                    return false;
            }
        }

        return true;
    }

    const std::vector<io::UInt160>& ContractParametersContext::GetScriptHashes() const
    {
        if (scriptHashes.empty())
        {
            scriptHashes = verifiable.GetScriptHashesForVerifying(snapshotCache);
        }
        return scriptHashes;
    }

    bool ContractParametersContext::Add(const Contract& contract, int index, const io::ByteVector& parameter)
    {
        ContextItem* item = CreateItem(contract);
        if (!item)
            return false;

        if (index < 0 || index >= static_cast<int>(item->parameters.size()))
            return false;

        item->parameters[index].SetValue(parameter);
        return true;
    }

    bool ContractParametersContext::Add(const Contract& contract, const std::vector<io::ByteVector>& parameters)
    {
        ContextItem* item = CreateItem(contract);
        if (!item)
            return false;

        if (parameters.size() > item->parameters.size())
            return false;

        for (size_t i = 0; i < parameters.size(); i++)
        {
            item->parameters[i].SetValue(parameters[i]);
        }
        return true;
    }

    bool ContractParametersContext::AddSignature(const Contract& contract, const cryptography::ecc::ECPoint& pubkey, const io::ByteVector& signature)
    {
        // TODO: Implement multi-signature contract support
        ContextItem* item = CreateItem(contract);
        if (!item)
            return false;

        // Add the signature
        item->signatures[pubkey] = signature;

        // For single-signature contracts, set the signature as the parameter value
        if (item->parameters.size() == 1 && item->parameters[0].GetType() == ContractParameterType::Signature)
        {
            item->parameters[0].SetValue(signature);
        }

        return true;
    }

    bool ContractParametersContext::AddWithScriptHash(const io::UInt160& scriptHash)
    {
        // Try to get the contract from the contract management
        auto contract = native::ContractManagement::GetContract(snapshotCache, scriptHash);
        if (!contract)
            return false;

        // Create a deployed contract
        Contract deployedContract;
        // TODO: Implement deployed contract creation

        // Only works with verify without parameters
        if (deployedContract.GetParameterList().empty())
        {
            return Add(deployedContract, std::vector<io::ByteVector>());
        }

        return false;
    }

    const ContractParameter* ContractParametersContext::GetParameter(const io::UInt160& scriptHash, int index) const
    {
        const auto* parameters = GetParameters(scriptHash);
        if (!parameters || index < 0 || index >= static_cast<int>(parameters->size()))
            return nullptr;

        return &(*parameters)[index];
    }

    const std::vector<ContractParameter>* ContractParametersContext::GetParameters(const io::UInt160& scriptHash) const
    {
        auto it = contextItems.find(scriptHash);
        if (it == contextItems.end() || !it->second)
            return nullptr;

        return &it->second->parameters;
    }

    const std::map<cryptography::ecc::ECPoint, io::ByteVector>* ContractParametersContext::GetSignatures(const io::UInt160& scriptHash) const
    {
        auto it = contextItems.find(scriptHash);
        if (it == contextItems.end() || !it->second)
            return nullptr;

        return &it->second->signatures;
    }

    std::vector<network::p2p::payloads::Witness> ContractParametersContext::GetWitnesses() const
    {
        std::vector<network::p2p::payloads::Witness> witnesses;
        for (const auto& scriptHash : GetScriptHashes())
        {
            auto it = contextItems.find(scriptHash);
            if (it == contextItems.end() || !it->second)
                throw std::runtime_error("Missing signature");

            // TODO: Implement witness creation
            network::p2p::payloads::Witness witness;
            witnesses.push_back(witness);
        }
        return witnesses;
    }

    std::unique_ptr<ContractParametersContext> ContractParametersContext::FromJson(const io::JsonReader& reader, const persistence::DataCache& snapshotCache)
    {
        // TODO: Implement deserialization from JSON
        return nullptr;
    }

    void ContractParametersContext::ToJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();

        // Write type
        writer.WritePropertyName("type");
        writer.WriteString(typeid(verifiable).name());

        // Write hash
        writer.WritePropertyName("hash");
        writer.WriteString(verifiable.GetHash().ToString());

        // Write data
        writer.WritePropertyName("data");
        std::stringstream stream;
        io::BinaryWriter binaryWriter(stream);
        verifiable.SerializeUnsigned(binaryWriter);
        writer.WriteString(io::ByteVector(stream.str()).ToBase64());

        // Write items
        writer.WritePropertyName("items");
        writer.WriteStartObject();
        for (const auto& item : contextItems)
        {
            writer.WritePropertyName(item.first.ToString());
            item.second->ToJson(writer);
        }
        writer.WriteEndObject();

        // Write network
        writer.WritePropertyName("network");
        writer.WriteNumber(network);

        writer.WriteEndObject();
    }

    ContractParametersContext::ContextItem* ContractParametersContext::CreateItem(const Contract& contract)
    {
        auto it = contextItems.find(contract.GetScriptHash());
        if (it != contextItems.end())
            return it->second.get();

        if (std::find(GetScriptHashes().begin(), GetScriptHashes().end(), contract.GetScriptHash()) == GetScriptHashes().end())
            return nullptr;

        auto item = std::make_unique<ContextItem>(contract);
        auto* result = item.get();
        contextItems[contract.GetScriptHash()] = std::move(item);
        return result;
    }
}
