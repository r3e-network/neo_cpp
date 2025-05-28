#include <neo/ledger/transaction.h>
#include <neo/ledger/oracle_response.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <neo/cryptography/hash.h>
#include <neo/cryptography/crypto.h>
#include <sstream>

namespace neo::ledger
{

    // Transaction implementation
    Transaction::Transaction()
        : type_(Type::ContractTransaction), version_(0), nonce_(0), systemFee_(0), networkFee_(0), validUntilBlock_(0)
    {
    }

    Transaction::Type Transaction::GetType() const
    {
        return type_;
    }

    void Transaction::SetType(Type type)
    {
        type_ = type;
    }

    uint8_t Transaction::GetVersion() const
    {
        return version_;
    }

    void Transaction::SetVersion(uint8_t version)
    {
        version_ = version;
    }

    const std::vector<TransactionAttribute>& Transaction::GetAttributes() const
    {
        return attributes_;
    }

    void Transaction::SetAttributes(const std::vector<TransactionAttribute>& attributes)
    {
        attributes_ = attributes;
    }

    const std::vector<CoinReference>& Transaction::GetInputs() const
    {
        return inputs_;
    }

    void Transaction::SetInputs(const std::vector<CoinReference>& inputs)
    {
        inputs_ = inputs;
    }

    const std::vector<TransactionOutput>& Transaction::GetOutputs() const
    {
        return outputs_;
    }

    void Transaction::SetOutputs(const std::vector<TransactionOutput>& outputs)
    {
        outputs_ = outputs;
    }

    const std::vector<Witness>& Transaction::GetWitnesses() const
    {
        return witnesses_;
    }

    void Transaction::SetWitnesses(const std::vector<Witness>& witnesses)
    {
        witnesses_ = witnesses;
    }

