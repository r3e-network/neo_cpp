#include <neo/cryptography/hash.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/transaction.h>
#include <sstream>

namespace neo::ledger
{
// Witness JSON serialization/deserialization
void Witness::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("invocation", invocationScript_.AsSpan());
    writer.Write("verification", verificationScript_.AsSpan());
}

void Witness::DeserializeJson(const io::JsonReader& reader)
{
    invocationScript_ = reader.ReadBytes("invocation");
    verificationScript_ = reader.ReadBytes("verification");
}

// CoinReference JSON serialization/deserialization
void CoinReference::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("txid", prevHash_);
    writer.Write("vout", prevIndex_);
}

void CoinReference::DeserializeJson(const io::JsonReader& reader)
{
    prevHash_ = reader.ReadUInt256("txid");
    prevIndex_ = reader.ReadUInt16("vout");
}

// TransactionOutput JSON serialization/deserialization
void TransactionOutput::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("asset", assetId_);
    writer.Write("value", value_);
    writer.Write("address", scriptHash_);
}

void TransactionOutput::DeserializeJson(const io::JsonReader& reader)
{
    assetId_ = reader.ReadUInt256("asset");
    value_ = reader.ReadFixed8("value");
    scriptHash_ = reader.ReadUInt160("address");
}

// TransactionAttribute JSON serialization/deserialization
void TransactionAttribute::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("usage", static_cast<uint8_t>(usage_));
    writer.Write("data", data_.AsSpan());
}

void TransactionAttribute::DeserializeJson(const io::JsonReader& reader)
{
    usage_ = static_cast<Usage>(reader.ReadUInt8("usage"));
    data_ = reader.ReadBytes("data");
}

// Transaction JSON serialization/deserialization
void Transaction::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("txid", GetHash());
    writer.Write("type", static_cast<uint8_t>(type_));
    writer.Write("version", version_);

    // Serialize attributes
    nlohmann::json attrArray = nlohmann::json::array();
    for (const auto& attr : attributes_)
    {
        nlohmann::json attrJson = nlohmann::json::object();
        io::JsonWriter attrWriter(attrJson);
        attr.SerializeJson(attrWriter);
        attrArray.push_back(attrJson);
    }
    writer.Write("attributes", attrArray);

    // Serialize inputs
    nlohmann::json inputArray = nlohmann::json::array();
    for (const auto& input : inputs_)
    {
        nlohmann::json inputJson = nlohmann::json::object();
        io::JsonWriter inputWriter(inputJson);
        input.SerializeJson(inputWriter);
        inputArray.push_back(inputJson);
    }
    writer.Write("vin", inputArray);

    // Serialize outputs
    nlohmann::json outputArray = nlohmann::json::array();
    for (const auto& output : outputs_)
    {
        nlohmann::json outputJson = nlohmann::json::object();
        io::JsonWriter outputWriter(outputJson);
        output.SerializeJson(outputWriter);
        outputArray.push_back(outputJson);
    }
    writer.Write("vout", outputArray);

    // Serialize witnesses
    nlohmann::json witnessArray = nlohmann::json::array();
    for (const auto& witness : witnesses_)
    {
        nlohmann::json witnessJson = nlohmann::json::object();
        io::JsonWriter witnessWriter(witnessJson);
        witness.SerializeJson(witnessWriter);
        witnessArray.push_back(witnessJson);
    }
    writer.Write("witnesses", witnessArray);

    // Calculate actual transaction size
    try
    {
        // Get the actual serialized size of the transaction
        size_t actual_size = GetSize();
        writer.Write("size", static_cast<int64_t>(actual_size));
    }
    catch (const std::exception& e)
    {
        // Fallback to calculating size from components if GetSize() is not available
        size_t estimated_size = 0;

        // Transaction header size
        estimated_size += sizeof(uint8_t);   // version
        estimated_size += sizeof(uint32_t);  // nonce
        estimated_size += sizeof(int64_t);   // systemFee
        estimated_size += sizeof(int64_t);   // networkFee
        estimated_size += sizeof(uint32_t);  // validUntilBlock

        // Add signers size
        const auto& signers = GetSigners();
        estimated_size += GetVarintSize(signers.size());
        for (const auto& signer : signers)
        {
            estimated_size += signer.GetSize();
        }

        // Add attributes size
        const auto& attributes = GetAttributes();
        estimated_size += GetVarintSize(attributes.size());
        for (const auto& attr : attributes)
        {
            if (attr)
            {
                estimated_size += attr->GetSize();
            }
        }

        // Add script size
        const auto& script = GetScript();
        estimated_size += GetVarintSize(script.Size());
        estimated_size += script.Size();

        // Add witnesses size
        const auto& witnesses = GetWitnesses();
        estimated_size += GetVarintSize(witnesses.size());
        for (const auto& witness : witnesses)
        {
            estimated_size += witness.GetSize();
        }

        writer.Write("size", static_cast<int64_t>(estimated_size));
    }
}

void Transaction::DeserializeJson(const io::JsonReader& reader)
{
    type_ = static_cast<Type>(reader.ReadUInt8("type"));
    version_ = reader.ReadUInt8("version");

    // Deserialize attributes
    auto attrArray = reader.ReadArray("attributes");
    attributes_.clear();
    attributes_.reserve(attrArray.size());

    for (const auto& attrJson : attrArray)
    {
        TransactionAttribute attr;
        io::JsonReader attrReader(attrJson);
        attr.DeserializeJson(attrReader);
        attributes_.push_back(attr);
    }

    // Deserialize inputs
    auto inputArray = reader.ReadArray("vin");
    inputs_.clear();
    inputs_.reserve(inputArray.size());

    for (const auto& inputJson : inputArray)
    {
        CoinReference input;
        io::JsonReader inputReader(inputJson);
        input.DeserializeJson(inputReader);
        inputs_.push_back(input);
    }

    // Deserialize outputs
    auto outputArray = reader.ReadArray("vout");
    outputs_.clear();
    outputs_.reserve(outputArray.size());

    for (const auto& outputJson : outputArray)
    {
        TransactionOutput output;
        io::JsonReader outputReader(outputJson);
        output.DeserializeJson(outputReader);
        outputs_.push_back(output);
    }

    // Deserialize witnesses
    auto witnessArray = reader.ReadArray("witnesses");
    witnesses_.clear();
    witnesses_.reserve(witnessArray.size());

    for (const auto& witnessJson : witnessArray)
    {
        Witness witness;
        io::JsonReader witnessReader(witnessJson);
        witness.DeserializeJson(witnessReader);
        witnesses_.push_back(witness);
    }
}
}  // namespace neo::ledger
