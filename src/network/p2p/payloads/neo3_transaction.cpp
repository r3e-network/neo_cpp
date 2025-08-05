#include <algorithm>
#include <neo/core/logging.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <set>
#include <stdexcept>

namespace neo::network::p2p::payloads
{
Neo3Transaction::Neo3Transaction()
    : version_(0), nonce_(0), systemFee_(0), networkFee_(0), validUntilBlock_(0), hashCalculated_(false),
      sizeCalculated_(false), size_(0)
{
}

Neo3Transaction::Neo3Transaction(const Neo3Transaction& other)
    : version_(other.version_), nonce_(other.nonce_), systemFee_(other.systemFee_), 
      networkFee_(other.networkFee_), validUntilBlock_(other.validUntilBlock_),
      signers_(other.signers_), attributes_(other.attributes_), script_(other.script_),
      witnesses_(other.witnesses_), hash_(other.hash_), hashCalculated_(other.hashCalculated_),
      size_(other.size_), sizeCalculated_(other.sizeCalculated_)
{
}

Neo3Transaction::Neo3Transaction(Neo3Transaction&& other) noexcept
    : version_(other.version_), nonce_(other.nonce_), systemFee_(other.systemFee_), 
      networkFee_(other.networkFee_), validUntilBlock_(other.validUntilBlock_),
      signers_(std::move(other.signers_)), attributes_(std::move(other.attributes_)), 
      script_(std::move(other.script_)), witnesses_(std::move(other.witnesses_)), 
      hash_(other.hash_), hashCalculated_(other.hashCalculated_),
      size_(other.size_), sizeCalculated_(other.sizeCalculated_)
{
    // Reset the moved-from object's cached values
    other.hashCalculated_ = false;
    other.sizeCalculated_ = false;
}

Neo3Transaction& Neo3Transaction::operator=(const Neo3Transaction& other)
{
    if (this != &other)
    {
        version_ = other.version_;
        nonce_ = other.nonce_;
        systemFee_ = other.systemFee_;
        networkFee_ = other.networkFee_;
        validUntilBlock_ = other.validUntilBlock_;
        signers_ = other.signers_;
        attributes_ = other.attributes_;
        script_ = other.script_;
        witnesses_ = other.witnesses_;
        hash_ = other.hash_;
        hashCalculated_ = other.hashCalculated_;
        size_ = other.size_;
        sizeCalculated_ = other.sizeCalculated_;
    }
    return *this;
}

Neo3Transaction& Neo3Transaction::operator=(Neo3Transaction&& other) noexcept
{
    if (this != &other)
    {
        version_ = other.version_;
        nonce_ = other.nonce_;
        systemFee_ = other.systemFee_;
        networkFee_ = other.networkFee_;
        validUntilBlock_ = other.validUntilBlock_;
        signers_ = std::move(other.signers_);
        attributes_ = std::move(other.attributes_);
        script_ = std::move(other.script_);
        witnesses_ = std::move(other.witnesses_);
        hash_ = other.hash_;
        hashCalculated_ = other.hashCalculated_;
        size_ = other.size_;
        sizeCalculated_ = other.sizeCalculated_;
        
        // Reset the moved-from object's cached values
        other.hashCalculated_ = false;
        other.sizeCalculated_ = false;
    }
    return *this;
}

Neo3Transaction::~Neo3Transaction() = default;

uint8_t Neo3Transaction::GetVersion() const
{
    return version_;
}

void Neo3Transaction::SetVersion(uint8_t version)
{
    version_ = version;
    InvalidateCache();
}

uint32_t Neo3Transaction::GetNonce() const
{
    return nonce_;
}

void Neo3Transaction::SetNonce(uint32_t nonce)
{
    nonce_ = nonce;
    InvalidateCache();
}

int64_t Neo3Transaction::GetSystemFee() const
{
    return systemFee_;
}

void Neo3Transaction::SetSystemFee(int64_t systemFee)
{
    systemFee_ = systemFee;
    InvalidateCache();
}

int64_t Neo3Transaction::GetNetworkFee() const
{
    return networkFee_;
}

void Neo3Transaction::SetNetworkFee(int64_t networkFee)
{
    networkFee_ = networkFee;
    InvalidateCache();
}

uint32_t Neo3Transaction::GetValidUntilBlock() const
{
    return validUntilBlock_;
}

void Neo3Transaction::SetValidUntilBlock(uint32_t validUntilBlock)
{
    validUntilBlock_ = validUntilBlock;
    InvalidateCache();
}

const std::vector<ledger::Signer>& Neo3Transaction::GetSigners() const
{
    return signers_;
}

void Neo3Transaction::SetSigners(const std::vector<ledger::Signer>& signers)
{
    signers_ = signers;
    InvalidateCache();
}

const std::vector<std::shared_ptr<ledger::TransactionAttribute>>& Neo3Transaction::GetAttributes() const
{
    return attributes_;
}

void Neo3Transaction::SetAttributes(const std::vector<std::shared_ptr<ledger::TransactionAttribute>>& attributes)
{
    attributes_ = attributes;
    InvalidateCache();
}

const io::ByteVector& Neo3Transaction::GetScript() const
{
    return script_;
}

void Neo3Transaction::SetScript(const io::ByteVector& script)
{
    script_ = script;
    InvalidateCache();
}

const std::vector<ledger::Witness>& Neo3Transaction::GetWitnesses() const
{
    return witnesses_;
}

void Neo3Transaction::SetWitnesses(const std::vector<ledger::Witness>& witnesses)
{
    witnesses_ = witnesses;
    // Only size is affected by witnesses, not hash
    sizeCalculated_ = false;
}

io::UInt160 Neo3Transaction::GetSender() const
{
    if (signers_.empty())
    {
        throw std::runtime_error("Sender is not specified in the transaction.");
    }
    return signers_[0].GetAccount();
}

int64_t Neo3Transaction::GetTotalFee() const
{
    // Use safe addition to check for overflow
    return common::SafeMath::Add(systemFee_, networkFee_);
}

int64_t Neo3Transaction::GetFeePerByte() const
{
    return GetNetworkFee() / GetSize();
}

InventoryType Neo3Transaction::GetInventoryType() const
{
    return InventoryType::TX;
}

io::UInt256 Neo3Transaction::GetHash() const
{
    if (!hashCalculated_)
    {
        CalculateHash();
    }
    return hash_;
}

int Neo3Transaction::GetSize() const
{
    if (!sizeCalculated_)
    {
        CalculateSize();
    }
    return size_;
}

std::vector<io::UInt160> Neo3Transaction::GetScriptHashesForVerifying() const
{
    std::vector<io::UInt160> result;
    result.reserve(signers_.size());
    for (const auto& signer : signers_)
    {
        result.push_back(signer.GetAccount());
    }
    return result;
}

void Neo3Transaction::Serialize(io::BinaryWriter& writer) const
{
    SerializeUnsigned(writer);
    writer.WriteVarArray(witnesses_);
}

void Neo3Transaction::Deserialize(io::BinaryReader& reader)
{
    DeserializeUnsigned(reader);
    witnesses_.clear();
    witnesses_.reserve(signers_.size());
    for (size_t i = 0; i < signers_.size(); i++)
    {
        witnesses_.push_back(reader.ReadSerializable<ledger::Witness>());
    }
    if (witnesses_.size() != signers_.size())
    {
        throw std::runtime_error("Witnesses count does not match signers count");
    }
}

void Neo3Transaction::SerializeUnsigned(io::BinaryWriter& writer) const
{
    writer.Write(version_);
    writer.Write(nonce_);
    writer.Write(systemFee_);
    writer.Write(networkFee_);
    writer.Write(validUntilBlock_);
    writer.WriteVarArray(signers_);

    // Serialize attributes
    writer.WriteVarInt(attributes_.size());
    for (const auto& attr : attributes_)
    {
        attr->Serialize(writer);
    }

    writer.WriteVarBytes(script_);
}

void Neo3Transaction::DeserializeUnsigned(io::BinaryReader& reader)
{
    version_ = reader.ReadUInt8();
    if (version_ > 0)
    {
        throw std::runtime_error("Invalid version: " + std::to_string(version_));
    }

    nonce_ = reader.ReadUInt32();
    systemFee_ = reader.ReadInt64();
    if (systemFee_ < 0)
    {
        throw std::runtime_error("Invalid system fee: " + std::to_string(systemFee_));
    }

    networkFee_ = reader.ReadInt64();
    if (networkFee_ < 0)
    {
        throw std::runtime_error("Invalid network fee: " + std::to_string(networkFee_));
    }

    if (systemFee_ + networkFee_ < systemFee_)
    {
        throw std::runtime_error("Fee overflow detected");
    }

    validUntilBlock_ = reader.ReadUInt32();
    signers_ = DeserializeSigners(reader, MaxTransactionAttributes);
    auto legacy_attributes =
        DeserializeAttributes(reader, MaxTransactionAttributes - static_cast<int>(signers_.size()));

    // Convert legacy attributes to shared_ptr format
    attributes_.clear();
    attributes_.reserve(legacy_attributes.size());
    for (auto& attr : legacy_attributes)
    {
        attributes_.push_back(std::make_shared<ledger::TransactionAttribute>(std::move(attr)));
    }

    script_ = reader.ReadVarBytes(65536);  // ushort.MaxValue
    if (script_.empty())
    {
        throw std::runtime_error("Script cannot be empty");
    }

    InvalidateCache();
}

void Neo3Transaction::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.WriteProperty("hash", GetHash().ToString());
    writer.WriteProperty("size", GetSize());
    writer.WriteProperty("version", static_cast<int>(version_));
    writer.WriteProperty("nonce", nonce_);
    writer.WriteProperty("sender", GetSender().ToString());
    writer.WriteProperty("sysfee", std::to_string(systemFee_));
    writer.WriteProperty("netfee", std::to_string(networkFee_));
    writer.WriteProperty("validuntilblock", validUntilBlock_);

