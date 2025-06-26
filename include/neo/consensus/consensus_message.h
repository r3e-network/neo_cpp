#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <vector>
#include <memory>

namespace neo::consensus
{
    /**
     * @brief Type of consensus message
     */
    enum class ConsensusMessageType : uint8_t
    {
        ChangeView = 0x00,
        PrepareRequest = 0x20,
        PrepareResponse = 0x21,
        Commit = 0x30,
        RecoveryRequest = 0x40,
        RecoveryMessage = 0x41
    };

    /**
     * @brief Base class for all consensus messages
     */
    class ConsensusMessage : public io::ISerializable
    {
    protected:
        ConsensusMessageType type_;
        uint32_t view_number_;
        uint32_t validator_index_;
        uint32_t block_index_;
        
    public:
        ConsensusMessage(ConsensusMessageType type);
        
        ConsensusMessageType GetType() const { return type_; }
        uint32_t GetViewNumber() const { return view_number_; }
        uint32_t GetValidatorIndex() const { return validator_index_; }
        uint32_t GetBlockIndex() const { return block_index_; }
        
        void SetViewNumber(uint32_t view) { view_number_ = view; }
        void SetValidatorIndex(uint32_t index) { validator_index_ = index; }
        void SetBlockIndex(uint32_t index) { block_index_ = index; }
        
        // ISerializable
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
        
        /**
         * @brief Create message from type
         */
        static std::unique_ptr<ConsensusMessage> CreateFromType(ConsensusMessageType type);
    };

    /**
     * @brief View change request message
     */
    class ViewChangeMessage : public ConsensusMessage
    {
    private:
        uint32_t new_view_number_;
        std::chrono::system_clock::time_point timestamp_;
        
    public:
        ViewChangeMessage();
        
        uint32_t GetNewViewNumber() const { return new_view_number_; }
        void SetNewViewNumber(uint32_t view) { new_view_number_ = view; }
        
        std::chrono::system_clock::time_point GetTimestamp() const { return timestamp_; }
        void SetTimestamp(std::chrono::system_clock::time_point time) { timestamp_ = time; }
        
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
    };

    /**
     * @brief Prepare request from primary node
     */
    class PrepareRequestMessage : public ConsensusMessage
    {
    private:
        uint64_t nonce_;
        std::chrono::system_clock::time_point timestamp_;
        std::vector<io::UInt256> transaction_hashes_;
        
    public:
        PrepareRequestMessage();
        
        uint64_t GetNonce() const { return nonce_; }
        void SetNonce(uint64_t nonce) { nonce_ = nonce; }
        
        std::chrono::system_clock::time_point GetTimestamp() const { return timestamp_; }
        void SetTimestamp(std::chrono::system_clock::time_point time) { timestamp_ = time; }
        
        const std::vector<io::UInt256>& GetTransactionHashes() const { return transaction_hashes_; }
        void SetTransactionHashes(const std::vector<io::UInt256>& hashes) { transaction_hashes_ = hashes; }
        
        /**
         * @brief Calculate hash of this prepare request
         */
        io::UInt256 GetHash() const;
        
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
    };

    /**
     * @brief Prepare response from backup nodes
     */
    class PrepareResponseMessage : public ConsensusMessage
    {
    private:
        io::UInt256 prepare_request_hash_;
        
    public:
        PrepareResponseMessage();
        
        const io::UInt256& GetPrepareRequestHash() const { return prepare_request_hash_; }
        void SetPrepareRequestHash(const io::UInt256& hash) { prepare_request_hash_ = hash; }
        
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
    };

    /**
     * @brief Commit message with signature
     */
    class CommitMessage : public ConsensusMessage
    {
    private:
        std::vector<uint8_t> signature_;
        
    public:
        CommitMessage();
        
        const std::vector<uint8_t>& GetSignature() const { return signature_; }
        void SetSignature(const std::vector<uint8_t>& sig) { signature_ = sig; }
        
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
    };

    /**
     * @brief Recovery request message
     */
    class RecoveryRequestMessage : public ConsensusMessage
    {
    public:
        RecoveryRequestMessage();
        
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
    };

    /**
     * @brief Recovery response message
     */
    class RecoveryMessage : public ConsensusMessage
    {
    private:
        std::unique_ptr<ViewChangeMessage> view_change_;
        std::unique_ptr<PrepareRequestMessage> prepare_request_;
        std::vector<std::unique_ptr<PrepareResponseMessage>> prepare_responses_;
        std::vector<std::unique_ptr<CommitMessage>> commits_;
        
    public:
        RecoveryMessage();
        
        void SetViewChange(std::unique_ptr<ViewChangeMessage> msg) { view_change_ = std::move(msg); }
        void SetPrepareRequest(std::unique_ptr<PrepareRequestMessage> msg) { prepare_request_ = std::move(msg); }
        void AddPrepareResponse(std::unique_ptr<PrepareResponseMessage> msg);
        void AddCommit(std::unique_ptr<CommitMessage> msg);
        
        const ViewChangeMessage* GetViewChange() const { return view_change_.get(); }
        const PrepareRequestMessage* GetPrepareRequest() const { return prepare_request_.get(); }
        const std::vector<std::unique_ptr<PrepareResponseMessage>>& GetPrepareResponses() const { return prepare_responses_; }
        const std::vector<std::unique_ptr<CommitMessage>>& GetCommits() const { return commits_; }
        
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
    };
}