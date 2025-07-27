#pragma once

#include <neo/smartcontract/native/neo_token.h>

namespace neo::smartcontract::native
{
/**
 * @brief Committee-related methods for the NEO token.
 */
class NeoTokenCommittee
{
  public:
    /**
     * @brief Gets the committee members.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @return The committee members.
     */
    static std::vector<cryptography::ecc::ECPoint> GetCommittee(const NeoToken& token,
                                                                std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Gets the validators.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @return The validators.
     */
    static std::vector<cryptography::ecc::ECPoint> GetValidators(const NeoToken& token,
                                                                 std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Gets the next block validators.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param validatorsCount The number of validators.
     * @return The validators.
     */
    static std::vector<cryptography::ecc::ECPoint>
    GetNextBlockValidators(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                           int32_t validatorsCount);

    /**
     * @brief Computes the committee members.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param committeeSize The committee size.
     * @return The committee members.
     */
    static std::vector<cryptography::ecc::ECPoint>
    ComputeCommitteeMembers(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                            int32_t committeeSize);

    /**
     * @brief Helper method to check if the committee should be refreshed.
     * @param token The NEO token.
     * @param blockIndex The block index.
     * @param committeeSize The committee size.
     * @return True if the committee should be refreshed, false otherwise.
     */
    static bool ShouldRefreshCommittee(const NeoToken& token, uint32_t blockIndex, int32_t committeeSize);

    /**
     * @brief Helper method to get the committee from cache.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @return The committee.
     */
    static std::vector<NeoToken::CommitteeMember>
    GetCommitteeFromCache(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Gets the committee address.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @return The committee address.
     */
    static io::UInt160 GetCommitteeAddress(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Handles the getValidators method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnGetValidators(const NeoToken& token,
                                                          neo::smartcontract::ApplicationEngine& engine,
                                                          const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getCommittee method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnGetCommittee(const NeoToken& token,
                                                         neo::smartcontract::ApplicationEngine& engine,
                                                         const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getNextBlockValidators method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem>
    OnGetNextBlockValidators(const NeoToken& token, neo::smartcontract::ApplicationEngine& engine,
                             const std::vector<std::shared_ptr<vm::StackItem>>& args);
};
}  // namespace neo::smartcontract::native