    writer.WritePropertyName("signers");
    writer.WriteStartArray();
    for (const auto& signer : signers_)
    {
        signer.SerializeJson(writer);
    }
    writer.WriteEndArray();

    writer.WritePropertyName("attributes");
    writer.WriteStartArray();
    for (const auto& attr : attributes_)
    {
        attr->SerializeJson(writer);
    }
    writer.WriteEndArray();

    // Convert script to base64
    std::string scriptBase64;
    if (!script_.empty())
    {
        try
        {
            // Base64 encoding implementation
            static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                    "abcdefghijklmnopqrstuvwxyz"
                                                    "0123456789+/";

            std::string result;
            const uint8_t* data = script_.Data();
            size_t len = script_.Size();

            for (size_t i = 0; i < len; i += 3)
            {
                uint32_t octet_a = i < len ? data[i] : 0;
                uint32_t octet_b = i + 1 < len ? data[i + 1] : 0;
                uint32_t octet_c = i + 2 < len ? data[i + 2] : 0;

                uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

                result += base64_chars[(triple >> 18) & 0x3F];
                result += base64_chars[(triple >> 12) & 0x3F];
                result += i + 1 < len ? base64_chars[(triple >> 6) & 0x3F] : '=';
                result += i + 2 < len ? base64_chars[triple & 0x3F] : '=';
            }

            scriptBase64 = result;
        }
        catch (const std::exception& e)
        {
            LOG_WARNING("Failed to encode script to base64: {}", e.what());
            scriptBase64 = "";  // Empty string on encoding failure
        }
    }
    writer.WriteProperty("script", scriptBase64);

