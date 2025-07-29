#include <algorithm>
#include <chrono>
#include <neo/consensus/consensus_service.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/network/message.h>
#include <neo/network/p2p/message_command.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/role_management.h>
#include <sstream>

namespace neo::consensus
{
ConsensusService::ConsensusService(std::shared_ptr<node::NeoSystem> neoSystem,
                                   const cryptography::ecc::KeyPair& keyPair)
    : neoSystem_(neoSystem), keyPair_(keyPair), lastChangeViewTime_(0), lastPrepareRequestTime_(0), lastBlockTime_(0),
      running_(false)
{
    InitializeValidators();
}

ConsensusService::~ConsensusService()
{
    Stop();
}

void ConsensusService::Start()
{
    if (running_)
        return;

    running_ = true;
    consensusThread_ = std::thread(&ConsensusService::RunConsensus, this);

    // Register message handler
    node_->RegisterMessageHandler(
        network::p2p::MessageCommand::Consensus,
        [this](const network::Message& message, const cryptography::ecc::ECPoint& sender)
        {
            try
            {
                std::istringstream stream(std::string(reinterpret_cast<const char*>(message.GetPayload().Data()),
                                                      message.GetPayload().Size()));
                io::BinaryReader reader(stream);

                auto consensusMessage = std::shared_ptr<ConsensusMessage>();

                // Read message type
                auto type = static_cast<MessageType>(reader.ReadByte());

                // Create message based on type
                switch (type)
                {
                    case MessageType::ChangeView:
                        consensusMessage = std::make_shared<ChangeViewMessage>(0, 0, 0);
                        break;
                    case MessageType::PrepareRequest:
                        consensusMessage = std::make_shared<PrepareRequest>(0, 0, 0, io::UInt160());
                        break;
                    case MessageType::PrepareResponse:
                        consensusMessage = std::make_shared<PrepareResponse>(0, io::UInt256());
                        break;
                    case MessageType::Commit:
                        consensusMessage = std::make_shared<CommitMessage>(0, io::UInt256(), io::ByteVector());
                        break;
                    case MessageType::RecoveryMessage:
                        consensusMessage = std::make_shared<RecoveryMessage>(0);
                        break;
                    case MessageType::RecoveryRequest:
                        consensusMessage = std::make_shared<RecoveryRequest>(0, 0);
                        break;
                    default:
                        throw std::runtime_error("Invalid message type");
                }

                // Reset stream position
                stream.seekg(0);

                // Deserialize message
                consensusMessage->Deserialize(reader);

                // Handle message
                OnMessage(consensusMessage, sender);
            }
            catch (const std::exception& ex)
            {
                // Log error
                std::cerr << "Failed to handle consensus message: " << ex.what() << std::endl;
            }
        });

    // Initialize consensus
    Initialize();
}

void ConsensusService::Stop()
{
    if (!running_)
        return;

    running_ = false;
    condition_.notify_all();

    if (consensusThread_.joinable())
        consensusThread_.join();

    // Unregister message handler
    node_->UnregisterMessageHandler(network::p2p::MessageCommand::Consensus);
}

bool ConsensusService::IsRunning() const
{
    return running_;
}

const std::vector<cryptography::ecc::ECPoint>& ConsensusService::GetValidators() const
{
    return context_ ? context_->GetValidators() : std::vector<cryptography::ecc::ECPoint>();
}

uint16_t ConsensusService::GetValidatorIndex() const
{
    return context_ ? context_->GetValidatorIndex() : 0xFFFF;
}

uint16_t ConsensusService::GetPrimaryIndex() const
{
    return context_ ? context_->GetPrimaryIndex() : 0;
}

uint8_t ConsensusService::GetViewNumber() const
{
    return context_ ? context_->GetViewNumber() : 0;
}

uint32_t ConsensusService::GetBlockIndex() const
{
    return context_ ? context_->GetBlockIndex() : 0;
}

const cryptography::ecc::KeyPair& ConsensusService::GetKeyPair() const
{
    return keyPair_;
}

std::shared_ptr<node::NeoSystem> ConsensusService::GetNeoSystem() const
{
    return neoSystem_;
}

void ConsensusService::Initialize()
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Reset consensus state
    Reset();

    // Get current block index
    uint32_t blockIndex = neoSystem_->GetBlockchain().GetCurrentBlockIndex() + 1;

    // Initialize validators
    InitializeValidators();

    // Create consensus context
    context_ = std::make_shared<ConsensusContext>(GetValidators(), GetValidatorIndex(), keyPair_, blockIndex);

    // Check if node is validator
    if (context_->GetValidatorIndex() == 0xFFFF)
    {
        // Not a validator, wait for next block
        std::this_thread::sleep_for(std::chrono::seconds(15));
        Initialize();
        return;
    }

