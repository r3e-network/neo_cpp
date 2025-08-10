#pragma once

#include <neo/smartcontract/native/neo_token.h>

namespace neo::smartcontract::native
{
/**
 * @brief Candidate-related methods for the NEO token.
 */
class NeoTokenCandidate
{
   public:
    /**
     * @brief Registers a candidate.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @return True if the candidate was registered, false otherwise.
     */
    static bool RegisterCandidate(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                  const cryptography::ecc::ECPoint& pubKey);

    /**
     * @brief Unregisters a candidate.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @return True if the candidate was unregistered, false otherwise.
     */
    static bool UnregisterCandidate(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                    const cryptography::ecc::ECPoint& pubKey);

    /**
     * @brief Gets the candidate state.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @return The candidate state.
     */
    static NeoToken::CandidateState GetCandidateState(const NeoToken& token,
                                                      std::shared_ptr<persistence::DataCache> snapshot,
                                                      const cryptography::ecc::ECPoint& pubKey);

    /**
     * @brief Gets all candidates.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @return The candidates.
     */
    static std::vector<std::pair<cryptography::ecc::ECPoint, NeoToken::CandidateState>> GetCandidates(
        const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Gets the candidate vote.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @return The candidate vote.
     */
    static int64_t GetCandidateVote(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                                    const cryptography::ecc::ECPoint& pubKey);

    /**
     * @brief Checks a candidate.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @param pubKey The public key.
     * @param state The candidate state.
     */
    static void CheckCandidate(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot,
                               const cryptography::ecc::ECPoint& pubKey, const NeoToken::CandidateState& state);

    /**
     * @brief Handles the registerCandidate method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnRegisterCandidate(const NeoToken& token, ApplicationEngine& engine,
                                                              const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the unregisterCandidate method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnUnregisterCandidate(
        const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getCandidates method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnGetCandidates(const NeoToken& token, ApplicationEngine& engine,
                                                          const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getCandidateVote method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnGetCandidateVote(const NeoToken& token, ApplicationEngine& engine,
                                                             const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Gets the register price.
     * @param token The NEO token.
     * @param snapshot The snapshot.
     * @return The register price.
     */
    static int64_t GetRegisterPrice(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Handles the getRegisterPrice method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnGetRegisterPrice(const NeoToken& token, ApplicationEngine& engine,
                                                             const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setRegisterPrice method.
     * @param token The NEO token.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    static std::shared_ptr<vm::StackItem> OnSetRegisterPrice(const NeoToken& token, ApplicationEngine& engine,
                                                             const std::vector<std::shared_ptr<vm::StackItem>>& args);
};
}  // namespace neo::smartcontract::native