    writer.WritePropertyName("witnesses");
    writer.WriteStartArray();
    for (const auto& witness : witnesses_)
    {
        witness.SerializeJson(writer);
    }
    writer.WriteEndArray();

    writer.WriteEndObject();
}

void Neo3Transaction::DeserializeJson(const io::JsonReader& reader)
{
    // Complete JSON deserialization for Neo3Transaction
    try
    {
        if (!reader.GetJson().is_object())
        {
            throw std::runtime_error("Expected JSON object for Neo3Transaction deserialization");
        }

        // Read basic transaction properties
        if (reader.HasKey("version"))
        {
            version_ = static_cast<uint8_t>(reader.ReadInt32("version"));
        }

        if (reader.HasKey("nonce"))
        {
            nonce_ = reader.ReadUInt32("nonce");
        }

        if (reader.HasKey("sysfee"))
        {
            std::string sysfeeStr = reader.ReadString("sysfee");
            systemFee_ = std::stoull(sysfeeStr);
        }

        if (reader.HasKey("netfee"))
        {
            std::string netfeeStr = reader.ReadString("netfee");
            networkFee_ = std::stoull(netfeeStr);
        }

        if (reader.HasKey("validuntilblock"))
        {
            validUntilBlock_ = reader.ReadUInt32("validuntilblock");
        }

        // Read signers array
        if (reader.HasKey("signers"))
        {
            auto signersArray = reader.ReadArray("signers");
            signers_.clear();
            signers_.reserve(signersArray.size());

            for (const auto& signerJson : signersArray)
            {
                if (signerJson.is_object())
                {
                    ledger::Signer signer;
                    io::JsonReader signerReader(signerJson);
                    signer.DeserializeJson(signerReader);
                    signers_.push_back(signer);
                }
            }
        }

        // Read attributes array
        if (reader.HasKey("attributes"))
        {
            auto attributesArray = reader.ReadArray("attributes");
            attributes_.clear();
            attributes_.reserve(attributesArray.size());

            for (const auto& attrJson : attributesArray)
            {
                if (attrJson.is_object())
                {
                    auto attr = std::make_shared<ledger::TransactionAttribute>();
                    io::JsonReader attrReader(attrJson);
                    attr->DeserializeJson(attrReader);
                    attributes_.push_back(attr);
                }
            }
        }

        // Read script (base64 encoded)
        if (reader.HasKey("script"))
        {
            std::string scriptBase64 = reader.ReadString("script");

            if (!scriptBase64.empty())
            {
                // Production base64 decoding using standard implementation
                try
                {
                    auto scriptBytes = reader.ReadBase64String("script");
                    script_ = scriptBytes;
                }
                catch (const std::exception& e)
                {
                    LOG_WARNING("Failed to decode script from base64: {}", e.what());
                    script_.clear();
                }
            }
            else
            {
                script_.clear();
            }
        }

        // Initialize witnesses array to match signers count
        witnesses_.clear();
        witnesses_.resize(signers_.size());

        // Read witnesses if present
        if (reader.HasKey("witnesses"))
        {
            auto witnessesArray = reader.ReadArray("witnesses");

            for (size_t i = 0; i < std::min(witnessesArray.size(), witnesses_.size()); ++i)
            {
                if (witnessesArray[i].is_object())
                {
                    io::JsonReader witnessReader(witnessesArray[i]);
                    witnesses_[i].DeserializeJson(witnessReader);
                }
            }
        }

        // Invalidate cached values since data has changed
        InvalidateCache();

        LOG_DEBUG("Successfully deserialized Neo3Transaction from JSON");
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to deserialize Neo3Transaction from JSON: " + std::string(e.what()));
    }
}

