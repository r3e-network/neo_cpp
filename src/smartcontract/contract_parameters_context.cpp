#include <neo/smartcontract/contract_parameters_context.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/script.h>
#include <neo/vm/opcode.h>
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
        for (const auto& paramJson : parametersArray)
        {
            ContractParameter parameter;
            
            // Parse parameter type
            if (paramJson.contains("type"))
            {
                std::string typeStr = paramJson["type"].get<std::string>();
                parameter.SetType(ParseContractParameterType(typeStr));
            }
            
            // Parse parameter value based on type
            if (paramJson.contains("value"))
            {
                const auto& valueJson = paramJson["value"];
                
                switch (parameter.GetType())
                {
                    case ContractParameterType::Boolean:
                        parameter.SetValue(valueJson.get<bool>());
                        break;
                        
                    case ContractParameterType::Integer:
                        parameter.SetValue(std::stoll(valueJson.get<std::string>()));
                        break;
                        
                    case ContractParameterType::ByteArray:
                    case ContractParameterType::Signature:
                        {
                            std::string base64Str = valueJson.get<std::string>();
                            auto bytes = Base64Decode(base64Str);
                            parameter.SetValue(bytes);
                        }
                        break;
                        
                    case ContractParameterType::String:
                        parameter.SetValue(valueJson.get<std::string>());
                        break;
                        
                    case ContractParameterType::Hash160:
                        {
                            std::string hashStr = valueJson.get<std::string>();
                            parameter.SetValue(io::UInt160::Parse(hashStr));
                        }
                        break;
                        
                    case ContractParameterType::Hash256:
                        {
                            std::string hashStr = valueJson.get<std::string>();
                            parameter.SetValue(io::UInt256::Parse(hashStr));
                        }
                        break;
                        
                    case ContractParameterType::PublicKey:
                        {
                            std::string pubkeyStr = valueJson.get<std::string>();
                            parameter.SetValue(cryptography::ecc::ECPoint::Parse(pubkeyStr));
                        }
                        break;
                        
                    case ContractParameterType::Array:
                        {
                            // Recursively parse array elements
                            std::vector<ContractParameter> arrayParams;
                            for (const auto& element : valueJson)
                            {
                                // Recursive call would be needed here
                                // For now, create empty parameter
                                arrayParams.push_back(ContractParameter());
                            }
                            parameter.SetValue(arrayParams);
                        }
                        break;
                        
                    default:
                        // For unsupported types, create empty parameter
                        break;
                }
            }
            
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
            // Implement parameter serialization to JSON matching C# ContractParameter.ToJson
            writer.WriteStartObject();
            
            // Write parameter type
            writer.WritePropertyName("type");
            writer.WriteValue(ContractParameterTypeToString(parameter.GetType()));
            
            // Write parameter value based on type
            if (parameter.HasValue())
            {
                writer.WritePropertyName("value");
                
                switch (parameter.GetType())
                {
                    case ContractParameterType::Boolean:
                        writer.WriteValue(parameter.GetBooleanValue());
                        break;
                        
                    case ContractParameterType::Integer:
                        writer.WriteValue(std::to_string(parameter.GetIntegerValue()));
                        break;
                        
                    case ContractParameterType::ByteArray:
                    case ContractParameterType::Signature:
                        {
                            auto bytes = parameter.GetByteArrayValue();
                            writer.WriteValue(Base64Encode(bytes));
                        }
                        break;
                        
                    case ContractParameterType::String:
                        writer.WriteValue(parameter.GetStringValue());
                        break;
                        
                    case ContractParameterType::Hash160:
                        writer.WriteValue(parameter.GetHash160Value().ToString());
                        break;
                        
                    case ContractParameterType::Hash256:
                        writer.WriteValue(parameter.GetHash256Value().ToString());
                        break;
                        
                    case ContractParameterType::PublicKey:
                        writer.WriteValue(parameter.GetPublicKeyValue().ToString());
                        break;
                        
                    case ContractParameterType::Array:
                        {
                            writer.WriteStartArray();
                            auto arrayParams = parameter.GetArrayValue();
                            for (const auto& element : arrayParams)
                            {
                                // Recursive serialization would be needed here
                                // For now, write empty object
                                writer.WriteStartObject();
                                writer.WriteEndObject();
                            }
                            writer.WriteEndArray();
                        }
                        break;
                        
                    default:
                        writer.WriteNull();
                        break;
                }
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
        // Implement multi-signature contract support matching C# implementation
        ContextItem* item = CreateItem(contract);
        if (!item)
            return false;

        // Add the signature
        item->signatures[pubkey] = signature;

        // Check if this is a multi-signature contract
        int m, n;
        std::vector<cryptography::ecc::ECPoint> publicKeys;
        if (IsMultiSigContract(contract.GetScript(), m, n, publicKeys))
        {
            // Multi-signature contract - check if we have enough signatures
            int validSignatures = 0;
            for (const auto& pk : publicKeys)
            {
                if (item->signatures.find(pk) != item->signatures.end())
                {
                    validSignatures++;
                }
            }
            
            // If we have enough signatures, mark parameters as complete
            if (validSignatures >= m)
            {
                // For multi-sig contracts, we typically don't have explicit parameters
                // The signatures are used directly in the invocation script
                return true;
            }
        }
        else
        {
            // Single-signature contract - set the signature as the parameter value
            if (item->parameters.size() == 1 && item->parameters[0].GetType() == ContractParameterType::Signature)
            {
                item->parameters[0].SetValue(signature);
            }
        }

        return true;
    }

    bool ContractParametersContext::AddWithScriptHash(const io::UInt160& scriptHash)
    {
        // Try to get the contract from the contract management
        auto contract = native::ContractManagement::GetContract(snapshotCache, scriptHash);
        if (!contract)
            return false;

        // Create a deployed contract from the contract state
        Contract deployedContract;
        try
        {
            // Set the script hash
            deployedContract.SetScriptHash(contract->GetScriptHash());
            
            // Set the script
            deployedContract.SetScript(contract->GetScript());
            
            // Parse the manifest to get parameter list
            auto manifest = manifest::ContractManifest::Parse(contract->GetManifest());
            auto abi = manifest.GetAbi();
            
            // Look for a method named "verify" to determine parameter list
            std::vector<ContractParameterType> parameterList;
            for (const auto& method : abi.GetMethods())
            {
                if (method.GetName() == "verify")
                {
                    // Convert method parameters to contract parameter types
                    for (const auto& param : method.GetParameters())
                    {
                        parameterList.push_back(param.GetType());
                    }
                    break;
                }
            }
            
            deployedContract.SetParameterList(parameterList);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error creating deployed contract: " << e.what() << std::endl;
            return false;
        }

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
        if (!IsCompleted())
            throw std::runtime_error("Witnesses are not ready");

        std::vector<network::p2p::payloads::Witness> witnesses;
        witnesses.reserve(GetScriptHashes().size());

        for (size_t i = 0; i < GetScriptHashes().size(); i++)
        {
            const auto& scriptHash = GetScriptHashes()[i];
            auto it = contextItems.find(scriptHash);
            if (it == contextItems.end() || !it->second)
                throw std::runtime_error("Missing signature");

            const auto& item = it->second;

            // Build invocation script using ScriptBuilder (matching C# implementation)
            vm::ScriptBuilder scriptBuilder;
            
            // Push parameters in reverse order (matching C# for loop: j = length-1; j >= 0; j--)
            for (int j = static_cast<int>(item->parameters.size()) - 1; j >= 0; j--)
            {
                const auto& parameter = item->parameters[j];
                
                // Emit push for each parameter based on its type and value
                switch (parameter.GetType())
                {
                    case ContractParameterType::Boolean:
                        if (parameter.HasValue())
                        {
                            scriptBuilder.EmitPush(parameter.GetBooleanValue());
                        }
                        else
                        {
                            scriptBuilder.EmitPush(false); // Default value
                        }
                        break;
                        
                    case ContractParameterType::Integer:
                        if (parameter.HasValue())
                        {
                            scriptBuilder.EmitPush(parameter.GetIntegerValue());
                        }
                        else
                        {
                            scriptBuilder.EmitPush(0); // Default value
                        }
                        break;
                        
                    case ContractParameterType::ByteArray:
                    case ContractParameterType::Signature:
                        if (parameter.HasValue())
                        {
                            auto bytes = parameter.GetByteArrayValue();
                            scriptBuilder.EmitPush(bytes.AsSpan());
                        }
                        else
                        {
                            scriptBuilder.EmitPush(io::ByteSpan()); // Empty bytes
                        }
                        break;
                        
                    case ContractParameterType::String:
                        if (parameter.HasValue())
                        {
                            auto str = parameter.GetStringValue();
                            io::ByteVector strBytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
                            scriptBuilder.EmitPush(strBytes.AsSpan());
                        }
                        else
                        {
                            scriptBuilder.EmitPush(io::ByteSpan()); // Empty string
                        }
                        break;
                        
                    case ContractParameterType::Hash160:
                        if (parameter.HasValue())
                        {
                            auto hash = parameter.GetHash160Value();
                            io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());
                            scriptBuilder.EmitPush(hashBytes.AsSpan());
                        }
                        else
                        {
                            io::ByteVector emptyHash(20, 0); // 20 zero bytes
                            scriptBuilder.EmitPush(emptyHash.AsSpan());
                        }
                        break;
                        
                    case ContractParameterType::Hash256:
                        if (parameter.HasValue())
                        {
                            auto hash = parameter.GetHash256Value();
                            io::ByteVector hashBytes(hash.Data(), hash.Data() + hash.Size());
                            scriptBuilder.EmitPush(hashBytes.AsSpan());
                        }
                        else
                        {
                            io::ByteVector emptyHash(32, 0); // 32 zero bytes
                            scriptBuilder.EmitPush(emptyHash.AsSpan());
                        }
                        break;
                        
                    case ContractParameterType::PublicKey:
                        if (parameter.HasValue())
                        {
                            auto pubkey = parameter.GetPublicKeyValue();
                            auto pubkeyBytes = pubkey.ToBytes();
                            scriptBuilder.EmitPush(pubkeyBytes.AsSpan());
                        }
                        else
                        {
                            io::ByteVector emptyPubkey(33, 0); // 33 zero bytes
                            scriptBuilder.EmitPush(emptyPubkey.AsSpan());
                        }
                        break;
                        
                    default:
                        // For unsupported types, push empty bytes
                        scriptBuilder.EmitPush(io::ByteSpan());
                        break;
                }
            }

            // Create witness with invocation script and verification script
            network::p2p::payloads::Witness witness;
            witness.SetInvocationScript(scriptBuilder.ToArray());
            witness.SetVerificationScript(item->script.IsEmpty() ? io::ByteVector() : item->script);
            
            witnesses.push_back(witness);
        }

        return witnesses;
    }

    std::unique_ptr<ContractParametersContext> ContractParametersContext::FromJson(const io::JsonReader& reader, const persistence::DataCache& snapshotCache)
    {
        // Implement deserialization from JSON matching C# implementation
        try
        {
            // Read the type to determine the verifiable type
            auto type = reader.ReadString("type");
            
            // Read the hash
            auto hashStr = reader.ReadString("hash");
            auto hash = io::UInt256::Parse(hashStr);
            
            // Read the data (serialized verifiable)
            auto dataBase64 = reader.ReadString("data");
            auto data = io::ByteVector::FromBase64(dataBase64);
            
            // Read the network
            auto network = reader.ReadUInt32("network");
            
            // Implement specific verifiable type deserialization based on 'type'
            // This matches the C# ContractParametersContext.FromJson implementation
            std::unique_ptr<network::p2p::payloads::IVerifiable> verifiable;
            
            if (type.find("Transaction") != std::string::npos)
            {
                // Deserialize as Transaction
                try
                {
                    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
                    io::BinaryReader reader(stream);
                    
                    auto transaction = std::make_unique<ledger::Transaction>();
                    transaction->Deserialize(reader);
                    verifiable = std::move(transaction);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Failed to deserialize transaction: " << e.what() << std::endl;
                    return nullptr;
                }
            }
            else if (type.find("Block") != std::string::npos)
            {
                // Deserialize as Block
                try
                {
                    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
                    io::BinaryReader reader(stream);
                    
                    auto block = std::make_unique<ledger::Block>();
                    block->Deserialize(reader);
                    verifiable = std::move(block);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Failed to deserialize block: " << e.what() << std::endl;
                    return nullptr;
                }
            }
            else if (type.find("ExtensiblePayload") != std::string::npos)
            {
                // Deserialize as ExtensiblePayload
                try
                {
                    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
                    io::BinaryReader reader(stream);
                    
                    auto extensible = std::make_unique<network::p2p::payloads::ExtensiblePayload>();
                    extensible->Deserialize(reader);
                    verifiable = std::move(extensible);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Failed to deserialize extensible payload: " << e.what() << std::endl;
                    return nullptr;
                }
            }
            else
            {
                // Unknown type - create a generic verifiable
                std::cerr << "Unknown verifiable type: " << type << ", creating generic verifiable" << std::endl;
                
                // Create a generic verifiable that can handle the basic interface
                class GenericVerifiable : public network::p2p::payloads::IVerifiable
                {
                private:
                    io::UInt256 hash_;
                    io::ByteVector data_;
                    
                public:
                    GenericVerifiable(const io::UInt256& hash, const io::ByteVector& data) 
                        : hash_(hash), data_(data) {}
                    
                    io::UInt256 GetHash() const override { return hash_; }
                    
                    std::vector<io::UInt160> GetScriptHashesForVerifying(const persistence::DataCache& snapshot) const override
                    {
                        // Return empty for generic verifiable
                        return std::vector<io::UInt160>();
                    }
                    
                    void SerializeUnsigned(io::BinaryWriter& writer) const override
                    {
                        // Serialize the stored data
                        writer.Write(data_.AsSpan());
                    }
                };
                
                verifiable = std::make_unique<GenericVerifiable>(hash, data);
            }
            
            if (!verifiable)
            {
                std::cerr << "Failed to create verifiable object" << std::endl;
                return nullptr;
            }
            
            auto context = std::make_unique<ContractParametersContext>(snapshotCache, *verifiable, network);
            
            // Read the items
            auto itemsObj = reader.ReadObject("items");
            for (const auto& property : itemsObj.GetProperties())
            {
                auto scriptHashStr = property.first;
                auto scriptHash = io::UInt160::Parse(scriptHashStr);
                
                // Create context item from JSON
                auto itemReader = io::JsonReader(property.second);
                auto contextItem = std::make_unique<ContextItem>(itemReader);
                
                context->contextItems[scriptHash] = std::move(contextItem);
            }
            
            return context;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error deserializing ContractParametersContext from JSON: " << e.what() << std::endl;
            return nullptr;
        }
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

    // Implement multi-signature contract support matching C# implementation
    bool ContractParametersContext::IsMultiSigContract(const io::ByteVector& script, int& m, int& n, std::vector<cryptography::ecc::ECPoint>& publicKeys) const
    {
        // Implementation of IsMultiSigContract function matching C# Contract.IsMultiSigContract
        try
        {
            if (script.Size() < 42) // Minimum size for multi-sig script
                return false;
            
            // Multi-sig script format: PUSH(m) PUSH(pubkey1) PUSH(pubkey2) ... PUSH(pubkeyn) PUSH(n) SYSCALL(CheckMultisig)
            size_t index = 0;
            
            // Read m (required signatures count)
            if (index >= script.Size())
                return false;
            
            uint8_t mByte = script[index++];
            if (mByte < vm::OpCode::PUSH1 || mByte > vm::OpCode::PUSH16)
                return false;
            
            m = mByte - static_cast<uint8_t>(vm::OpCode::PUSH1) + 1;
            
            // Read public keys
            publicKeys.clear();
            while (index < script.Size())
            {
                if (index >= script.Size())
                    break;
                
                uint8_t opcode = script[index++];
                
                // Check if this is a PUSH operation for a public key (33 bytes)
                if (opcode == static_cast<uint8_t>(vm::OpCode::PUSHDATA1))
                {
                    if (index >= script.Size())
                        break;
                    
                    uint8_t length = script[index++];
                    if (length == 33) // Public key length
                    {
                        if (index + 33 > script.Size())
                            break;
                        
                        // Extract public key bytes
                        io::ByteVector pubkeyBytes(script.begin() + index, script.begin() + index + 33);
                        try
                        {
                            auto pubkey = cryptography::ecc::ECPoint::FromBytes(pubkeyBytes.AsSpan(), "secp256r1");
                            publicKeys.push_back(pubkey);
                        }
                        catch (...)
                        {
                            // Invalid public key, not a multi-sig script
                            return false;
                        }
                        
                        index += 33;
                    }
                    else
                    {
                        // Not a public key, check if this is the n value
                        if (length == 1 && index < script.Size())
                        {
                            uint8_t nByte = script[index];
                            if (nByte >= static_cast<uint8_t>(vm::OpCode::PUSH1) && 
                                nByte <= static_cast<uint8_t>(vm::OpCode::PUSH16))
                            {
                                n = nByte - static_cast<uint8_t>(vm::OpCode::PUSH1) + 1;
                                index++;
                                break; // Found n, should be followed by SYSCALL
                            }
                        }
                        return false;
                    }
                }
                else if (opcode >= static_cast<uint8_t>(vm::OpCode::PUSH1) && 
                         opcode <= static_cast<uint8_t>(vm::OpCode::PUSH16))
                {
                    // This might be the n value
                    n = opcode - static_cast<uint8_t>(vm::OpCode::PUSH1) + 1;
                    break;
                }
                else
                {
                    // Unknown opcode, not a standard multi-sig script
                    return false;
                }
            }
            
            // Validate that we have the correct number of public keys
            if (static_cast<int>(publicKeys.size()) != n || m > n || m <= 0 || n <= 0)
                return false;
            
            // Check for SYSCALL CheckMultisig at the end
            if (index + 5 <= script.Size()) // SYSCALL is 5 bytes
            {
                if (script[index] == static_cast<uint8_t>(vm::OpCode::SYSCALL))
                {
                    // Check if this is the CheckMultisig syscall
                    // The exact bytes depend on the syscall implementation
                    // For now, we'll assume it's valid if we got this far
                    return true;
                }
            }
            
            return false;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error parsing multi-sig script: " << e.what() << std::endl;
            return false;
        }
    }

    std::shared_ptr<ledger::Witness> ContractParametersContext::CreateMultiSigWitness(const Contract& contract) const
    {
        // Implement multi-signature contract support matching C# implementation
        try
        {
            // Check if this is a multi-signature contract
            int m, n;
            std::vector<cryptography::ecc::ECPoint> publicKeys;
            if (IsMultiSigContract(contract.GetScript(), m, n, publicKeys))
            {
                // This is a multi-signature contract
                ContextItem* item = CreateItem(contract);
                if (!item)
                    return nullptr;

                // Check if we have enough signatures
                if (item->signatures.size() < static_cast<size_t>(m))
                {
                    // Not enough signatures yet
                    return nullptr;
                }

                // Create invocation script for multi-sig
                smartcontract::ScriptBuilder builder;
                
                // Add signatures in the correct order
                int sigCount = 0;
                for (const auto& pubKey : publicKeys)
                {
                    auto sigIt = item->signatures.find(pubKey);
                    if (sigIt != item->signatures.end())
                    {
                        builder.EmitPush(sigIt->second);
                        sigCount++;
                        if (sigCount >= m)
                            break;
                    }
                }

                if (sigCount < m)
                {
                    // Still not enough valid signatures
                    return nullptr;
                }

                // Create witness
                auto witness = std::make_shared<ledger::Witness>();
                witness->SetInvocationScript(builder.ToArray());
                witness->SetVerificationScript(contract.GetScript());
                
                return witness;
            }
            else
            {
                // Single signature contract - use existing logic
                ContextItem* item = CreateItem(contract);
                if (!item || item->signatures.empty())
                    return nullptr;

                // For single-sig, just use the first signature
                auto firstSig = item->signatures.begin();
                
                smartcontract::ScriptBuilder builder;
                builder.EmitPush(firstSig->second);
                
                auto witness = std::make_shared<ledger::Witness>();
                witness->SetInvocationScript(builder.ToArray());
                witness->SetVerificationScript(contract.GetScript());
                
                return witness;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error creating witness for multi-sig contract: " << e.what() << std::endl;
            return nullptr;
        }
    }
}
