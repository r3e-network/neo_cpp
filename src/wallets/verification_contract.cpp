#include <neo/wallets/verification_contract.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/op_code.h>
#include <neo/smartcontract/contract_parameter_type.h>

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
            if (script.Size() >= 35 && script[0] == 0x21 && script[34] == 0xac)
            {
                // Format: PUSHDATA1 <pubkey> CHECKSIG
                publicKeys_.push_back(cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + 1, 33)));
            }
        }
        else if (IsMultiSigContract())
        {
            // Extract the public keys and M value from the multi-signature contract
            auto script = contract.GetScript();
            int n = static_cast<int>(script[script.Size() - 2] - 0x50);
            m_ = static_cast<int>(script[0] - 0x50);
            
            int offset = 1;
            for (int i = 0; i < n; i++)
            {
                if (script[offset] == 0x21)
                {
                    // Format: PUSHDATA1 <pubkey>
                    publicKeys_.push_back(cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + offset + 1, 33)));
                    offset += 34;
                }
            }
        }
    }

    VerificationContract::VerificationContract(const cryptography::ecc::ECPoint& publicKey)
        : m_(1)
    {
        publicKeys_.push_back(publicKey);
        
        // Create a signature contract
        vm::ScriptBuilder sb;
        sb.EmitPushData(publicKey.ToBytes(true));
        sb.Emit(vm::OpCode::CHECKSIG);
        
        contract_.SetScript(sb.ToArray());
        contract_.SetParameterList({ smartcontract::ContractParameterType::Signature });
    }

    VerificationContract::VerificationContract(const std::vector<cryptography::ecc::ECPoint>& publicKeys, int m)
        : publicKeys_(publicKeys), m_(m)
    {
        // Create a multi-signature contract
        vm::ScriptBuilder sb;
        sb.EmitPushNumber(m);
        
        for (const auto& publicKey : publicKeys)
        {
            sb.EmitPushData(publicKey.ToBytes(true));
        }
        
        sb.EmitPushNumber(publicKeys.size());
        sb.Emit(vm::OpCode::CHECKMULTISIG);
        
        contract_.SetScript(sb.ToArray());
        
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

    const io::UInt160& VerificationContract::GetScriptHash() const
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
        // Check if the contract is a signature contract
        auto script = contract_.GetScript();
        return script.Size() >= 35 && script[0] == 0x21 && script[34] == 0xac;
    }

    bool VerificationContract::IsMultiSigContract() const
    {
        // Check if the contract is a multi-signature contract
        auto script = contract_.GetScript();
        if (script.Size() < 37)
            return false;
        
        if (script[script.Size() - 1] != static_cast<uint8_t>(vm::OpCode::CHECKMULTISIG))
            return false;
        
        int n = static_cast<int>(script[script.Size() - 2] - 0x50);
        if (n < 1 || n > 1024)
            return false;
        
        int m = static_cast<int>(script[0] - 0x50);
        if (m < 1 || m > n)
            return false;
        
        return true;
    }

    void VerificationContract::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        
        writer.WritePropertyName("script");
        writer.WriteBase64String(contract_.GetScript().Data(), contract_.GetScript().Size());
        
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
        // Read script
        auto scriptBase64 = reader.ReadBase64String("script");
        contract_.SetScript(scriptBase64);
        
        // Read parameters
        auto parametersArray = reader.ReadArray("parameters");
        std::vector<smartcontract::ContractParameterType> parameterList;
        parameterNames_.clear();
        
        for (size_t i = 0; i < parametersArray.size(); i++)
        {
            auto paramType = static_cast<smartcontract::ContractParameterType>(
                static_cast<uint8_t>(parametersArray[i].ReadNumber("type")));
            parameterList.push_back(paramType);
            
            auto paramName = parametersArray[i].ReadString("name");
            parameterNames_.push_back(paramName);
        }
        
        contract_.SetParameterList(parameterList);
        
        // Read public keys
        auto pubkeysArray = reader.ReadArray("pubkeys");
        publicKeys_.clear();
        
        for (size_t i = 0; i < pubkeysArray.size(); i++)
        {
            auto pubkeyStr = pubkeysArray[i].GetString();
            publicKeys_.push_back(cryptography::ecc::ECPoint::FromHex(pubkeyStr));
        }
        
        // Read M value
        m_ = static_cast<int>(reader.ReadNumber("m"));
    }
}
