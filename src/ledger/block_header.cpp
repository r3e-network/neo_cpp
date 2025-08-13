/**
 * @file block_header.cpp
 * @brief Block structure and validation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/hash.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/block.h>
#include <neo/ledger/block_header.h>

#include <nlohmann/json.hpp>
#include <sstream>

namespace neo::ledger
{
BlockHeader::BlockHeader() : version_(0), timestamp_(0), nonce_(0), index_(0), primaryIndex_(0) {}

BlockHeader::BlockHeader(const Block& block)
    : version_(block.GetVersion()),
      prevHash_(block.GetPreviousHash()),
      merkleRoot_(block.GetMerkleRoot()),
      timestamp_(block.GetTimestamp()),
      nonce_(0),  // Block doesn't have nonce
      index_(block.GetIndex()),
      primaryIndex_(static_cast<uint8_t>(block.GetPrimaryIndex())),
      nextConsensus_(block.GetNextConsensus()),
      witness_()  // Block doesn't have witness
{
}

uint32_t BlockHeader::GetVersion() const { return version_; }

void BlockHeader::SetVersion(uint32_t version) { version_ = version; }

const io::UInt256& BlockHeader::GetPrevHash() const { return prevHash_; }

void BlockHeader::SetPrevHash(const io::UInt256& prevHash) { prevHash_ = prevHash; }

const io::UInt256& BlockHeader::GetMerkleRoot() const { return merkleRoot_; }

void BlockHeader::SetMerkleRoot(const io::UInt256& merkleRoot) { merkleRoot_ = merkleRoot; }

uint64_t BlockHeader::GetTimestamp() const { return timestamp_; }

void BlockHeader::SetTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }

uint64_t BlockHeader::GetNonce() const { return nonce_; }

void BlockHeader::SetNonce(uint64_t nonce) { nonce_ = nonce; }

uint32_t BlockHeader::GetIndex() const { return index_; }

void BlockHeader::SetIndex(uint32_t index) { index_ = index; }

uint8_t BlockHeader::GetPrimaryIndex() const { return primaryIndex_; }

void BlockHeader::SetPrimaryIndex(uint8_t primaryIndex) { primaryIndex_ = primaryIndex; }

const io::UInt160& BlockHeader::GetNextConsensus() const { return nextConsensus_; }

void BlockHeader::SetNextConsensus(const io::UInt160& nextConsensus) { nextConsensus_ = nextConsensus; }

const Witness& BlockHeader::GetWitness() const { return witness_; }

void BlockHeader::SetWitness(const Witness& witness) { witness_ = witness; }

io::UInt256 BlockHeader::GetHash() const
{
    std::ostringstream stream;
    io::BinaryWriter writer(stream);

    // Serialize the unsigned header data (Neo N3): version, prevHash, merkleRoot, timestamp, index,
    // primaryIndex, nextConsensus. Nonce is not part of N3 header hash.
    writer.Write(version_);
    writer.Write(prevHash_);
    writer.Write(merkleRoot_);
    writer.Write(timestamp_);
    writer.Write(index_);
    writer.Write(primaryIndex_);
    writer.Write(nextConsensus_);

    std::string data = stream.str();
    return cryptography::Hash::Hash256(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

int BlockHeader::GetSize() const
{
    // Exactly match C# Header.Size calculation
    return sizeof(uint32_t) +         // Version
           32 +                       // PrevHash (UInt256.Length)
           32 +                       // MerkleRoot (UInt256.Length)
           sizeof(uint64_t) +         // Timestamp
           sizeof(uint32_t) +         // Index
           sizeof(uint8_t) +          // PrimaryIndex
           20 +                       // NextConsensus (UInt160.Length)
           (1 + witness_.GetSize());  // Witness (1 byte for count + witness size)
}

bool BlockHeader::Verify() const
{
    // Verify the block header
    if (version_ != 0) return false;

    // Genesis block has no previous hash
    if (index_ == 0)
    {
        if (prevHash_ != io::UInt256::Zero()) return false;
    }
    else
    {
        if (prevHash_ == io::UInt256::Zero()) return false;
    }

    // Verify the witness
    return VerifyWitness();
}

bool BlockHeader::VerifyWitness() const
{
    // Implement witness verification for block headers
    // Block headers should be signed by consensus nodes
    try
    {
        if (witness_.GetVerificationScript().Size() == 0) return false;

        // For blocks, the witness should be a multi-signature contract
        // signed by the consensus nodes (validators)
        auto verificationScript = witness_.GetVerificationScript();

        // Check if it's a multi-signature contract
        if (!IsMultiSignatureContract(verificationScript)) return false;

        // Verify the multi-signature
        return VerifyMultiSignatureWitness(witness_);
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
    catch (const std::invalid_argument&)
    {
        return false;
    }
}

void BlockHeader::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(version_);
    writer.Write(prevHash_);
    writer.Write(merkleRoot_);
    writer.Write(timestamp_);
    writer.Write(index_);
    writer.Write(nextConsensus_);
    witness_.Serialize(writer);
}

void BlockHeader::Deserialize(io::BinaryReader& reader)
{
    version_ = reader.ReadUInt32();
    prevHash_ = reader.ReadUInt256();
    merkleRoot_ = reader.ReadUInt256();
    timestamp_ = reader.ReadUInt64();
    index_ = reader.ReadUInt32();
    nextConsensus_ = reader.ReadUInt160();
    witness_.Deserialize(reader);
}

void BlockHeader::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("version", version_);
    writer.Write("previousblockhash", prevHash_.ToHexString());
    writer.Write("merkleroot", merkleRoot_.ToHexString());
    writer.Write("time", timestamp_);
    writer.Write("index", index_);
    writer.Write("nextconsensus", nextConsensus_.ToHexString());

    // Write witness as nested object using proper JsonWriter interface
    writer.WritePropertyName("witness");
    writer.WriteStartObject();
    writer.Write("invocation", witness_.GetInvocationScript().ToHexString());
    writer.Write("verification", witness_.GetVerificationScript().ToHexString());
    writer.WriteEndObject();

    writer.Write("hash", GetHash().ToHexString());
}

void BlockHeader::DeserializeJson(const io::JsonReader& reader)
{
    auto json = reader.GetJson();
    version_ = json["version"];
    prevHash_ = io::UInt256::Parse(json["previousblockhash"].get<std::string>());
    merkleRoot_ = io::UInt256::Parse(json["merkleroot"].get<std::string>());
    timestamp_ = json["time"];
    index_ = json["index"];
    nextConsensus_ = io::UInt160::Parse(json["nextconsensus"].get<std::string>());

    io::ByteVector invocationScript = io::ByteVector::Parse(json["witness"]["invocation"].get<std::string>());
    io::ByteVector verificationScript = io::ByteVector::Parse(json["witness"]["verification"].get<std::string>());
    witness_ = Witness(invocationScript, verificationScript);
}

bool BlockHeader::operator==(const BlockHeader& other) const
{
    return version_ == other.version_ && prevHash_ == other.prevHash_ && merkleRoot_ == other.merkleRoot_ &&
           timestamp_ == other.timestamp_ && index_ == other.index_ && nextConsensus_ == other.nextConsensus_ &&
           witness_ == other.witness_;
}

bool BlockHeader::operator!=(const BlockHeader& other) const { return !(*this == other); }

bool BlockHeader::IsMultiSignatureContract(const io::ByteVector& script) const
{
    // Check if script is a multi-signature contract (same logic as Transaction)
    if (script.Size() < 37)  // Minimum size for 1-of-1 multisig
        return false;

    // Check for CHECKMULTISIG opcode at the end
    if (script[script.Size() - 1] != 0xC1)  // CHECKMULTISIG
        return false;

    // Extract n (number of public keys) from second-to-last byte
    uint8_t nByte = script[script.Size() - 2];
    if (nByte < 0x51 || nByte > 0x60)  // PUSH1 to PUSH16
        return false;
    int n = nByte - 0x50;

    // Extract m (required signatures) from first byte
    uint8_t mByte = script[0];
    if (mByte < 0x51 || mByte > 0x60)  // PUSH1 to PUSH16
        return false;
    int m = mByte - 0x50;

    // Validate m <= n
    if (m > n || m < 1 || n < 1 || n > 16) return false;

    return true;
}

bool BlockHeader::VerifyMultiSignatureWitness(const Witness& witness) const
{
    try
    {
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
            if (offset >= verificationScript.Size() || verificationScript[offset] != 0x21) return false;

            auto pubkeyBytes = io::ByteSpan(verificationScript.Data() + offset + 1, 33);
            publicKeys.push_back(cryptography::ecc::ECPoint::FromBytes(pubkeyBytes, "secp256r1"));
            offset += 34;  // 1 byte opcode + 33 bytes pubkey
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

        // Verify signatures
        auto signData = GetSignData();
        int validSignatures = 0;
        int pubKeyIndex = 0;

        for (const auto& signature : signatures)
        {
            if (validSignatures >= m) break;

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

            if (!signatureValid) return false;

            // Check if we can still reach m signatures with remaining public keys
            if (m - validSignatures > n - pubKeyIndex) return false;
        }

        return validSignatures >= m;
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
    catch (const std::invalid_argument&)
    {
        return false;
    }
    catch (const std::out_of_range&)
    {
        return false;
    }
}

io::ByteVector BlockHeader::GetSignData() const
{
    // Get the data that should be signed for this block header
    std::ostringstream stream;
    io::BinaryWriter writer(stream);

    // Serialize block header data without witness (unsigned data)
    writer.Write(version_);
    writer.Write(prevHash_);
    writer.Write(merkleRoot_);
    writer.Write(timestamp_);
    writer.Write(nonce_);
    writer.Write(index_);
    writer.Write(primaryIndex_);
    writer.Write(nextConsensus_);

    // Add network magic
    uint32_t networkMagic = 860833102;  // Neo N3 MainNet magic
    writer.Write(networkMagic);

    std::string data = stream.str();
    return io::ByteVector(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}
}  // namespace neo::ledger