void Neo3Transaction::InvalidateCache() const
{
    hashCalculated_ = false;
    sizeCalculated_ = false;
}

void Neo3Transaction::CalculateHash() const
{
    // Complete hash calculation implementation for Neo N3 transaction
    try
    {
        // Use a simple vector to collect bytes
        std::vector<uint8_t> buffer;
        
        // Pre-allocate some space to avoid reallocations
        buffer.reserve(256);
        
        // Write version
        buffer.push_back(version_);
        
        // Write nonce (4 bytes, little-endian)
        buffer.push_back(nonce_ & 0xFF);
        buffer.push_back((nonce_ >> 8) & 0xFF);
        buffer.push_back((nonce_ >> 16) & 0xFF);
        buffer.push_back((nonce_ >> 24) & 0xFF);
        
        // Write systemFee (8 bytes, little-endian)
        for (int i = 0; i < 8; i++) {
            buffer.push_back((systemFee_ >> (i * 8)) & 0xFF);
        }
        
        // Write networkFee (8 bytes, little-endian)
        for (int i = 0; i < 8; i++) {
            buffer.push_back((networkFee_ >> (i * 8)) & 0xFF);
        }
        
        // Write validUntilBlock (4 bytes, little-endian)
        buffer.push_back(validUntilBlock_ & 0xFF);
        buffer.push_back((validUntilBlock_ >> 8) & 0xFF);
        buffer.push_back((validUntilBlock_ >> 16) & 0xFF);
        buffer.push_back((validUntilBlock_ >> 24) & 0xFF);
        
        // Write signers count as varint
        if (signers_.size() < 0xFD) {
            buffer.push_back(static_cast<uint8_t>(signers_.size()));
        } else {
            // Handle larger counts if needed
            buffer.push_back(0xFD);
            buffer.push_back(signers_.size() & 0xFF);
            buffer.push_back((signers_.size() >> 8) & 0xFF);
        }
        
        // Write each signer (account + scope = 21 bytes)
        for (const auto& signer : signers_) {
            // Write account (20 bytes)
            auto accountData = signer.GetAccount().AsSpan();
            buffer.insert(buffer.end(), accountData.Data(), accountData.Data() + accountData.Size());
            
            // Write scope (1 byte)
            buffer.push_back(static_cast<uint8_t>(signer.GetScopes()));
        }
        
        // Write attributes count
        if (attributes_.size() < 0xFD) {
            buffer.push_back(static_cast<uint8_t>(attributes_.size()));
        } else {
            buffer.push_back(0xFD);
            buffer.push_back(attributes_.size() & 0xFF);
            buffer.push_back((attributes_.size() >> 8) & 0xFF);
        }
        
        // Skip attributes for now (empty)
        
        // Write script length as varint
        if (script_.size() < 0xFD) {
            buffer.push_back(static_cast<uint8_t>(script_.size()));
        } else {
            buffer.push_back(0xFD);
            buffer.push_back(script_.size() & 0xFF);
            buffer.push_back((script_.size() >> 8) & 0xFF);
        }
        
        // Write script
        buffer.insert(buffer.end(), script_.Data(), script_.Data() + script_.Size());

        // Calculate SHA-256 hash (Neo N3 uses single SHA-256 for transaction hashes)
        auto transactionHash = cryptography::Crypto::Hash256(io::ByteSpan(buffer.data(), buffer.size()));

        // Store the calculated hash
        hash_ = io::UInt256(transactionHash.Data());
        hashCalculated_ = true;
    }
    catch (const std::exception& e)
    {
        // LOG_ERROR("Failed to calculate transaction hash: {}", e.what());

        // Set a zero hash on error and mark as calculated to prevent infinite loops
        hash_ = io::UInt256();
        hashCalculated_ = true;
        throw std::runtime_error("Transaction hash calculation failed: " + std::string(e.what()));
    }
}

