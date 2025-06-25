#include <neo/ledger/transaction.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/transaction_attribute.h>
// #include <neo/ledger/coin_reference.h> - deprecated in Neo N3
// #include <neo/ledger/transaction_output.h> - deprecated in Neo N3
#include <neo/ledger/witness.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/cryptography/hash.h>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <memory>

namespace neo::ledger
{
    // Transaction basic implementation
    Transaction::Transaction()
        : type_(Type::ContractTransaction), version_(0), nonce_(0), systemFee_(0), networkFee_(0), validUntilBlock_(0), timestamp_(0)
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

    uint64_t Transaction::GetTimestamp() const
    {
        return timestamp_;
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

    std::size_t Transaction::GetSize() const
    {
        // Calculate the size of the transaction
        std::size_t size = 0;

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
            (void)input; // Suppress unused variable warning
            size += io::UInt256::Size;  // PrevHash
            size += sizeof(uint16_t);   // PrevIndex
        }

        // Outputs size (with count prefix)
        size += 1;  // Assuming output count < 0xFD
        for (const auto& output : outputs_)
        {
            (void)output; // Suppress unused variable warning
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

    void Transaction::SerializeExclusiveData(io::BinaryWriter& writer) const
    {
        (void)writer; // Suppress unused parameter warning
        // Default implementation does nothing
    }

    void Transaction::DeserializeExclusiveData(io::BinaryReader& reader)
    {
        (void)reader; // Suppress unused parameter warning
        // Default implementation does nothing
    }

} // namespace neo::ledger 