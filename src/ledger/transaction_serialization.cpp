#include <neo/config/protocol_settings.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/coin_reference.h>
#include <neo/ledger/oracle_response.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/transaction_output.h>
#include <neo/ledger/witness.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace neo::ledger
{
io::ByteVector Transaction::GetSignData(uint32_t networkMagic) const
{
    // Create signature data by serializing the transaction data with network magic
    // This matches the C# implementation in Transaction.GetSignData
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

    // Add network magic
    writer.Write(networkMagic);

    // Convert stream to ByteVector
    std::string data = stream.str();
    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
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

    // Add network magic from protocol settings
    uint32_t networkMagic = config::ProtocolSettings::GetDefault().GetNetwork();
    writer.Write(networkMagic);

    // Convert stream to ByteVector
    std::string data = stream.str();
    return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
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
    if (attributeCount < 0 || static_cast<uint64_t>(attributeCount) > std::numeric_limits<std::size_t>::max())
        throw std::out_of_range("Invalid attribute count");

    attributes_.clear();
    attributes_.reserve(static_cast<std::size_t>(attributeCount));

    for (int64_t i = 0; i < attributeCount; i++)
    {
        TransactionAttribute attribute;
        attribute.Deserialize(reader);
        attributes_.push_back(attribute);
    }

    // Deserialize inputs
    int64_t inputCount = reader.ReadVarInt();
    if (inputCount < 0 || static_cast<uint64_t>(inputCount) > std::numeric_limits<std::size_t>::max())
        throw std::out_of_range("Invalid input count");

    inputs_.clear();
    inputs_.reserve(static_cast<std::size_t>(inputCount));

    for (int64_t i = 0; i < inputCount; i++)
    {
        CoinReference input;
        input.Deserialize(reader);
        inputs_.push_back(input);
    }

    // Deserialize outputs
    int64_t outputCount = reader.ReadVarInt();
    if (outputCount < 0 || static_cast<uint64_t>(outputCount) > std::numeric_limits<std::size_t>::max())
        throw std::out_of_range("Invalid output count");

    outputs_.clear();
    outputs_.reserve(static_cast<std::size_t>(outputCount));

    for (int64_t i = 0; i < outputCount; i++)
    {
        TransactionOutput output;
        output.Deserialize(reader);
        outputs_.push_back(output);
    }

    // Deserialize witnesses
    int64_t witnessCount = reader.ReadVarInt();
    if (witnessCount < 0 || static_cast<uint64_t>(witnessCount) > std::numeric_limits<std::size_t>::max())
        throw std::out_of_range("Invalid witness count");

    witnesses_.clear();
    witnesses_.reserve(static_cast<std::size_t>(witnessCount));

    for (int64_t i = 0; i < witnessCount; i++)
    {
        Witness witness;
        witness.Deserialize(reader);
        witnesses_.push_back(witness);
    }
}

}  // namespace neo::ledger