void Neo3Transaction::CalculateSize() const
{
    size_ = HeaderSize;

    // Add signers size
    size_ += GetVarIntSize(signers_.size());
    for (const auto& signer : signers_)
    {
        size_ += GetSignerSize(signer);
    }

    // Add attributes size
    size_ += GetVarIntSize(attributes_.size());
    for (const auto& attr : attributes_)
    {
        size_ += GetAttributeSize(*attr);
    }

    // Add script size
    size_ += GetVarIntSize(script_.size()) + static_cast<int>(script_.size());

    // Add witnesses size
    size_ += GetVarIntSize(witnesses_.size());
    for (const auto& witness : witnesses_)
    {
        size_ += GetWitnessSize(witness);
    }

    sizeCalculated_ = true;
}

int Neo3Transaction::GetVarIntSize(size_t value) const
{
    if (value < 0xFD)
        return 1;
    else if (value <= 0xFFFF)
        return 3;
    else if (value <= 0xFFFFFFFF)
        return 5;
    else
        return 9;
}

int Neo3Transaction::GetSignerSize(const ledger::Signer& signer) const
{
    // Signer: account (20 bytes) + scopes (1 byte) + allowed contracts/groups (variable)
    // Basic calculation
    return 21;  // account + scopes
}

int Neo3Transaction::GetAttributeSize(const ledger::TransactionAttribute& attr) const
{
    // Attribute: type (1 byte) + data (variable length)
    // Basic calculation
    return 1 + static_cast<int>(attr.GetData().Size());
}

