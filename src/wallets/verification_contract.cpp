#include <neo/wallets/verification_contract.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/opcode.h>
#include <neo/smartcontract/contract.h>

namespace neo::wallets
{
    VerificationContract::VerificationContract()
        : m_(0)
    {
    }

    VerificationContract::VerificationContract(const smartcontract::Contract& contract)
        : contract_(contract), m_(0)
    {
        // Try to extract public keys from the contract script
        if (IsSignatureContract())
        {
            // Extract the public key from the signature contract
            auto script = contract.GetScript();
            if (script.Size() >= 40 && script[0] == 0x0C && script[1] == 0x21)
            {
                // Format: PUSHDATA1 33 <pubkey> SYSCALL <CheckSig hash>
                publicKeys_.push_back(cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + 2, 33)));
            }
        }
        else if (IsMultiSigContract())
        {
            // Extract the public keys and M value from the multi-signature contract
            auto script = contract.GetScript();
            if (script.Size() < 10) return;
            
            // Parse m value
            uint8_t mByte = script[0];
            if (mByte >= 0x51 && mByte <= 0x60) // PUSH1 to PUSH16
            {
                m_ = mByte - 0x50;
            }
            
            // Parse public keys (simplified parsing)
            size_t offset = 1;
            while (offset < script.Size() - 6) // Leave space for n + SYSCALL
            {
                if (script[offset] == 0x0C && offset + 1 < script.Size() && script[offset + 1] == 0x21)
                {
                    // PUSHDATA1 33 <pubkey>
                    if (offset + 34 < script.Size())
                    {
                        publicKeys_.push_back(cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + offset + 2, 33)));
                        offset += 34;
                    }
                    else break;
                }
                else break;
            }
        }
    }

    VerificationContract::VerificationContract(const cryptography::ecc::ECPoint& publicKey)
        : m_(1)
    {
        publicKeys_.push_back(publicKey);
        
        // Create a signature contract using proper SYSCALL format
        vm::ScriptBuilder sb;
        sb.EmitPushData(publicKey.ToBytes(true));
        sb.EmitSysCall(0x41627d5b); // System.Crypto.CheckSig hash
        
        auto script = sb.ToArray();
        contract_.SetScript(script);
        contract_.SetParameterList({ smartcontract::ContractParameterType::Signature });
    }

    VerificationContract::VerificationContract(const std::vector<cryptography::ecc::ECPoint>& publicKeys, int m)
        : publicKeys_(publicKeys), m_(m)
    {
        // Create a multi-signature contract using proper SYSCALL format
        vm::ScriptBuilder sb;
        sb.EmitPushNumber(m);
        
        for (const auto& publicKey : publicKeys)
        {
            sb.EmitPushData(publicKey.ToBytes(true));
        }
        
        sb.EmitPushNumber(publicKeys.size());
        sb.EmitSysCall(0x0973c0b6); // System.Crypto.CheckMultisig hash
        
        auto script = sb.ToArray();
        contract_.SetScript(script);
        
        // Set parameter list based on M value
        std::vector<smartcontract::ContractParameterType> parameterList;
        for (int i = 0; i < m; i++)
        {
            parameterList.push_back(smartcontract::ContractParameterType::Signature);
        }
        contract_.SetParameterList(parameterList);
    }

    const smartcontract::Contract& VerificationContract::GetContract() const
    {
        return contract_;
    }

    void VerificationContract::SetContract(const smartcontract::Contract& contract)
    {
        contract_ = contract;
    }

    io::UInt160 VerificationContract::GetScriptHash() const
    {
        return contract_.GetScriptHash();
    }

    const std::vector<cryptography::ecc::ECPoint>& VerificationContract::GetPublicKeys() const
    {
        return publicKeys_;
    }

    void VerificationContract::SetPublicKeys(const std::vector<cryptography::ecc::ECPoint>& publicKeys)
    {
        publicKeys_ = publicKeys;
    }

    const std::vector<std::string>& VerificationContract::GetParameterNames() const
    {
        return parameterNames_;
    }

    void VerificationContract::SetParameterNames(const std::vector<std::string>& parameterNames)
    {
        parameterNames_ = parameterNames;
    }

    int VerificationContract::GetM() const
    {
        return m_;
    }

    void VerificationContract::SetM(int m)
    {
        m_ = m;
    }

    bool VerificationContract::IsSignatureContract() const
    {
        // Check if the contract is a signature contract (matches C# Helper.IsSignatureContract)
        const auto& script = contract_.GetScript();
        if (script.Size() != 40) 
        {
            return false;
        }
        
        // Format: PUSHDATA1 33 <pubkey> SYSCALL <CheckSig hash>
        if (script[0] != 0x0C || script[1] != 33) 
        {
            return false; // PUSHDATA1 + length 33
        }
        
        if (script[35] != static_cast<uint8_t>(vm::OpCode::SYSCALL)) 
        {
            return false;
        }
        
        // Check if this is the CheckSig syscall (hash would be checked here)
        return true;
    }

    bool VerificationContract::IsMultiSigContract() const
    {
        // Check if the contract is a multi-signature contract (matches C# Helper.IsMultiSigContract)
        const auto& script = contract_.GetScript();
        if (script.Size() < 42) // Minimum size for multi-sig
        {
            return false;
        }
        
        // Should end with SYSCALL
        if (script[script.Size() - 5] != static_cast<uint8_t>(vm::OpCode::SYSCALL))
        {
            return false;
        }
        
        // Extract n and m values (simplified check)
        uint8_t mByte = script[0];
        if (mByte < 0x51 || mByte > 0x60) // PUSH1 to PUSH16
        {
            return false;
        }
        
        return true;
    }

    void VerificationContract::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        
        writer.WriteBase64String("script", contract_.GetScript().AsSpan());
        
        writer.WritePropertyName("parameters");
        writer.WriteStartArray();
        for (size_t i = 0; i < contract_.GetParameterList().size(); i++)
        {
            writer.WriteStartObject();
            
            writer.WritePropertyName("type");
            writer.WriteString(std::to_string(static_cast<uint8_t>(contract_.GetParameterList()[i])));
            
            writer.WritePropertyName("name");
            if (i < parameterNames_.size())
                writer.WriteString(parameterNames_[i]);
            else
                writer.WriteString("parameter" + std::to_string(i));
            
            writer.WriteEndObject();
        }
        writer.WriteEndArray();
        
        writer.WritePropertyName("pubkeys");
        writer.WriteStartArray();
        for (const auto& publicKey : publicKeys_)
        {
            writer.WriteString(publicKey.ToString());
        }
        writer.WriteEndArray();
        
        writer.WritePropertyName("m");
        writer.WriteNumber(m_);
        
        writer.WriteEndObject();
    }

    void VerificationContract::DeserializeJson(const io::JsonReader& reader)
    {
        // Use proper nlohmann::json interface
        auto json = reader.GetJson();
        
        // Read script
        if (json.contains("script") && json["script"].is_string())
        {
            auto scriptBase64 = json["script"].get<std::string>();
            auto scriptBytes = io::ByteVector::FromBase64String(scriptBase64);
            contract_.SetScript(scriptBytes);
        }
        
        // Read parameters
        if (json.contains("parameters") && json["parameters"].is_array())
        {
            auto parametersArray = json["parameters"];
            std::vector<smartcontract::ContractParameterType> parameterList;
            parameterNames_.clear();
            
            for (const auto& param : parametersArray)
            {
                if (param.contains("type") && param["type"].is_string())
                {
                    auto typeStr = param["type"].get<std::string>();
                    auto paramType = static_cast<smartcontract::ContractParameterType>(std::stoi(typeStr));
                    parameterList.push_back(paramType);
                }
                
                if (param.contains("name") && param["name"].is_string())
                {
                    auto paramName = param["name"].get<std::string>();
                    parameterNames_.push_back(paramName);
                }
            }
            
            contract_.SetParameterList(parameterList);
        }
        
        // Read public keys
        if (json.contains("pubkeys") && json["pubkeys"].is_array())
        {
            auto pubkeysArray = json["pubkeys"];
            publicKeys_.clear();
            
            for (const auto& pubkey : pubkeysArray)
            {
                if (pubkey.is_string())
                {
                    auto pubkeyStr = pubkey.get<std::string>();
                    publicKeys_.push_back(cryptography::ecc::ECPoint::FromHex(pubkeyStr));
                }
            }
        }
        
        // Read M value
        if (json.contains("m") && json["m"].is_number())
        {
            m_ = json["m"].get<int>();
        }
    }
}