    // Start consensus
    lastBlockTime_ = GetCurrentTimestamp();

    // Check if node is primary
    if (context_->IsPrimary())
    {
        // Send prepare request
        SendPrepareRequest();
    }
    else
    {
        // Send recovery request
        SendRecoveryRequest();
    }
}

void ConsensusService::RunConsensus()
{
    while (running_)
    {
        try
        {
            // Wait for timeout
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait_for(lock, std::chrono::seconds(1));

            // Check if node should change view
            if (ShouldChangeView())
            {
                // Send change view message
                SendChangeView();
            }

            // Check if node has received enough change view messages
            if (HasReceivedEnoughChangeViewMessages(viewNumber_ + 1))
            {
                // Change view
                ChangeView(viewNumber_ + 1);
            }

            // Check if node has received enough prepare responses
            if (HasReceivedEnoughPrepareResponses())
            {
                // Send commit
                SendCommit();
            }

            // Check if node has received enough commits
            if (HasReceivedEnoughCommits())
            {
                // Create block
                auto block = CreateBlock();

                // Process block
                ProcessBlock(block);

                // Reset consensus
                Reset();

                // Initialize consensus
                Initialize();
            }
        }
        catch (const std::exception& ex)
        {
            // Log error
            std::cerr << "Consensus error: " << ex.what() << std::endl;

            // Reset consensus
            Reset();

            // Initialize consensus
            Initialize();
        }
    }
}

bool ConsensusService::IsPrimary() const
{
    return context_ ? context_->IsPrimary() : false;
}

bool ConsensusService::IsBackup() const
{
    return context_ ? context_->IsBackup() : false;
}

uint16_t ConsensusService::GetPrimaryIndex(uint8_t viewNumber) const
{
    return context_ ? context_->GetPrimaryIndex(viewNumber) : 0;
}

void ConsensusService::OnPrepareRequestReceived(const PrepareRequest& request)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!context_ || request.GetValidatorIndex() != GetPrimaryIndex())
        return;

    // Verify prepare request
    if (request.GetTimestamp() <= lastBlockTime_)
        return;

    // Process the prepare request
    context_->SetPrepareRequest(std::make_shared<PrepareRequest>(request));

    // Send prepare response if we agree
    if (context_->IsPrepareResponseSent())
        return;

    auto response = std::make_shared<PrepareResponse>();
    response->SetBlockHash(request.GetBlockHash());
    response->SetValidatorIndex(GetValidatorIndex());

    SignAndBroadcast(response);
    context_->SetPrepareResponseSent(true);
}

void ConsensusService::OnPrepareResponseReceived(const PrepareResponse& response)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!context_)
        return;

    // Add prepare response to context
    context_->AddPrepareResponse(response);

    // Check if we have enough prepare responses
    if (context_->GetPrepareResponseCount() >= context_->GetM())
    {
        if (!context_->IsCommitSent())
        {
            auto commit = std::make_shared<CommitMessage>();
            commit->SetValidatorIndex(GetValidatorIndex());
            commit->SetSignature(SignBlock(context_->GetBlock()));

            SignAndBroadcast(commit);
            context_->SetCommitSent(true);
        }
    }
}

void ConsensusService::OnChangeViewReceived(const ChangeViewMessage& message)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!context_)
        return;

    // Add change view to context
    context_->AddChangeView(message);

    // Check if we need to change view
    if (context_->GetChangeViewCount() >= context_->GetM())
    {
        uint8_t newViewNumber = message.GetViewNumber();
        if (newViewNumber > context_->GetViewNumber())
        {
            ChangeView(newViewNumber);
        }
    }
}

void ConsensusService::OnCommitReceived(const CommitMessage& commit)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!context_)
        return;

    // Add commit to context
    context_->AddCommit(commit);

    // Check if we have enough commits
    if (context_->GetCommitCount() >= context_->GetM())
    {
        // Create and persist block
        auto block = context_->GetBlock();
        if (block && ValidateBlock(block))
        {
            PersistBlock(block);
            Initialize();  // Reset for next round
        }
    }
}

bool ConsensusService::ValidateBlock(std::shared_ptr<ledger::Block> block)
{
    if (!block)
        return false;

    // Verify block timestamp
    if (block->GetTimestamp() <= lastBlockTime_)
        return false;

    // Verify block index
    if (block->GetIndex() != GetBlockIndex())
        return false;

    // Verify merkle root
    auto merkleRoot = block->ComputeMerkleRoot();
    if (merkleRoot != block->GetMerkleRoot())
        return false;

    // Verify transactions
    for (const auto& tx : block->GetTransactions())
    {
        if (!neoSystem_->GetBlockchain().VerifyTransaction(tx))
            return false;
    }

    return true;
}

}  // namespace neo::consensus