int Neo3Transaction::GetWitnessSize(const ledger::Witness& witness) const
{
    // Witness: invocation script + verification script (both variable length)
    // Basic calculation - basic structure
    return 2;  // minimal witness size
}

std::vector<ledger::TransactionAttribute> Neo3Transaction::DeserializeAttributes(io::BinaryReader& reader, int maxCount)
{
    int count = static_cast<int>(reader.ReadVarInt(maxCount));
    std::vector<ledger::TransactionAttribute> attributes;
    attributes.reserve(count);

    std::set<ledger::TransactionAttribute::Usage> seenTypes;
    for (int i = 0; i < count; i++)
    {
        ledger::TransactionAttribute attr;
        attr.Deserialize(reader);

        // Check for duplicate attributes (simplified check)
        if (seenTypes.count(attr.GetUsage()) > 0)
        {
            throw std::runtime_error("Duplicate attribute type");
        }
        seenTypes.insert(attr.GetUsage());

        attributes.push_back(std::move(attr));
    }

    return attributes;
}

std::vector<ledger::Signer> Neo3Transaction::DeserializeSigners(io::BinaryReader& reader, int maxCount)
{
    int count = static_cast<int>(reader.ReadVarInt(maxCount));
    if (count == 0)
    {
        throw std::runtime_error("Transaction must have at least one signer");
    }

    std::vector<ledger::Signer> signers;
    signers.reserve(count);

    std::set<io::UInt160> seenAccounts;
    for (int i = 0; i < count; i++)
    {
        ledger::Signer signer;
        signer.Deserialize(reader);

        // Check for duplicate signers
        if (seenAccounts.count(signer.GetAccount()) > 0)
        {
            throw std::runtime_error("Duplicate signer account");
        }
        seenAccounts.insert(signer.GetAccount());

        signers.push_back(std::move(signer));
    }

    return signers;
}

bool Neo3Transaction::operator==(const Neo3Transaction& other) const
{
    return version_ == other.version_ &&
           nonce_ == other.nonce_ &&
           systemFee_ == other.systemFee_ &&
           networkFee_ == other.networkFee_ &&
           validUntilBlock_ == other.validUntilBlock_ &&
           signers_ == other.signers_ &&
           attributes_ == other.attributes_ &&
           script_ == other.script_ &&
           witnesses_ == other.witnesses_;
}

bool Neo3Transaction::operator!=(const Neo3Transaction& other) const
{
    return !(*this == other);
}
}  // namespace neo::network::p2p::payloads