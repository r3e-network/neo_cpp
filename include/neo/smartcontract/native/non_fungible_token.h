/**
 * @file non_fungible_token.h
 * @brief Non Fungible Token
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/uint160.h>
#include <neo/persistence/store_view.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/vm/stack_item.h>

#include <map>
#include <memory>
#include <string>

namespace neo::smartcontract::native
{
/**
 * @brief The base class of all native tokens that are compatible with NEP-11.
 */
class NonFungibleToken : public NativeContract
{
   public:
    /**
     * @brief The storage prefix for token owners.
     */
    static constexpr uint8_t PREFIX_OWNER = 1;

    /**
     * @brief The storage prefix for token properties.
     */
    static constexpr uint8_t PREFIX_PROPERTIES = 2;

    /**
     * @brief The storage prefix for token balances.
     */
    static constexpr uint8_t PREFIX_BALANCE = 3;

    /**
     * @brief The storage prefix for token supply.
     */
    static constexpr uint8_t PREFIX_SUPPLY = 4;

    /**
     * @brief The storage prefix for token IDs.
     */
    static constexpr uint8_t PREFIX_TOKEN = 5;

    /**
     * @brief The storage prefix for token IDs by owner.
     */
    static constexpr uint8_t PREFIX_ACCOUNT_TOKEN = 6;

    /**
     * @brief Constructs a NonFungibleToken.
     * @param name The name.
     * @param id The ID.
     */
    NonFungibleToken(const char* name, uint32_t id);

    /**
     * @brief Gets the symbol of the token.
     * @return The symbol.
     */
    virtual std::string GetSymbol() const = 0;

    /**
     * @brief Gets the number of decimal places of the token.
     * @return The number of decimal places.
     */
    virtual uint8_t GetDecimals() const;

    /**
     * @brief Gets the total supply of the token.
     * @param snapshot The snapshot.
     * @return The total supply.
     */
    virtual int64_t GetTotalSupply(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Gets the balance of the specified account.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return The balance.
     */
    virtual int64_t GetBalanceOf(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const;

    /**
     * @brief Gets the owner of the specified token.
     * @param snapshot The snapshot.
     * @param tokenId The token ID.
     * @return The owner.
     */
    virtual io::UInt160 GetOwnerOf(std::shared_ptr<persistence::StoreView> snapshot,
                                   const io::ByteVector& tokenId) const;

    /**
     * @brief Gets the properties of the specified token.
     * @param snapshot The snapshot.
     * @param tokenId The token ID.
     * @return The properties.
     */
    virtual std::map<std::string, std::shared_ptr<vm::StackItem>> GetProperties(
        std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& tokenId) const;

    /**
     * @brief Gets all tokens.
     * @param snapshot The snapshot.
     * @return The tokens.
     */
    virtual std::vector<io::ByteVector> GetTokens(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Gets all tokens owned by the specified account.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return The tokens.
     */
    virtual std::vector<io::ByteVector> GetTokensOf(std::shared_ptr<persistence::StoreView> snapshot,
                                                    const io::UInt160& account) const;

    /**
     * @brief Transfers a token from one account to another.
     * @param snapshot The snapshot.
     * @param from The source account.
     * @param to The destination account.
     * @param tokenId The token ID.
     * @return True if successful, false otherwise.
     */
    virtual bool Transfer(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& from,
                          const io::UInt160& to, const io::ByteVector& tokenId);

    /**
     * @brief Transfers a token from one account to another.
     * @param engine The engine.
     * @param from The source account.
     * @param to The destination account.
     * @param tokenId The token ID.
     * @param data The data.
     * @param callOnPayment Whether to call onPayment.
     * @return True if successful, false otherwise.
     */
    virtual bool Transfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to,
                          const io::ByteVector& tokenId, std::shared_ptr<vm::StackItem> data, bool callOnPayment);

   protected:
    /**
     * @brief Mints a new token.
     * @param snapshot The snapshot.
     * @param tokenId The token ID.
     * @param owner The owner.
     * @param properties The properties.
     * @return True if successful, false otherwise.
     */
    virtual bool Mint(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& tokenId,
                      const io::UInt160& owner,
                      const std::map<std::string, std::shared_ptr<vm::StackItem>>& properties);

    /**
     * @brief Mints a new token.
     * @param engine The engine.
     * @param tokenId The token ID.
     * @param owner The owner.
     * @param properties The properties.
     * @param data The data.
     * @param callOnPayment Whether to call onPayment.
     * @return True if successful, false otherwise.
     */
    virtual bool Mint(ApplicationEngine& engine, const io::ByteVector& tokenId, const io::UInt160& owner,
                      const std::map<std::string, std::shared_ptr<vm::StackItem>>& properties,
                      std::shared_ptr<vm::StackItem> data, bool callOnPayment);

    /**
     * @brief Burns a token.
     * @param snapshot The snapshot.
     * @param tokenId The token ID.
     * @return True if successful, false otherwise.
     */
    virtual bool Burn(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& tokenId);

    /**
     * @brief Burns a token.
     * @param engine The engine.
     * @param tokenId The token ID.
     * @return True if successful, false otherwise.
     */
    virtual bool Burn(ApplicationEngine& engine, const io::ByteVector& tokenId);

    /**
     * @brief Called after a transfer.
     * @param engine The engine.
     * @param from The source account.
     * @param to The destination account.
     * @param amount The amount transferred.
     * @param tokenId The token ID.
     * @param data The data.
     * @param callOnPayment Whether to call onPayment.
     * @return True if successful, false otherwise.
     */
    virtual bool PostTransfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount,
                              const io::ByteVector& tokenId, std::shared_ptr<vm::StackItem> data, bool callOnPayment);
};
}  // namespace neo::smartcontract::native
