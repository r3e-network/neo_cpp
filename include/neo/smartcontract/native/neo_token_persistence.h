/**
 * @file neo_token_persistence.h
 * @brief NEO governance token contract
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/smartcontract/native/neo_token.h>

namespace neo::smartcontract::native
{
/**
 * @brief Persistence-related methods for the NEO token.
 */
class NeoTokenPersistence
{
   public:
    /**
     * @brief Initializes the contract when it's first deployed.
     * @param token The NEO token.
     * @param engine The application engine.
     * @param hardfork The hardfork version.
     * @return True if the initialization was successful, false otherwise.
     */
    static bool InitializeContract(const NeoToken& token, ApplicationEngine& engine, uint32_t hardfork);

    /**
     * @brief Handles the OnPersist event.
     * @param token The NEO token.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    static bool OnPersist(const NeoToken& token, ApplicationEngine& engine);

    /**
     * @brief Handles the PostPersist event.
     * @param token The NEO token.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    static bool PostPersist(const NeoToken& token, ApplicationEngine& engine);
};
}  // namespace neo::smartcontract::native
