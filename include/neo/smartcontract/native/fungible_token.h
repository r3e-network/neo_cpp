#pragma once

#include <neo/smartcontract/native/native_contract.h>
#include <neo/io/uint160.h>
#include <neo/vm/stack_item.h>
#include <neo/persistence/store_view.h>
#include <memory>
#include <string>

namespace neo::smartcontract::native
{
    /**
     * @brief The base class of all native tokens that are compatible with NEP-17.
     */
    class FungibleToken : public NativeContract
    {
    public:
        /**
         * @brief The storage prefix for balances.
         */
        static constexpr uint8_t PREFIX_BALANCE = 1;

        /**
         * @brief The storage prefix for total supply.
         */
        static constexpr uint8_t PREFIX_TOTAL_SUPPLY = 11;

        /**
         * @brief Constructs a FungibleToken.
         * @param name The name.
         * @param id The ID.
         */
        FungibleToken(const char* name, uint32_t id);

        /**
         * @brief Gets the symbol of the token.
         * @return The symbol.
         */
        virtual std::string GetSymbol() const = 0;

        /**
         * @brief Gets the number of decimal places of the token.
         * @return The number of decimal places.
         */
        virtual uint8_t GetDecimals() const = 0;

        /**
         * @brief Gets the factor used when calculating the displayed value of the token value.
         * @return The factor.
         */
        virtual int64_t GetFactor() const;

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
        virtual int64_t GetBalance(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const;

        /**
         * @brief Transfers tokens from one account to another.
         * @param snapshot The snapshot.
         * @param from The source account.
         * @param to The destination account.
         * @param amount The amount to transfer.
         * @return True if successful, false otherwise.
         */
        virtual bool Transfer(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& from, const io::UInt160& to, int64_t amount);

        /**
         * @brief Transfers tokens from one account to another.
         * @param engine The engine.
         * @param from The source account.
         * @param to The destination account.
         * @param amount The amount to transfer.
         * @param data The data.
         * @param callOnPayment Whether to call onPayment.
         * @return True if successful, false otherwise.
         */
        virtual bool Transfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment);

        /**
         * @brief Mints tokens to an account.
         * @param snapshot The snapshot.
         * @param account The account.
         * @param amount The amount to mint.
         * @return True if successful, false otherwise.
         */
        virtual bool Mint(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount);

        /**
         * @brief Mints tokens to an account.
         * @param engine The engine.
         * @param account The account.
         * @param amount The amount to mint.
         * @param callOnPayment Whether to call onPayment.
         * @return True if successful, false otherwise.
         */
        virtual bool Mint(ApplicationEngine& engine, const io::UInt160& account, int64_t amount, bool callOnPayment);

        /**
         * @brief Burns tokens from an account.
         * @param snapshot The snapshot.
         * @param account The account.
         * @param amount The amount to burn.
         * @return True if successful, false otherwise.
         */
        virtual bool Burn(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount);

        /**
         * @brief Burns tokens from an account.
         * @param engine The engine.
         * @param account The account.
         * @param amount The amount to burn.
         * @return True if successful, false otherwise.
         */
        virtual bool Burn(ApplicationEngine& engine, const io::UInt160& account, int64_t amount);

    protected:
        /**
         * @brief Called after a transfer.
         * @param engine The engine.
         * @param from The source account.
         * @param to The destination account.
         * @param amount The amount transferred.
         * @param data The data.
         * @param callOnPayment Whether to call onPayment.
         * @return True if successful, false otherwise.
         */
        virtual bool PostTransfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment);

        /**
         * @brief Called when a balance is changing.
         * @param engine The engine.
         * @param account The account.
         * @param amount The amount.
         */
        virtual void OnBalanceChanging(ApplicationEngine& engine, const io::UInt160& account, int64_t amount);
    };
}
