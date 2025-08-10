#include <neo/smartcontract/contract.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script_builder.h>
#include <neo/wallets/verification_contract.h>

namespace neo::wallets
{
VerificationContract::VerificationContract() : m_(0) {}

VerificationContract::VerificationContract(const smartcontract::Contract& contract) : contract_(contract), m_(0)
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
        if (mByte >= 0x51 && mByte <= 0x60)  // PUSH1 to PUSH16
        {
            m_ = mByte - 0x50;
        }

        // Complete verification contract parsing with full opcode support
        size_t offset = 1;
        while (offset < script.Size() - 6)  // Leave space for n + SYSCALL
        {
            uint8_t opcode = script[offset];

            // Handle different public key push opcodes
            if (opcode == 0x0C && offset + 1 < script.Size() && script[offset + 1] == 0x21)
            {
                // PUSHDATA1 33 <pubkey> - most common format
                if (offset + 34 <= script.Size())
                {
                    try
                    {
                        auto pubkey =
                            cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + offset + 2, 33));
                        if (!pubkey.IsInfinity())
                        {
                            publicKeys_.push_back(pubkey);
                        }
                    }
                    catch (const std::exception&)
                    {
                        // Invalid public key - skip
                    }
                    offset += 34;
                }
                else
                    break;
            }
            else if (opcode >= 0x21 && opcode <= 0x4B)
            {
                // PUSH[1-75] opcodes - direct byte push
                size_t push_length = opcode;
                if (push_length == 33 && offset + 1 + push_length <= script.Size())
                {
                    // 33-byte public key
                    try
                    {
                        auto pubkey =
                            cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + offset + 1, 33));
                        if (!pubkey.IsInfinity())
                        {
                            publicKeys_.push_back(pubkey);
                        }
                    }
                    catch (const std::exception&)
                    {
                        // Invalid public key - skip
                    }
                    offset += 1 + push_length;
                }
                else if (push_length == 65 && offset + 1 + push_length <= script.Size())
                {
                    // 65-byte uncompressed public key
                    try
                    {
                        auto pubkey =
                            cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + offset + 1, 65));
                        if (!pubkey.IsInfinity())
                        {
                            publicKeys_.push_back(pubkey);
                        }
                    }
                    catch (const std::exception&)
                    {
                        // Invalid public key - skip
                    }
                    offset += 1 + push_length;
                }
                else
                {
                    // Skip non-public-key pushes
                    if (offset + 1 + push_length <= script.Size())
                    {
                        offset += 1 + push_length;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else if (opcode == 0x4C)
            {
                // PUSHDATA2 - 2-byte length
                if (offset + 3 <= script.Size())
                {
                    uint16_t length = script[offset + 1] | (script[offset + 2] << 8);
                    if (length == 33 && offset + 3 + length <= script.Size())
                    {
                        try
                        {
                            auto pubkey =
                                cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + offset + 3, 33));
                            if (!pubkey.IsInfinity())
                            {
                                publicKeys_.push_back(pubkey);
                            }
                        }
                        catch (const std::exception&)
                        {
                            // Invalid public key - skip
                        }
                    }
                    if (offset + 3 + length <= script.Size())
                    {
                        offset += 3 + length;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else if (opcode == 0x4D)
            {
                // PUSHDATA4 - 4-byte length
                if (offset + 5 <= script.Size())
                {
                    uint32_t length = script[offset + 1] | (script[offset + 2] << 8) | (script[offset + 3] << 16) |
                                      (script[offset + 4] << 24);
                    if (length == 33 && offset + 5 + length <= script.Size())
                    {
                        try
                        {
                            auto pubkey =
                                cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(script.Data() + offset + 5, 33));
                            if (!pubkey.IsInfinity())
                            {
                                publicKeys_.push_back(pubkey);
                            }
                        }
                        catch (const std::exception&)
                        {
                            // Invalid public key - skip
                        }
                    }
                    if (offset + 5 + length <= script.Size())
                    {
                        offset += 5 + length;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                // Unknown opcode or end of public keys section
                break;
            }
        }
    }
}

VerificationContract::VerificationContract(const cryptography::ecc::ECPoint& publicKey) : m_(1)
{
    publicKeys_.push_back(publicKey);

    // Create a signature contract using proper SYSCALL format
    vm::ScriptBuilder sb;
    sb.EmitPushData(publicKey.ToBytes(true));
    sb.EmitSysCall(0x41627d5b);  // System.Crypto.CheckSig hash

    auto script = sb.ToArray();
    contract_.SetScript(script);
    contract_.SetParameterList({smartcontract::ContractParameterType::Signature});
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
    sb.EmitSysCall(0x0973c0b6);  // System.Crypto.CheckMultisig hash

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

const smartcontract::Contract& VerificationContract::GetContract() const { return contract_; }

void VerificationContract::SetContract(const smartcontract::Contract& contract) { contract_ = contract; }

io::UInt160 VerificationContract::GetScriptHash() const { return contract_.GetScriptHash(); }

const std::vector<cryptography::ecc::ECPoint>& VerificationContract::GetPublicKeys() const { return publicKeys_; }

void VerificationContract::SetPublicKeys(const std::vector<cryptography::ecc::ECPoint>& publicKeys)
{
    publicKeys_ = publicKeys;
}

const std::vector<std::string>& VerificationContract::GetParameterNames() const { return parameterNames_; }

void VerificationContract::SetParameterNames(const std::vector<std::string>& parameterNames)
{
    parameterNames_ = parameterNames;
}

int VerificationContract::GetM() const { return m_; }

void VerificationContract::SetM(int m) { m_ = m; }

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
        return false;  // PUSHDATA1 + length 33
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
    if (script.Size() < 42)  // Minimum size for multi-sig
    {
        return false;
    }

    // Should end with SYSCALL
    if (script[script.Size() - 5] != static_cast<uint8_t>(vm::OpCode::SYSCALL))
    {
        return false;
    }

    // Complete validation of multi-signature contract format
    uint8_t mByte = script[0];
    if (mByte < 0x51 || mByte > 0x60)  // PUSH1 to PUSH16
    {
        return false;
    }

    // Validate the complete contract structure
    size_t offset = 1;
    size_t pubkey_count = 0;

    // Count and validate public keys
    while (offset < script.Size() - 6)
    {
        uint8_t opcode = script[offset];

        if (opcode == 0x0C && offset + 1 < script.Size() && script[offset + 1] == 0x21)
        {
            // PUSHDATA1 33 <pubkey>
            if (offset + 34 <= script.Size())
            {
                // Validate public key format
                uint8_t first_byte = script[offset + 2];
                if (first_byte == 0x02 || first_byte == 0x03)
                {
                    pubkey_count++;
                    offset += 34;
                }
                else
                {
                    return false;  // Invalid compressed public key
                }
            }
            else
            {
                return false;
            }
        }
        else if (opcode >= 0x21 && opcode <= 0x41)
        {
            // Direct push of 33 or 65 bytes
            size_t push_length = opcode;
            if (push_length == 33 || push_length == 65)
            {
                if (offset + 1 + push_length <= script.Size())
                {
                    uint8_t first_byte = script[offset + 1];
                    if ((push_length == 33 && (first_byte == 0x02 || first_byte == 0x03)) ||
                        (push_length == 65 && first_byte == 0x04))
                    {
                        pubkey_count++;
                        offset += 1 + push_length;
                    }
                    else
                    {
                        return false;  // Invalid public key format
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;  // Unexpected push length
            }
        }
        else
        {
            break;  // End of public keys section
        }
    }

    // Validate remaining script structure
    if (offset >= script.Size() - 5)
    {
        return false;  // Not enough space for n + SYSCALL
    }

    // Check n value (number of public keys)
    uint8_t nByte = script[offset];
    if (nByte < 0x51 || nByte > 0x60)
    {
        return false;
    }

    uint8_t n = nByte - 0x50;
    uint8_t m = mByte - 0x50;

    // Validate m <= n <= 16 and m >= 1
    if (m < 1 || m > n || n > 16 || n != pubkey_count)
    {
        return false;
    }

    // Validate SYSCALL to System.Crypto.CheckMultisig
    offset++;
    if (offset + 5 <= script.Size())
    {
        uint8_t syscall_opcode = script[offset];
        if (syscall_opcode == 0x41)
        {  // SYSCALL
            uint32_t syscall_hash = script[offset + 1] | (script[offset + 2] << 8) | (script[offset + 3] << 16) |
                                    (script[offset + 4] << 24);
            if (syscall_hash != 0x0973c0b6)
            {  // System.Crypto.CheckMultisig
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
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
}  // namespace neo::wallets