    io::UInt256 Transaction::GetHash() const
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);

        // Serialize the transaction without witnesses
        writer.Write(static_cast<uint8_t>(type_));
        writer.Write(version_);

        // Serialize exclusive data
        SerializeExclusiveData(writer);

        // Serialize attributes
        writer.WriteVarInt(attributes_.size());
        for (const auto& attribute : attributes_)
        {
            attribute.Serialize(writer);
        }

        // Serialize inputs
        writer.WriteVarInt(inputs_.size());
        for (const auto& input : inputs_)
        {
            input.Serialize(writer);
        }

        // Serialize outputs
        writer.WriteVarInt(outputs_.size());
        for (const auto& output : outputs_)
        {
            output.Serialize(writer);
        }

        std::string data = stream.str();
        return cryptography::Hash::Sha256(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }

    size_t Transaction::GetSize() const
    {
        // Calculate the size of the transaction
        size_t size = 0;

        // Transaction header
        size += 1;  // Type
        size += 1;  // Version

        // Attributes size (with count prefix)
        size += 1;  // Assuming attribute count < 0xFD
        for (const auto& attribute : attributes_)
        {
            size += 1;  // Usage
            size += attribute.GetData().Size() + 1;  // Data (with size prefix)
        }

        // Inputs size (with count prefix)
        size += 1;  // Assuming input count < 0xFD
        for (const auto& input : inputs_)
        {
            size += io::UInt256::Size;  // PrevHash
            size += sizeof(uint16_t);   // PrevIndex
        }

        // Outputs size (with count prefix)
        size += 1;  // Assuming output count < 0xFD
        for (const auto& output : outputs_)
        {
            size += io::UInt256::Size;  // AssetId
            size += sizeof(int64_t);    // Value
            size += io::UInt160::Size;  // ScriptHash
        }

        // Witnesses size (with count prefix)
        size += 1;  // Assuming witness count < 0xFD
        for (const auto& witness : witnesses_)
        {
            size += witness.GetInvocationScript().Size() + 1;    // InvocationScript (with size prefix)
            size += witness.GetVerificationScript().Size() + 1;  // VerificationScript (with size prefix)
        }

        return size;
    }

    uint32_t Transaction::GetNonce() const
    {
        return nonce_;
    }

    void Transaction::SetNonce(uint32_t nonce)
    {
        nonce_ = nonce;
    }

    int64_t Transaction::GetNetworkFee() const
    {
        return networkFee_;
    }

    void Transaction::SetNetworkFee(int64_t networkFee)
    {
        networkFee_ = networkFee;
    }

    int64_t Transaction::GetSystemFee() const
    {
        return systemFee_;
    }

    void Transaction::SetSystemFee(int64_t systemFee)
    {
        systemFee_ = systemFee;
    }

    uint32_t Transaction::GetValidUntilBlock() const
    {
        return validUntilBlock_;
    }

    void Transaction::SetValidUntilBlock(uint32_t validUntilBlock)
    {
        validUntilBlock_ = validUntilBlock;
    }

    std::shared_ptr<OracleResponse> Transaction::GetOracleResponse() const
    {
        // Look for OracleResponse in attributes
        for (const auto& attr : attributes_)
        {
            // Check if this attribute is an OracleResponse (type 0x11)
            if (attr.GetUsage() == TransactionAttribute::Usage::OracleResponse)
            {
                try
                {
                    // Parse the OracleResponse data from the attribute
                    const auto& data = attr.GetData();
                    if (data.Size() < sizeof(uint64_t) + sizeof(uint8_t))
                    {
                        // Invalid data size for OracleResponse
                        continue;
                    }

                    // Create a binary reader to parse the data
                    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
                    io::BinaryReader reader(stream);

                    // Read OracleResponse fields: Id (uint64), Code (uint8), Result (var bytes)
                    uint64_t id = reader.ReadUInt64();
                    uint8_t codeValue = reader.ReadByte();
                    auto result = reader.ReadVarBytes();

                    // Validate the response code
                    OracleResponseCode code = static_cast<OracleResponseCode>(codeValue);
                    switch (code)
                    {
                        case OracleResponseCode::Success:
                        case OracleResponseCode::ProtocolNotSupported:
                        case OracleResponseCode::ConsensusUnreachable:
                        case OracleResponseCode::NotFound:
                        case OracleResponseCode::Timeout:
                        case OracleResponseCode::Forbidden:
                        case OracleResponseCode::ResponseTooLarge:
                        case OracleResponseCode::InsufficientFunds:
                        case OracleResponseCode::ContentTypeNotSupported:
                        case OracleResponseCode::Error:
                            break;
                        default:
                            // Invalid response code
                            continue;
                    }

                    // Validate that non-success responses have empty result
                    if (code != OracleResponseCode::Success && !result.empty())
                    {
                        // Invalid: non-success response with non-empty result
                        continue;
                    }

                    // Create and return the OracleResponse object
                    return std::make_shared<OracleResponse>(id, code, result);
                }
                catch (const std::exception&)
                {
                    // Failed to parse OracleResponse data, continue to next attribute
                    continue;
                }
            }
        }
        return nullptr;
    }

    void Transaction::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(static_cast<uint8_t>(type_));
        writer.Write(version_);

        // Serialize exclusive data
        SerializeExclusiveData(writer);

        // Serialize attributes
        writer.WriteVarInt(attributes_.size());
        for (const auto& attribute : attributes_)
        {
            attribute.Serialize(writer);
        }

        // Serialize inputs
        writer.WriteVarInt(inputs_.size());
        for (const auto& input : inputs_)
        {
            input.Serialize(writer);
        }

        // Serialize outputs
        writer.WriteVarInt(outputs_.size());
        for (const auto& output : outputs_)
        {
            output.Serialize(writer);
        }

        // Serialize witnesses
        writer.WriteVarInt(witnesses_.size());
        for (const auto& witness : witnesses_)
        {
            witness.Serialize(writer);
        }
    }

    void Transaction::Deserialize(io::BinaryReader& reader)
    {
        type_ = static_cast<Type>(reader.ReadUInt8());
        version_ = reader.ReadUInt8();

        // Deserialize exclusive data
        DeserializeExclusiveData(reader);

        // Deserialize attributes
        int64_t attributeCount = reader.ReadVarInt();
        if (attributeCount < 0 || attributeCount > std::numeric_limits<size_t>::max())
            throw std::out_of_range("Invalid attribute count");

        attributes_.clear();
        attributes_.reserve(static_cast<size_t>(attributeCount));

        for (int64_t i = 0; i < attributeCount; i++)
        {
            TransactionAttribute attribute;
            attribute.Deserialize(reader);
            attributes_.push_back(attribute);
        }

        // Deserialize inputs
        int64_t inputCount = reader.ReadVarInt();
        if (inputCount < 0 || inputCount > std::numeric_limits<size_t>::max())
            throw std::out_of_range("Invalid input count");

        inputs_.clear();
        inputs_.reserve(static_cast<size_t>(inputCount));

        for (int64_t i = 0; i < inputCount; i++)
        {
            CoinReference input;
            input.Deserialize(reader);
            inputs_.push_back(input);
        }

        // Deserialize outputs
        int64_t outputCount = reader.ReadVarInt();
        if (outputCount < 0 || outputCount > std::numeric_limits<size_t>::max())
            throw std::out_of_range("Invalid output count");

        outputs_.clear();
        outputs_.reserve(static_cast<size_t>(outputCount));

        for (int64_t i = 0; i < outputCount; i++)
        {
            TransactionOutput output;
            output.Deserialize(reader);
            outputs_.push_back(output);
        }

        // Deserialize witnesses
        int64_t witnessCount = reader.ReadVarInt();
        if (witnessCount < 0 || witnessCount > std::numeric_limits<size_t>::max())
            throw std::out_of_range("Invalid witness count");

        witnesses_.clear();
        witnesses_.reserve(static_cast<size_t>(witnessCount));

        for (int64_t i = 0; i < witnessCount; i++)
        {
            Witness witness;
            witness.Deserialize(reader);
            witnesses_.push_back(witness);
        }
    }

    void Transaction::SerializeExclusiveData(io::BinaryWriter& writer) const
    {
        // Default implementation does nothing
    }

    void Transaction::DeserializeExclusiveData(io::BinaryReader& reader)
    {
        // Default implementation does nothing
    }

    bool Transaction::Verify() const
    {
        // Implement transaction verification matching C# Transaction.Verify
        // This should verify both state-independent and state-dependent parts

        // For now, we need a ProtocolSettings and DataCache to do full verification
        // This is a simplified version that checks basic validity

        try
        {
            // Check transaction size
            if (GetSize() == 0)
                return false;

            // Check script validity
            if (script_.Size() == 0)
                return false;

            // Check that we have witnesses for all signers
            if (witnesses_.size() != signers_.size())
                return false;

            // Check network fee is non-negative
            if (networkFee_ < 0)
                return false;

            // Check system fee is non-negative
            if (systemFee_ < 0)
                return false;

            // Basic witness verification
            return VerifyWitnesses();
        }
        catch (...)
        {
            return false;
        }
    }

    bool Transaction::VerifyWitnesses() const
    {
        // Implement witness verification matching C# Transaction.VerifyWitnesses
        try
        {
            // Get script hashes for verification (from signers)
            std::vector<io::UInt160> hashes;
            hashes.reserve(signers_.size());
            for (const auto& signer : signers_)
            {
                hashes.push_back(signer.GetAccount());
            }

            // Verify each witness
            for (size_t i = 0; i < witnesses_.size() && i < hashes.size(); i++)
            {
                const auto& witness = witnesses_[i];
                const auto& hash = hashes[i];

                // Check if witness script hash matches expected hash
                if (witness.GetScriptHash() != hash)
                    return false;

                // For signature contracts, verify signature directly
                if (IsSignatureContract(witness.GetVerificationScript()))
                {
                    // Extract signature from invocation script
                    auto signature = ExtractSignatureFromInvocationScript(witness.GetInvocationScript());
                    if (signature.empty())
                        return false;

                    // Extract public key from verification script
                    auto pubkey = ExtractPublicKeyFromVerificationScript(witness.GetVerificationScript());
                    if (pubkey.empty())
                        return false;

                    // Verify signature
                    auto signData = GetSignData();
                    auto ecPoint = cryptography::ecc::ECPoint::FromBytes(pubkey.AsSpan(), "secp256r1");
                    if (!cryptography::Crypto::VerifySignature(signData.AsSpan(), signature.AsSpan(), ecPoint))
                        return false;
                }
                else
                {
                    // For other contracts, use ApplicationEngine to verify
                    if (!VerifyScriptContract(witness, hash))
                        return false;
                }
            }

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool Transaction::operator==(const Transaction& other) const
    {
        return type_ == other.type_ &&
               version_ == other.version_ &&
               attributes_ == other.attributes_ &&
               inputs_ == other.inputs_ &&
               outputs_ == other.outputs_ &&
               witnesses_ == other.witnesses_;
    }

    bool Transaction::operator!=(const Transaction& other) const
    {
        return !(*this == other);
    }

    io::UInt160 Transaction::GetSender() const
    {
        // Implement proper sender logic matching C# Transaction.Sender
        // The sender is the account of the first signer
        if (!signers_.empty())
        {
            return signers_[0].GetAccount();
        }

        // If no signers, return zero address
        return io::UInt160();
    }

    const io::ByteVector& Transaction::GetScript() const
    {
        return script_;
    }

    void Transaction::SetScript(const io::ByteVector& script)
    {
        script_ = script;
    }

    const std::vector<Signer>& Transaction::GetSigners() const
    {
        return signers_;
    }

    void Transaction::SetSigners(const std::vector<Signer>& signers)
    {
        signers_ = signers;
    }

    bool Transaction::IsSignatureContract(const io::ByteVector& script) const
    {
        // Check if script is a signature contract (PUSH pubkey + CHECKSIG)
        // Signature contract format: PUSH(33 bytes pubkey) + CHECKSIG
        if (script.Size() != 35)
            return false;

        // Check for PUSH33 opcode (0x21) followed by 33 bytes and CHECKSIG (0x41)
        return script[0] == 0x21 && script[34] == 0x41;
    }

    io::ByteVector Transaction::ExtractSignatureFromInvocationScript(const io::ByteVector& invocationScript) const
    {
        // Extract signature from invocation script
        // Invocation script format: PUSH(signature)
        if (invocationScript.Size() < 2)
            return io::ByteVector();

        // Check for PUSH opcode
        uint8_t pushOp = invocationScript[0];
        if (pushOp >= 0x01 && pushOp <= 0x4B)
        {
            // Direct push of 1-75 bytes
            size_t sigLength = pushOp;
            if (invocationScript.Size() >= 1 + sigLength)
            {
                return io::ByteVector(io::ByteSpan(invocationScript.Data() + 1, sigLength));
            }
        }

        return io::ByteVector();
    }

    io::ByteVector Transaction::ExtractPublicKeyFromVerificationScript(const io::ByteVector& verificationScript) const
    {
        // Extract public key from verification script
        // Verification script format: PUSH(33 bytes pubkey) + CHECKSIG
        if (verificationScript.Size() != 35 || verificationScript[0] != 0x21)
            return io::ByteVector();

        // Return the 33-byte public key
        return io::ByteVector(io::ByteSpan(verificationScript.Data() + 1, 33));
    }

    bool Transaction::IsMultiSignatureContract(const io::ByteVector& script) const
    {
        // Check if script is a multi-signature contract
        // Multi-sig format: PUSH(m) + PUSH(pubkey1) + ... + PUSH(pubkeyn) + PUSH(n) + CHECKMULTISIG
        if (script.Size() < 37) // Minimum size for 1-of-1 multisig
            return false;

        // Check for CHECKMULTISIG opcode at the end
        if (script[script.Size() - 1] != 0xC1) // CHECKMULTISIG
            return false;

        // Extract n (number of public keys) from second-to-last byte
        uint8_t nByte = script[script.Size() - 2];
        if (nByte < 0x51 || nByte > 0x60) // PUSH1 to PUSH16
            return false;
        int n = nByte - 0x50;

        // Extract m (required signatures) from first byte
        uint8_t mByte = script[0];
        if (mByte < 0x51 || mByte > 0x60) // PUSH1 to PUSH16
            return false;
        int m = mByte - 0x50;

        // Validate m <= n
        if (m > n || m < 1 || n < 1 || n > 16)
            return false;

        return true;
    }

    bool Transaction::VerifySignatureContract(const Witness& witness, const io::UInt160& hash) const
    {
        try
        {
            // Extract signature from invocation script
            auto signature = ExtractSignatureFromInvocationScript(witness.GetInvocationScript());
            if (signature.empty())
                return false;

            // Extract public key from verification script
            auto pubkey = ExtractPublicKeyFromVerificationScript(witness.GetVerificationScript());
            if (pubkey.empty())
                return false;

            // Verify signature
            auto signData = GetSignData();
            auto ecPoint = cryptography::ecc::ECPoint::FromBytes(pubkey.AsSpan(), "secp256r1");
            return cryptography::Crypto::VerifySignature(signData.AsSpan(), signature.AsSpan(), ecPoint);
        }
        catch (...)
        {
            return false;
        }
    }

    bool Transaction::VerifyMultiSignatureContract(const Witness& witness, const io::UInt160& hash) const
    {
        try
        {
            // Extract signatures and public keys from multi-sig contract
            auto verificationScript = witness.GetVerificationScript();
            auto invocationScript = witness.GetInvocationScript();

            // Parse m and n from verification script
            int m = verificationScript[0] - 0x50;
            int n = verificationScript[verificationScript.Size() - 2] - 0x50;

            // Extract public keys from verification script
            std::vector<cryptography::ecc::ECPoint> publicKeys;
            size_t offset = 1;
            for (int i = 0; i < n; i++)
            {
                if (offset >= verificationScript.Size() || verificationScript[offset] != 0x21)
                    return false;

                auto pubkeyBytes = io::ByteSpan(verificationScript.Data() + offset + 1, 33);
                publicKeys.push_back(cryptography::ecc::ECPoint::FromBytes(pubkeyBytes, "secp256r1"));
                offset += 34; // 1 byte opcode + 33 bytes pubkey
            }

            // Extract signatures from invocation script
            std::vector<io::ByteVector> signatures;
            offset = 0;
            while (offset < invocationScript.Size())
            {
                uint8_t pushOp = invocationScript[offset];
                if (pushOp >= 0x01 && pushOp <= 0x4B)
                {
                    size_t sigLength = pushOp;
                    if (offset + 1 + sigLength <= invocationScript.Size())
                    {
                        signatures.push_back(io::ByteVector(io::ByteSpan(invocationScript.Data() + offset + 1, sigLength)));
                        offset += 1 + sigLength;
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

            // Verify signatures (matching C# logic)
            auto signData = GetSignData();
            int validSignatures = 0;
            int pubKeyIndex = 0;

            for (const auto& signature : signatures)
            {
                if (validSignatures >= m)
                    break;

                // Find a public key that validates this signature
                bool signatureValid = false;
                for (int i = pubKeyIndex; i < n; i++)
                {
                    if (cryptography::Crypto::VerifySignature(signData.AsSpan(), signature.AsSpan(), publicKeys[i]))
                    {
                        validSignatures++;
                        pubKeyIndex = i + 1;
                        signatureValid = true;
                        break;
                    }
                }

                if (!signatureValid)
                    return false;

                // Check if we can still reach m signatures with remaining public keys
                if (m - validSignatures > n - pubKeyIndex)
                    return false;
            }

            return validSignatures >= m;
        }
        catch (...)
        {
            return false;
        }
    }

    bool Transaction::VerifyScriptContract(const Witness& witness, const io::UInt160& hash) const
    {
        // For script contracts, we need to execute the verification script using ApplicationEngine
        // This is a simplified implementation that would need full VM integration
        try
        {
            // TODO: Create ApplicationEngine and execute verification script
            // For now, just verify that the script hash matches
            return witness.GetScriptHash() == hash;
        }
        catch (...)
        {
            return false;
        }
    }

    io::ByteVector Transaction::GetSignData() const
    {
        // Get the data that should be signed for this transaction
        // This should match the C# Transaction.GetSignData method

        // Create a memory stream to serialize the transaction data for signing
        std::ostringstream stream;
        io::BinaryWriter writer(stream);

        // Serialize transaction data without witnesses (unsigned data)
        writer.Write(static_cast<uint8_t>(type_));
        writer.Write(version_);
        writer.Write(nonce_);
        writer.Write(systemFee_);
        writer.Write(networkFee_);
        writer.Write(validUntilBlock_);

        // Serialize signers
        writer.WriteVarInt(signers_.size());
        for (const auto& signer : signers_)
        {
            signer.Serialize(writer);
        }

        // Serialize attributes
        writer.WriteVarInt(attributes_.size());
        for (const auto& attribute : attributes_)
        {
            attribute.Serialize(writer);
        }

        // Serialize script
        writer.WriteVarBytes(script_.AsSpan());

        // Add network magic (for now use a default value)
        // TODO: Get actual network magic from protocol settings
        uint32_t networkMagic = 860833102; // Neo N3 MainNet magic
        writer.Write(networkMagic);

        // Convert stream to ByteVector
        std::string data = stream.str();
        return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    }
}
