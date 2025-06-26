#pragma once

#include <neo/smartcontract/native/fungible_token.h>
#include <neo/io/uint160.h>
#include <memory>
#include <string>

namespace neo::smartcontract::native
{
    /**
     * @brief Represents the Gas token native contract.
     */
    class GasToken : public FungibleToken
    {
        // Friend classes for testing
        friend class NativeContractTest;
        friend class GasTokenTest;
        
    public:
        /**
         * @brief The contract ID.
         */
        static constexpr uint32_t ID = 2;

        /**
         * @brief The contract name.
         */
        static constexpr const char* NAME = "Gas";

        /**
         * @brief The storage prefix for balances.
         */
        static constexpr uint8_t PREFIX_BALANCE = 1;

        /**
         * @brief The storage prefix for total supply.
         */
        static constexpr uint8_t PREFIX_TOTAL_SUPPLY = 2;

        /**
         * @brief The storage prefix for gas per block.
         */
        static constexpr uint8_t PREFIX_GAS_PER_BLOCK = 3;

        /**
         * @brief The storage prefix for gas distribution.
         */
        static constexpr uint8_t PREFIX_GAS_DISTRIBUTION = 4;

        /**
         * @brief The factor.
         */
        static constexpr int64_t FACTOR = 100000000;

        /**
         * @brief The total supply.
         */
        static constexpr int64_t TOTAL_SUPPLY = 100000000 * FACTOR;

        /**
         * @brief Constructs a GasToken.
         */
        GasToken();

        /**
         * @brief Gets the symbol of the token.
         * @return The symbol.
         */
        std::string GetSymbol() const override;

        /**
         * @brief Gets the number of decimal places of the token.
         * @return The number of decimal places.
         */
        uint8_t GetDecimals() const override;

        /**
         * @brief Gets the balance.
         * @param snapshot The snapshot.
         * @param account The account.
         * @return The balance.
         */
        int64_t GetBalance(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const override;

        /**
         * @brief Gets the total supply.
         * @param snapshot The snapshot.
         * @return The total supply.
         */
        int64_t GetTotalSupply(std::shared_ptr<persistence::StoreView> snapshot) const override;

        /**
         * @brief Gets the gas per block.
         * @param snapshot The snapshot.
         * @return The gas per block.
         */
        int64_t GetGasPerBlock(std::shared_ptr<persistence::StoreView> snapshot) const;

        /**
         * @brief Sets the gas per block.
         * @param snapshot The snapshot.
         * @param gasPerBlock The gas per block.
         */
        void SetGasPerBlock(std::shared_ptr<persistence::StoreView> snapshot, int64_t gasPerBlock);

        /**
         * @brief Transfers tokens between accounts.
         * @param snapshot The snapshot.
         * @param from The from account.
         * @param to The to account.
         * @param amount The amount to transfer.
         * @return True if the transfer was successful, false otherwise.
         */
        bool Transfer(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& from, const io::UInt160& to, int64_t amount);

        /**
         * @brief Transfers tokens between accounts with callback.
         * @param engine The application engine.
         * @param from The from account.
         * @param to The to account.
         * @param amount The amount to transfer.
         * @param data The data to pass to the callback.
         * @param callOnPayment Whether to call the onNEP17Payment callback.
         * @return True if the transfer was successful, false otherwise.
         */
        bool Transfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment);

        /**
         * @brief Mints tokens to an account.
         * @param snapshot The snapshot.
         * @param account The account.
         * @param amount The amount to mint.
         * @return True if the minting was successful, false otherwise.
         */
        bool Mint(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount);

        /**
         * @brief Mints tokens to an account with callback.
         * @param engine The application engine.
         * @param account The account.
         * @param amount The amount to mint.
         * @param callOnPayment Whether to call the onNEP17Payment callback.
         * @return True if the minting was successful, false otherwise.
         */
        bool Mint(ApplicationEngine& engine, const io::UInt160& account, int64_t amount, bool callOnPayment);

        /**
         * @brief Burns tokens from an account.
         * @param snapshot The snapshot.
         * @param account The account.
         * @param amount The amount to burn.
         * @return True if the burning was successful, false otherwise.
         */
        bool Burn(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount);

        /**
         * @brief Burns tokens from an account with callback.
         * @param engine The application engine.
         * @param account The account.
         * @param amount The amount to burn.
         * @return True if the burning was successful, false otherwise.
         */
        bool Burn(ApplicationEngine& engine, const io::UInt160& account, int64_t amount);

        /**
         * @brief Handles post-transfer operations.
         * @param engine The application engine.
         * @param from The from account.
         * @param to The to account.
         * @param amount The amount transferred.
         * @param data The data passed to the transfer.
         * @param callOnPayment Whether to call the onNEP17Payment callback.
         * @return True if the post-transfer was successful, false otherwise.
         */
        bool PostTransfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment);

        /**
         * @brief Gets the instance.
         * @return The instance.
         */
        static std::shared_ptr<GasToken> GetInstance();

        /**
         * @brief Handles the OnPersist event.
         * @param engine The engine.
         * @return True if the event was handled, false otherwise.
         */
        bool OnPersist(ApplicationEngine& engine);

        /**
         * @brief Handles the PostPersist event.
         * @param engine The engine.
         * @return True if the event was handled, false otherwise.
         */
        bool PostPersist(ApplicationEngine& engine);

        /**
         * @brief Initializes the contract when it's first deployed.
         * @param engine The application engine.
         * @param hardfork The hardfork version.
         * @return True if the initialization was successful, false otherwise.
         */
        bool InitializeContract(ApplicationEngine& engine, uint32_t hardfork);

    protected:
        /**
         * @brief Initializes the contract.
         */
        void Initialize() override;

    private:
        /**
         * @brief Handles the symbol method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnSymbol(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the decimals method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnDecimals(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the totalSupply method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnTotalSupply(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the balanceOf method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBalanceOf(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the transfer method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnTransfer(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the onNEP17Payment method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnNEP17Payment(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);
    };
}
