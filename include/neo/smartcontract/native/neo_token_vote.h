#pragma once

#include <neo/smartcontract/native/neo_token.h>

namespace neo::smartcontract::native
{
    /**
     * @brief Vote-related methods for the NEO token.
     */
    class NeoTokenVote
    {
    public:
        /**
         * @brief Votes for a candidate.
         * @param token The NEO token.
         * @param snapshot The snapshot.
         * @param account The account.
         * @param pubKeys The public keys.
         * @return True if the vote was successful, false otherwise.
         */
        static bool Vote(const NeoToken& token, std::shared_ptr<persistence::DataCache> snapshot, const io::UInt160& account, const std::vector<cryptography::ecc::ECPoint>& pubKeys);

        /**
         * @brief Handles the vote method.
         * @param token The NEO token.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        static std::shared_ptr<vm::StackItem> OnVote(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);
        
        /**
         * @brief Handles the unVote method.
         * @param token The NEO token.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        static std::shared_ptr<vm::StackItem> OnUnVote(const NeoToken& token, ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);
    };
}
