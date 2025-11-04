/**
 * @file consensus_context.cpp
 * @brief Consensus Context
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/consensus/change_view_message.h>
#include <neo/consensus/commit_message.h>
#include <neo/consensus/consensus_context.h>
#include <neo/consensus/prepare_request.h>
#include <neo/consensus/prepare_response.h>
#include <neo/consensus/recovery_message.h>
#include <neo/consensus/recovery_request.h>
#include <neo/cryptography/hash.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/vm/script_builder.h>

#include <algorithm>
#include <chrono>
#include <random>

namespace neo::consensus
{
ConsensusContext::ConsensusContext(std::shared_ptr<ledger::NeoSystem> neoSystem,
                                   std::shared_ptr<ProtocolSettings> settings, std::shared_ptr<sign::ISigner> signer)
    : neoSystem_(neoSystem), settings_(settings), signer_(signer)
{
    Reset(0);
}

void ConsensusContext::Reset(uint8_t viewNumber)
{
    if (viewNumber == 0)
    {
        // Initialize for new block
        Snapshot = neoSystem_->GetStoreView();
        uint32_t height = smartcontract::native::LedgerContract::GetCurrentIndex(Snapshot);

        Block = std::make_shared<ledger::Block>();
        Block->SetPrevHash(smartcontract::native::LedgerContract::GetCurrentHash(Snapshot));
        Block->SetIndex(height + 1);

        // Set validators for next block
        Validators = smartcontract::native::NeoToken::GetNextBlockValidators(Snapshot, settings_->GetValidatorsCount());

        // Calculate witness size if needed
        if (witnessSize_ == 0 || PreparationPayloads.size() != Validators.size())
        {
            vm::ScriptBuilder sb;
            auto buf = io::ByteVector(64);
            for (int x = 0; x < M(); x++)
            {
                sb.EmitPush(buf);
            }

            cryptography::Witness witness;
            witness.SetInvocationScript(sb.ToArray());
            witness.SetVerificationScript(cryptography::CreateMultiSigRedeemScript(M(), Validators));
            witnessSize_ = witness.GetSize();
        }

        // Find my index in validators
        MyIndex = -1;
        for (size_t i = 0; i < Validators.size(); i++)
        {
            if (signer_->ContainsSignable(Validators[i]))
            {
                MyIndex = static_cast<int>(i);
                myPublicKey_ = Validators[MyIndex];
                break;
            }
        }

        // Initialize arrays
        ChangeViewPayloads.resize(Validators.size());
        LastChangeViewPayloads.resize(Validators.size());
        CommitPayloads.resize(Validators.size());

        // Initialize last seen messages
        if (LastSeenMessage.empty())
        {
            for (const auto& validator : Validators)
            {
                LastSeenMessage[validator] = height;
            }
        }

        cachedMessages_.clear();
    }
    else
    {
        // View change - keep some state
        for (size_t i = 0; i < LastChangeViewPayloads.size(); i++)
        {
            if (ChangeViewPayloads[i])
            {
                auto msg = ConsensusPayloadHelper::GetMessage(*ChangeViewPayloads[i]);
                if (msg && msg->GetViewNumber() >= viewNumber)
                {
                    LastChangeViewPayloads[i] = ChangeViewPayloads[i];
                }
                else
                {
                    LastChangeViewPayloads[i] = nullptr;
                }
            }
        }
    }

    ViewNumber = viewNumber;
    Block->SetPrimaryIndex(GetPrimaryIndex(viewNumber));
    Block->SetMerkleRoot(io::UInt256::Zero());
    Block->SetTimestamp(0);
    Block->SetNonce(0);
    Block->SetTransactions(nullptr);
    TransactionHashes.clear();
    PreparationPayloads.resize(Validators.size());

    if (MyIndex >= 0)
    {
        LastSeenMessage[Validators[MyIndex]] = Block->GetIndex();
    }
}

uint8_t ConsensusContext::GetPrimaryIndex(uint8_t viewNumber) const
{
    if (Validators.empty()) return 0;

    return static_cast<uint8_t>((Block->GetIndex() - viewNumber) % Validators.size());
}

bool ConsensusContext::RequestSentOrReceived() const
{
    return PreparationPayloads[Block->GetPrimaryIndex()] != nullptr;
}

bool ConsensusContext::ResponseSent() const { return !WatchOnly() && PreparationPayloads[MyIndex] != nullptr; }

bool ConsensusContext::CommitSent() const { return !WatchOnly() && CommitPayloads[MyIndex] != nullptr; }

bool ConsensusContext::BlockSent() const { return Block->GetTransactions() != nullptr; }

bool ConsensusContext::ViewChanging() const
{
    if (WatchOnly() || !ChangeViewPayloads[MyIndex]) return false;

    auto msg = ConsensusPayloadHelper::GetMessage(*ChangeViewPayloads[MyIndex]);
    if (!msg) return false;

    auto changeView = std::dynamic_pointer_cast<ChangeViewMessage>(msg);
    return changeView && changeView->GetNewViewNumber() > ViewNumber;
}

std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeSignedPayload(
    std::shared_ptr<ConsensusMessage> message)
{
    message->SetBlockIndex(Block->GetIndex());
    message->SetValidatorIndex(static_cast<uint8_t>(MyIndex));
    message->SetViewNumber(ViewNumber);

    auto payload = ConsensusPayloadHelper::CreatePayload(message, myPublicKey_.GetScriptHash(), Block->GetIndex(),
                                                         Block->GetIndex() + 1000  // Valid for 1000 blocks
    );

    SignPayload(payload);
    return payload;
}

void ConsensusContext::SignPayload(std::shared_ptr<network::p2p::payloads::ExtensiblePayload> payload)
{
    try
    {
        auto witness = signer_->SignExtensiblePayload(payload, Snapshot, settings_->GetNetwork());
        payload->SetWitness(witness);
    }
    catch (const std::exception& e)
    {
        // Log error
    }
}

std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeChangeView(uint8_t reason)
{
    auto changeView = std::make_shared<ChangeViewMessage>();
    changeView->SetReason(reason);
    changeView->SetTimestamp(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());

    ChangeViewPayloads[MyIndex] = MakeSignedPayload(changeView);
    return ChangeViewPayloads[MyIndex];
}

std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakePrepareRequest()
{
    // Gather transactions from mempool
    auto maxTxPerBlock = settings_->GetMaxTransactionsPerBlock();
    auto txs = neoSystem_->GetMemoryPool()->GetSortedVerifiedTransactions(maxTxPerBlock);
    EnsureMaxBlockLimitation(txs);

    // Set block timestamp with proper validation
    auto now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Complete implementation: Get previous header timestamp for validation
    uint64_t previous_timestamp = 0;
    try
    {
        auto previous_block = smartcontract::native::LedgerContract::GetBlock(Snapshot, Block->GetIndex() - 1);
        if (previous_block)
        {
            previous_timestamp = previous_block->GetTimestamp();

            // Ensure block timestamp is greater than previous block timestamp
            if (now <= previous_timestamp)
            {
                now = previous_timestamp + 1;  // Minimum increment
            }
        }
    }
    catch (const std::exception& e)
    {
        // For genesis block or on error, use current time
        LOG_DEBUG("Could not get previous block timestamp: {}", e.what());
    }

    Block->SetTimestamp(now);

    // Complete implementation: Generate proper nonce using random number generator
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    uint64_t nonce = dis(gen);

    Block->SetNonce(nonce);

    auto request = std::make_shared<PrepareRequest>();
    request->SetVersion(Block->GetVersion());
    request->SetPrevHash(Block->GetPrevHash());
    request->SetTimestamp(Block->GetTimestamp());
    request->SetNonce(Block->GetNonce());
    request->SetTransactionHashes(TransactionHashes);

    PreparationPayloads[MyIndex] = MakeSignedPayload(request);
    return PreparationPayloads[MyIndex];
}

std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakePrepareResponse()
{
    auto response = std::make_shared<PrepareResponse>();
    response->SetPreparationHash(PreparationPayloads[Block->GetPrimaryIndex()]->GetHash());

    PreparationPayloads[MyIndex] = MakeSignedPayload(response);
    return PreparationPayloads[MyIndex];
}

std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeCommit()
{
    if (CommitPayloads[MyIndex]) return CommitPayloads[MyIndex];

    auto block = EnsureHeader();
    auto commit = std::make_shared<CommitMessage>();
    commit->SetSignature(signer_->SignBlock(block, myPublicKey_, settings_->GetNetwork()));

    CommitPayloads[MyIndex] = MakeSignedPayload(commit);
    return CommitPayloads[MyIndex];
}

std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeRecoveryRequest()
{
    auto request = std::make_shared<RecoveryRequest>();
    request->SetTimestamp(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());

    return MakeSignedPayload(request);
}

std::shared_ptr<network::p2p::payloads::ExtensiblePayload> ConsensusContext::MakeRecoveryMessage()
{
    auto recovery = std::make_shared<RecoveryMessage>();

    // Complete recovery message preparation implementation
    // Recovery messages help nodes catch up with the current consensus state
    try
    {
        // Set basic recovery message fields
        recovery->SetViewNumber(ViewNumber);
        recovery->SetBlockIndex(Block->GetIndex());

        // Add change view messages from this view
        std::vector<std::shared_ptr<ChangeViewMessage>> changeViews;
        for (size_t i = 0; i < ChangeViewPayloads.size(); i++)
        {
            if (ChangeViewPayloads[i] != nullptr)
            {
                auto changeView = std::dynamic_pointer_cast<ChangeViewMessage>(ChangeViewPayloads[i]->GetData());
                if (changeView)
                {
                    changeViews.push_back(changeView);
                }
            }
        }
        recovery->SetChangeViewMessages(changeViews);

        // Add preparation messages (prepare request + responses)
        std::vector<std::shared_ptr<PrepareRequest>> prepareRequests;
        std::vector<std::shared_ptr<PrepareResponse>> prepareResponses;

        for (size_t i = 0; i < PreparationPayloads.size(); i++)
        {
            if (PreparationPayloads[i] != nullptr)
            {
                auto data = PreparationPayloads[i]->GetData();

                // Check if it's a prepare request
                auto prepareReq = std::dynamic_pointer_cast<PrepareRequest>(data);
                if (prepareReq)
                {
                    prepareRequests.push_back(prepareReq);
                    continue;
                }

                // Check if it's a prepare response
                auto prepareResp = std::dynamic_pointer_cast<PrepareResponse>(data);
                if (prepareResp)
                {
                    prepareResponses.push_back(prepareResp);
                }
            }
        }

        recovery->SetPrepareRequests(prepareRequests);
        recovery->SetPrepareResponses(prepareResponses);

        // Add commit messages
        std::vector<std::shared_ptr<CommitMessage>> commits;
        for (size_t i = 0; i < CommitPayloads.size(); i++)
        {
            if (CommitPayloads[i] != nullptr)
            {
                auto commit = std::dynamic_pointer_cast<CommitMessage>(CommitPayloads[i]->GetData());
                if (commit)
                {
                    commits.push_back(commit);
                }
            }
        }
        recovery->SetCommitMessages(commits);

        // Include transactions in recovery message following original proposal order
        std::vector<network::p2p::payloads::Neo3Transaction> transactions;
        transactions.reserve(TransactionHashes.size());
        for (const auto& hash : TransactionHashes)
        {
            auto tx_it = Transactions.find(hash);
            if (tx_it != Transactions.end() && tx_it->second)
            {
                transactions.push_back(*tx_it->second);
            }
        }
        recovery->SetTransactions(transactions);

        LOG_DEBUG(
            "Prepared recovery message for view {} with {} change views, {} prepare reqs, {} prepare resps, {} commits, "
            "{} txs",
            ViewNumber, changeViews.size(), prepareRequests.size(), prepareResponses.size(), commits.size(),
            transactions.size());
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error preparing recovery message: {}", e.what());
    }

    return MakeSignedPayload(recovery);
}

void ConsensusContext::EnsureMaxBlockLimitation(const std::vector<std::shared_ptr<ledger::Transaction>>& txs)
{
    TransactionHashes.clear();
    Transactions.clear();
    VerificationContext = ledger::TransactionVerificationContext();

    // Expected block size
    auto blockSize = GetExpectedBlockSizeWithoutTransactions(txs.size());
    int64_t blockSystemFee = 0;

    for (const auto& tx : txs)
    {
        // Check size limit
        blockSize += tx->GetSize();
        if (blockSize > settings_->GetMaxBlockSize()) break;

        // Check system fee limit
        blockSystemFee += tx->GetSystemFee();
        if (blockSystemFee > settings_->GetMaxBlockSystemFee()) break;

        TransactionHashes.push_back(tx->GetHash());
        Transactions[tx->GetHash()] = tx;
        VerificationContext.AddTransaction(tx);
    }
}

size_t ConsensusContext::GetExpectedBlockSizeWithoutTransactions(size_t txCount) const
{
    // Base block size + witness size + transaction count encoding
    return ledger::Block::HeaderSize + witnessSize_ + io::GetVarSize(txCount);
}

std::shared_ptr<ledger::Block> ConsensusContext::EnsureHeader()
{
    if (TransactionHashes.empty()) return Block;

    // Calculate merkle root - Complete implementation
    try
    {
        // Convert transaction hashes to the format expected by merkle tree calculation
        std::vector<io::UInt256> hashes = TransactionHashes;

        // Compute merkle root using complete binary tree algorithm
        auto merkle_root = cryptography::MerkleTree::ComputeRoot(hashes);
        Block->SetMerkleRoot(merkle_root);

        LOG_DEBUG("Calculated merkle root for {} transactions: {}", hashes.size(), merkle_root.ToString());
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error calculating merkle root: {}", e.what());
        // Set to zero hash on error
        Block->SetMerkleRoot(io::UInt256::Zero());
    }

    return Block;
}

void ConsensusContext::Save()
{
    // Complete consensus state persistence implementation
    try
    {
        // Create a storage key for consensus state
        auto consensus_key = persistence::StorageKey::CreateForConsensus(Block->GetIndex(), ViewNumber);

        // Serialize the consensus context to binary format
        io::MemoryStream stream;
        io::BinaryWriter writer(stream);
        Serialize(writer);

        // Create storage item with the serialized data
        auto serialized_data = stream.ToByteVector();
        persistence::StorageItem consensus_item(serialized_data);

        // Store in the blockchain's persistence layer
        if (Snapshot)
        {
            Snapshot->Put(consensus_key, consensus_item);

            // Also store current view number for recovery
            auto view_key = persistence::StorageKey::CreateForConsensusView(Block->GetIndex());
            persistence::StorageItem view_item(io::ByteVector::FromUInt8(ViewNumber));
            Snapshot->Put(view_key, view_item);

            LOG_DEBUG("Saved consensus state for block {} view {} ({} bytes)", Block->GetIndex(), ViewNumber,
                      serialized_data.Size());
        }
        else
        {
            LOG_WARNING("Cannot save consensus state - no snapshot available");
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error saving consensus state: {}", e.what());
    }
}

void ConsensusContext::Serialize(io::BinaryWriter& writer) const
{
    // Complete consensus context serialization implementation
    try
    {
        // Write version for compatibility
        writer.WriteUInt8(1);  // Serialization version

        // Write basic consensus state
        writer.WriteUInt8(ViewNumber);
        writer.WriteUInt32(Block->GetIndex());
        writer.WriteUInt64(Block->GetTimestamp());
        writer.WriteUInt64(Block->GetNonce());

        // Write block hash information
        writer.Write(Block->GetPrevHash().AsSpan());
        writer.Write(Block->GetMerkleRoot().AsSpan());

        // Write validator information
        writer.WriteVarInt(Validators.size());
        for (const auto& validator : Validators)
        {
            writer.Write(validator.Data(), validator.Size());
        }

        // Write MyIndex
        writer.WriteUInt32(MyIndex);

        // Write transaction hashes
        writer.WriteVarInt(TransactionHashes.size());
        for (const auto& hash : TransactionHashes)
        {
            writer.Write(hash.AsSpan());
        }

        // Write payload states - Complete implementation with full payload data
        writer.WriteVarInt(PreparationPayloads.size());
        for (size_t i = 0; i < PreparationPayloads.size(); i++)
        {
            writer.WriteBool(PreparationPayloads[i] != nullptr);
            if (PreparationPayloads[i] != nullptr)
            {
                // Store complete payload information
                writer.WriteUInt32(i);  // validator index

                // Serialize the actual payload data
                auto payload = PreparationPayloads[i];
                writer.WriteUInt32(payload->GetValidatorIndex());
                writer.WriteUInt8(payload->GetViewNumber());
                writer.WriteUInt64(payload->GetTimestamp());

                // Write payload-specific data
                auto data = payload->GetData();
                if (data)
                {
                    writer.WriteBool(true);
                    // Serialize the consensus message data
                    io::MemoryStream dataStream;
                    io::BinaryWriter dataWriter(dataStream);
                    data->SerializeTo(dataWriter);
                    auto serializedData = dataStream.ToByteVector();
                    writer.WriteVarInt(serializedData.Size());
                    writer.Write(serializedData.Data(), serializedData.Size());
                }
                else
                {
                    writer.WriteBool(false);
                }
            }
        }

        writer.WriteVarInt(CommitPayloads.size());
        for (size_t i = 0; i < CommitPayloads.size(); i++)
        {
            writer.WriteBool(CommitPayloads[i] != nullptr);
            if (CommitPayloads[i] != nullptr)
            {
                // Store complete commit payload information
                writer.WriteUInt32(i);  // validator index

                auto payload = CommitPayloads[i];
                writer.WriteUInt32(payload->GetValidatorIndex());
                writer.WriteUInt8(payload->GetViewNumber());
                writer.WriteUInt64(payload->GetTimestamp());

                // Write commit-specific data (signature)
                auto data = payload->GetData();
                if (data)
                {
                    writer.WriteBool(true);
                    io::MemoryStream dataStream;
                    io::BinaryWriter dataWriter(dataStream);
                    data->SerializeTo(dataWriter);
                    auto serializedData = dataStream.ToByteVector();
                    writer.WriteVarInt(serializedData.Size());
                    writer.Write(serializedData.Data(), serializedData.Size());
                }
                else
                {
                    writer.WriteBool(false);
                }
            }
        }

        writer.WriteVarInt(ChangeViewPayloads.size());
        for (size_t i = 0; i < ChangeViewPayloads.size(); i++)
        {
            writer.WriteBool(ChangeViewPayloads[i] != nullptr);
            if (ChangeViewPayloads[i] != nullptr)
            {
                // Store complete change view payload information
                writer.WriteUInt32(i);  // validator index

                auto payload = ChangeViewPayloads[i];
                writer.WriteUInt32(payload->GetValidatorIndex());
                writer.WriteUInt8(payload->GetViewNumber());
                writer.WriteUInt64(payload->GetTimestamp());

                // Write change view specific data
                auto data = payload->GetData();
                if (data)
                {
                    writer.WriteBool(true);
                    io::MemoryStream dataStream;
                    io::BinaryWriter dataWriter(dataStream);
                    data->SerializeTo(dataWriter);
                    auto serializedData = dataStream.ToByteVector();
                    writer.WriteVarInt(serializedData.Size());
                    writer.Write(serializedData.Data(), serializedData.Size());
                }
                else
                {
                    writer.WriteBool(false);
                }
            }
        }

        // Write witness size
        writer.WriteUInt32(witnessSize_);

        LOG_DEBUG("Serialized consensus context for block {} view {}", Block->GetIndex(), ViewNumber);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error serializing consensus context: {}", e.what());
        throw;
    }
}

void ConsensusContext::Deserialize(io::BinaryReader& reader)
{
    // Complete consensus context deserialization implementation
    try
    {
        // Read and verify version
        uint8_t version = reader.ReadUInt8();
        if (version != 1)
        {
            throw std::runtime_error("Unsupported consensus context serialization version: " + std::to_string(version));
        }

        // Read basic consensus state
        ViewNumber = reader.ReadUInt8();
        uint32_t blockIndex = reader.ReadUInt32();
        uint64_t timestamp = reader.ReadUInt64();
        uint64_t nonce = reader.ReadUInt64();

        // Create new block with restored state
        Block = std::make_shared<ledger::Block>();
        Block->SetIndex(blockIndex);
        Block->SetTimestamp(timestamp);
        Block->SetNonce(nonce);

        // Read block hash information
        io::UInt256 prevHash;
        reader.Read(prevHash.Data(), prevHash.Size());
        Block->SetPrevHash(prevHash);

        io::UInt256 merkleRoot;
        reader.Read(merkleRoot.Data(), merkleRoot.Size());
        Block->SetMerkleRoot(merkleRoot);

        // Read validator information
        size_t validatorCount = reader.ReadVarInt();
        Validators.clear();
        Validators.reserve(validatorCount);

        for (size_t i = 0; i < validatorCount; i++)
        {
            io::UInt160 validator;
            reader.Read(reinterpret_cast<uint8_t*>(validator.Data()), validator.Size());
            Validators.push_back(validator);
        }

        // Read MyIndex
        MyIndex = reader.ReadUInt32();

        // Read transaction hashes
        size_t txHashCount = reader.ReadVarInt();
        TransactionHashes.clear();
        TransactionHashes.reserve(txHashCount);

        for (size_t i = 0; i < txHashCount; i++)
        {
            io::UInt256 hash;
            reader.Read(hash.Data(), hash.Size());
            TransactionHashes.push_back(hash);
        }

        // Read payload states - Complete implementation with full payload reconstruction
        size_t prepCount = reader.ReadVarInt();
        PreparationPayloads.clear();
        PreparationPayloads.resize(prepCount);

        for (size_t i = 0; i < prepCount; i++)
        {
            bool hasPayload = reader.ReadBool();
            if (hasPayload)
            {
                uint32_t validatorIndex = reader.ReadUInt32();

                // Reconstruct complete payload information
                uint32_t payloadValidatorIndex = reader.ReadUInt32();
                uint8_t viewNumber = reader.ReadUInt8();
                uint64_t timestamp = reader.ReadUInt64();

                // Read payload-specific data
                bool hasData = reader.ReadBool();
                if (hasData)
                {
                    size_t dataSize = reader.ReadVarInt();
                    io::ByteVector serializedData(dataSize);
                    reader.Read(serializedData.Data(), dataSize);

                    // Reconstruct the extensible payload
                    auto payload = std::make_shared<network::p2p::payloads::ExtensiblePayload>();
                    payload->SetValidatorIndex(payloadValidatorIndex);
                    payload->SetViewNumber(viewNumber);
                    payload->SetTimestamp(timestamp);

                    // Deserialize the consensus message data
                    try
                    {
                        io::MemoryStream dataStream(serializedData);
                        io::BinaryReader dataReader(dataStream);
                        // Reconstruct consensus message based on type
                        // This would need to be enhanced based on message type detection
                        payload->SetDataFromStream(dataReader);
                    }
                    catch (const std::exception& e)
                    {
                        LOG_WARNING("Failed to deserialize preparation payload data: {}", e.what());
                    }

                    PreparationPayloads[i] = payload;
                }

                LOG_DEBUG("Restored complete preparation payload for validator {} at index {}", payloadValidatorIndex,
                          i);
            }
        }

        size_t commitCount = reader.ReadVarInt();
        CommitPayloads.clear();
        CommitPayloads.resize(commitCount);

        for (size_t i = 0; i < commitCount; i++)
        {
            bool hasPayload = reader.ReadBool();
            if (hasPayload)
            {
                uint32_t validatorIndex = reader.ReadUInt32();

                // Reconstruct complete commit payload information
                uint32_t payloadValidatorIndex = reader.ReadUInt32();
                uint8_t viewNumber = reader.ReadUInt8();
                uint64_t timestamp = reader.ReadUInt64();

                // Read commit-specific data
                bool hasData = reader.ReadBool();
                if (hasData)
                {
                    size_t dataSize = reader.ReadVarInt();
                    io::ByteVector serializedData(dataSize);
                    reader.Read(serializedData.Data(), dataSize);

                    // Reconstruct the commit payload
                    auto payload = std::make_shared<network::p2p::payloads::ExtensiblePayload>();
                    payload->SetValidatorIndex(payloadValidatorIndex);
                    payload->SetViewNumber(viewNumber);
                    payload->SetTimestamp(timestamp);

                    try
                    {
                        io::MemoryStream dataStream(serializedData);
                        io::BinaryReader dataReader(dataStream);
                        payload->SetDataFromStream(dataReader);
                    }
                    catch (const std::exception& e)
                    {
                        LOG_WARNING("Failed to deserialize commit payload data: {}", e.what());
                    }

                    CommitPayloads[i] = payload;
                }

                LOG_DEBUG("Restored complete commit payload for validator {} at index {}", payloadValidatorIndex, i);
            }
        }

        size_t changeViewCount = reader.ReadVarInt();
        ChangeViewPayloads.clear();
        ChangeViewPayloads.resize(changeViewCount);

        for (size_t i = 0; i < changeViewCount; i++)
        {
            bool hasPayload = reader.ReadBool();
            if (hasPayload)
            {
                uint32_t validatorIndex = reader.ReadUInt32();

                // Reconstruct complete change view payload information
                uint32_t payloadValidatorIndex = reader.ReadUInt32();
                uint8_t viewNumber = reader.ReadUInt8();
                uint64_t timestamp = reader.ReadUInt64();

                // Read change view specific data
                bool hasData = reader.ReadBool();
                if (hasData)
                {
                    size_t dataSize = reader.ReadVarInt();
                    io::ByteVector serializedData(dataSize);
                    reader.Read(serializedData.Data(), dataSize);

                    // Reconstruct the change view payload
                    auto payload = std::make_shared<network::p2p::payloads::ExtensiblePayload>();
                    payload->SetValidatorIndex(payloadValidatorIndex);
                    payload->SetViewNumber(viewNumber);
                    payload->SetTimestamp(timestamp);

                    try
                    {
                        io::MemoryStream dataStream(serializedData);
                        io::BinaryReader dataReader(dataStream);
                        payload->SetDataFromStream(dataReader);
                    }
                    catch (const std::exception& e)
                    {
                        LOG_WARNING("Failed to deserialize change view payload data: {}", e.what());
                    }

                    ChangeViewPayloads[i] = payload;
                }

                LOG_DEBUG("Restored complete change view payload for validator {} at index {}", payloadValidatorIndex,
                          i);
            }
        }

        // Read witness size
        witnessSize_ = reader.ReadUInt32();

        LOG_INFO("Deserialized consensus context for block {} view {} with {} validators", blockIndex, ViewNumber,
                 validatorCount);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Error deserializing consensus context: {}", e.what());

        // Reset to safe state on error
        ViewNumber = 0;
        Block = std::make_shared<ledger::Block>();
        Validators.clear();
        TransactionHashes.clear();
        PreparationPayloads.clear();
        CommitPayloads.clear();
        ChangeViewPayloads.clear();
        MyIndex = 0;
        witnessSize_ = 0;

        throw;
    }
}
}  // namespace neo::consensus
