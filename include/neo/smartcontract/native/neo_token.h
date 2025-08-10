#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/fixed8.h>
#include <neo/io/uint160.h>
#include <neo/persistence/data_cache.h>
#include <neo/smartcontract/native/native_contract.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::smartcontract::native
{
// Forward declarations
class GasToken;
class NeoTokenGas;
class NeoTokenCandidate;
class NeoTokenCommittee;

/**
 * @brief Represents the NEO token contract.
 */
class NeoToken : public NativeContract
{
    // Friend classes for testing
    friend class NativeContractTest;
    friend class NeoTokenTest;

   public:
    /**
     * @brief Type alias for method handler function.
     */
    using MethodHandler = std::function<std::shared_ptr<vm::StackItem>(
        neo::smartcontract::ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>;

    /**
     * @brief The contract ID (matches Neo C# implementation).
     */
    static constexpr int32_t ID = 2;

    /**
     * @brief The contract name.
     */
    static constexpr const char* NAME = "Neo";

    /**
     * @brief The token symbol.
     */
    static constexpr const char* SYMBOL = "NEO";

    /**
     * @brief The token decimals.
     */
    static constexpr uint8_t DECIMALS = 0;

    /**
     * @brief The total amount of NEO.
     */
    static constexpr int64_t TOTAL_AMOUNT = 100000000;

    /**
     * @brief The effective voter turnout in NEO.
     * The voted candidates will only be effective when the voting turnout exceeds this value.
     */
    static constexpr double EFFECTIVE_VOTER_TURNOUT = 0.2;

    /**
     * @brief The committee reward ratio (percentage of gas per block).
     */
    static constexpr int COMMITTEE_REWARD_RATIO = 10;  // 10% of gas per block

    /**
     * @brief The NEO holder reward ratio (percentage of gas per block).
     */
    static constexpr int NEO_HOLDER_REWARD_RATIO = 10;  // 10% of gas per block

    /**
     * @brief The voter reward ratio (percentage of gas per block).
     */
    static constexpr int VOTER_REWARD_RATIO = 80;  // 80% of gas per block

    // Storage prefixes (matching C# implementation)
    static constexpr uint8_t PREFIX_VOTERS_COUNT = 1;
    static constexpr uint8_t PREFIX_CANDIDATE = 33;
    static constexpr uint8_t PREFIX_COMMITTEE = 14;
    static constexpr uint8_t PREFIX_GAS_PER_BLOCK = 29;
    static constexpr uint8_t PREFIX_REGISTER_PRICE = 13;
    static constexpr uint8_t PREFIX_VOTER_REWARD_PER_COMMITTEE = 23;

    /**
     * @brief Gets the contract ID.
     * @return The contract ID.
     */
    static io::UInt160 GetContractId();

    /**
     * @brief Gets the instance.
     * @return The instance.
     */
    static std::shared_ptr<NeoToken> GetInstance();

    /**
     * @brief Initializes the contract when it's first deployed.
     * @param engine The application engine.
     * @param hardfork The hardfork version.
     * @return True if the initialization was successful, false otherwise.
     */
    bool InitializeContract(neo::smartcontract::ApplicationEngine& engine, uint32_t hardfork);

    /**
     * @brief Gets the total supply.
     * @param snapshot The snapshot.
     * @return The total supply.
     */
    io::Fixed8 GetTotalSupply(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Gets the token symbol.
     * @return The token symbol "NEO".
     */
    std::string Symbol() const;

    /**
     * @brief Gets the token decimals.
     * @return The token decimals (0 for NEO).
     */
    uint8_t Decimals() const;

    /**
     * @brief Gets the balance of an account.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return The balance.
     */
    io::Fixed8 GetBalance(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account) const;

    /**
     * @brief Transfers NEO from one account to another.
     * @param engine The application engine.
     * @param snapshot The snapshot.
     * @param from The from account.
     * @param to The to account.
     * @param amount The amount.
     * @return True if the transfer was successful, false otherwise.
     */
    bool Transfer(ApplicationEngine& engine, std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& from,
                  const io::UInt160& to, const io::Fixed8& amount);

    /**
     * @brief Gets the validators.
     * @param snapshot The snapshot.
     * @return The validators.
     */
    std::vector<cryptography::ecc::ECPoint> GetValidators(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Registers a candidate.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @return True if the candidate was registered, false otherwise.
     */
    bool RegisterCandidate(std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey);

    /**
     * @brief Unregisters a candidate.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @return True if the candidate was unregistered, false otherwise.
     */
    bool UnregisterCandidate(std::shared_ptr<persistence::DataCache> snapshot,
                             const cryptography::ecc::ECPoint& pubKey);

    /**
     * @brief Votes for a candidate.
     * @param snapshot The snapshot.
     * @param account The account.
     * @param pubKeys The public keys.
     * @return True if the vote was successful, false otherwise.
     */
    bool Vote(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account,
              const std::vector<cryptography::ecc::ECPoint>& pubKeys);

    /**
     * @brief Gets the committee members.
     * @param snapshot The snapshot.
     * @return The committee members.
     */
    std::vector<cryptography::ecc::ECPoint> GetCommittee(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Gets the next block validators.
     * @param snapshot The snapshot.
     * @param validatorsCount The number of validators.
     * @return The validators.
     */
    std::vector<cryptography::ecc::ECPoint> GetNextBlockValidators(std::shared_ptr<persistence::DataCache> snapshot,
                                                                   int32_t validatorsCount) const;

    /**
     * @brief Gets the register price.
     * @param snapshot The snapshot.
     * @return The register price.
     */
    int64_t GetRegisterPrice(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Gets the storage prefix.
     * @return The storage prefix.
     */
    io::ByteVector GetStoragePrefix() const override;

   protected:
    /**
     * @brief Initializes the contract.
     */
    void Initialize() override;

   public:
    /**
     * @brief Account state structure.
     */
    struct AccountState
    {
        int64_t balance;
        uint32_t balanceHeight;
        cryptography::ecc::ECPoint voteTo;
        int64_t lastGasPerVote;

        void Serialize(io::BinaryWriter& writer) const;
        void Deserialize(io::BinaryReader& reader);
    };

    /**
     * @brief Candidate state structure.
     */
    struct CandidateState
    {
        bool registered;
        int64_t votes;

        void Serialize(io::BinaryWriter& writer) const;
        void Deserialize(io::BinaryReader& reader);
    };

    /**
     * @brief Committee member structure.
     */
    struct CommitteeMember
    {
        cryptography::ecc::ECPoint publicKey;
        int64_t votes;

        void Serialize(io::BinaryWriter& writer) const;
        void Deserialize(io::BinaryReader& reader);
    };

    /**
     * @brief Gas distribution structure.
     */
    struct GasDistribution
    {
        io::UInt160 account;
        int64_t amount;
    };

    /**
     * @brief Gets the gas per block.
     * @param snapshot The snapshot.
     * @return The gas per block.
     */
    int64_t GetGasPerBlock(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Sets the gas per block.
     * @param snapshot The snapshot.
     * @param gasPerBlock The gas per block.
     */
    void SetGasPerBlock(std::shared_ptr<persistence::DataCache> snapshot, int64_t gasPerBlock);

    /**
     * @brief Gets the unclaimed gas for an account.
     * @param snapshot The snapshot.
     * @param account The account.
     * @param end The block index used when calculating GAS.
     * @return The unclaimed gas.
     */
    int64_t GetUnclaimedGas(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account,
                            uint32_t end) const;

    /**
     * @brief Gets the committee address.
     * @param snapshot The snapshot.
     * @return The committee address.
     */
    io::UInt160 GetCommitteeAddress(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Gets the account state.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return The account state.
     */
    AccountState GetAccountState(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account) const;

    /**
     * @brief Gets the candidate state.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @return The candidate state.
     */
    CandidateState GetCandidateState(std::shared_ptr<persistence::DataCache> snapshot,
                                     const cryptography::ecc::ECPoint& pubKey) const;

    /**
     * @brief Gets all candidates.
     * @param snapshot The snapshot.
     * @return The candidates.
     */
    std::vector<std::pair<cryptography::ecc::ECPoint, CandidateState>> GetCandidates(
        std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Gets the candidate vote.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @return The candidate vote.
     */
    int64_t GetCandidateVote(std::shared_ptr<persistence::DataCache> snapshot,
                             const cryptography::ecc::ECPoint& pubKey) const;

    /**
     * @brief Distributes gas to an account.
     * @param engine The engine.
     * @param account The account.
     * @param state The account state.
     * @return The gas distribution.
     */
    GasDistribution DistributeGas(neo::smartcontract::ApplicationEngine& engine, const io::UInt160& account,
                                  const AccountState& state);

    /**
     * @brief Computes the committee members.
     * @param snapshot The snapshot.
     * @param committeeSize The committee size.
     * @return The committee members.
     */
    std::vector<cryptography::ecc::ECPoint> ComputeCommitteeMembers(std::shared_ptr<persistence::DataCache> snapshot,
                                                                    int32_t committeeSize) const;

    /**
     * @brief Handles the OnPersist event.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    bool OnPersist(neo::smartcontract::ApplicationEngine& engine);

    /**
     * @brief Handles the PostPersist event.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    bool PostPersist(neo::smartcontract::ApplicationEngine& engine);

    /**
     * @brief Constructs a NeoToken.
     * @note This constructor should only be called by GetInstance().
     */
    NeoToken();

    /**
     * @brief Storage prefixes for the NEO token.
     */
    enum class StoragePrefix : uint8_t
    {
        TotalSupply = 0x00,
        Account = 0x01,
        Candidate = 0x21,  // 33 in decimal = PREFIX_CANDIDATE
        Vote = 0x03,
        Committee = 0x0E,  // 14 in decimal = PREFIX_COMMITTEE
        VotersCount = 0x05,
        GasPerBlock = 0x06,
        RegisterPrice = 0x07,
        VoterReward = 0x08,
        Voter = 0x22  // 34 in decimal = PREFIX_VOTER
    };

    // Storage prefix constants for compatibility
    static constexpr uint8_t PREFIX_BALANCE = 1;
    static constexpr uint8_t PREFIX_VOTER = 34;

   private:
    /**
     * @brief Handles the totalSupply method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnTotalSupply(neo::smartcontract::ApplicationEngine& engine,
                                                 const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the balanceOf method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnBalanceOf(neo::smartcontract::ApplicationEngine& engine,
                                               const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the transfer method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnTransfer(neo::smartcontract::ApplicationEngine& engine,
                                              const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getValidators method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetValidators(neo::smartcontract::ApplicationEngine& engine,
                                                   const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getCommittee method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetCommittee(neo::smartcontract::ApplicationEngine& engine,
                                                  const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getNextBlockValidators method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetNextBlockValidators(neo::smartcontract::ApplicationEngine& engine,
                                                            const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the registerCandidate method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnRegisterCandidate(neo::smartcontract::ApplicationEngine& engine,
                                                       const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the unregisterCandidate method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnUnregisterCandidate(neo::smartcontract::ApplicationEngine& engine,
                                                         const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the vote method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnVote(neo::smartcontract::ApplicationEngine& engine,
                                          const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getGasPerBlock method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetGasPerBlock(neo::smartcontract::ApplicationEngine& engine,
                                                    const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setGasPerBlock method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetGasPerBlock(neo::smartcontract::ApplicationEngine& engine,
                                                    const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Helper method to check if the committee should be refreshed.
     * @param blockIndex The block index.
     * @param committeeSize The committee size.
     * @return True if the committee should be refreshed, false otherwise.
     */
    bool ShouldRefreshCommittee(uint32_t blockIndex, int32_t committeeSize) const;

    /**
     * @brief Helper method to get the committee from cache.
     * @param snapshot The snapshot.
     * @return The committee.
     */
    std::vector<CommitteeMember> GetCommitteeFromCache(std::shared_ptr<persistence::DataCache> snapshot) const;

    /**
     * @brief Calculates the bonus for an account.
     * @param snapshot The snapshot.
     * @param state The account state.
     * @param end The block index.
     * @return The bonus.
     */
    int64_t CalculateBonus(std::shared_ptr<persistence::DataCache> snapshot, const AccountState& state,
                           uint32_t end) const;

    /**
     * @brief Calculates the NEO holder reward.
     * @param snapshot The snapshot.
     * @param value The value.
     * @param start The start block index.
     * @param end The end block index.
     * @return The reward.
     */
    int64_t CalculateNeoHolderReward(std::shared_ptr<persistence::DataCache> snapshot, int64_t value, uint32_t start,
                                     uint32_t end) const;

    /**
     * @brief Checks a candidate.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @param state The candidate state.
     */
    void CheckCandidate(std::shared_ptr<persistence::DataCache> snapshot, const cryptography::ecc::ECPoint& pubKey,
                        const CandidateState& state);

    /**
     * @brief Handles the getAccountState method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetAccountState(neo::smartcontract::ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getCandidates method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetCandidates(neo::smartcontract::ApplicationEngine& engine,
                                                   const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getCandidateVote method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetCandidateVote(neo::smartcontract::ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getRegisterPrice method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetRegisterPrice(neo::smartcontract::ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setRegisterPrice method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetRegisterPrice(neo::smartcontract::ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getUnclaimedGas method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetUnclaimedGas(neo::smartcontract::ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);
};
}  // namespace neo::smartcontract::native
