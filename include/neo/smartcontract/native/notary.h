/**
 * @file notary.h
 * @brief Notary
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/uint160.h>
#include <neo/persistence/store_view.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/vm/iinteroperable.h>
#include <neo/vm/ireference_counter.h>
#include <neo/vm/stack_item.h>

#include <memory>
#include <string>

// Forward declarations
namespace neo::smartcontract
{
class ApplicationEngine;
}

namespace neo::smartcontract::native
{

/**
 * @brief The Notary native contract used for multisignature transactions forming assistance.
 */
class Notary : public NativeContract
{
   public:
    // Forward declaration
    class Deposit;

    /**
     * @brief The contract ID.
     */
    static constexpr int32_t ID = -10;

    /**
     * @brief A default value for maximum allowed NotValidBeforeDelta. It is set to be
     * 20 rounds for 7 validators, a little more than half an hour for 15-seconds blocks.
     */
    static constexpr int DEFAULT_MAX_NOT_VALID_BEFORE_DELTA = 140;

    /**
     * @brief A default value for deposit lock period.
     */
    static constexpr int DEFAULT_DEPOSIT_DELTA_TILL = 5760;

    /**
     * @brief The storage prefix for deposits.
     */
    static constexpr uint8_t PREFIX_DEPOSIT = 1;

    /**
     * @brief The storage prefix for max not valid before delta.
     */
    static constexpr uint8_t PREFIX_MAX_NOT_VALID_BEFORE_DELTA = 10;

    /**
     * @brief Constructs a Notary.
     */
    Notary();

    /**
     * @brief Gets the instance.
     * @return The instance.
     */
    static std::shared_ptr<Notary> GetInstance();

    /**
     * @brief Gets the active in hardfork.
     * @return The active in hardfork.
     */
    uint32_t GetActiveInHardfork() const;

    /**
     * @brief Initializes the contract.
     * @param engine The engine.
     * @param hardfork The hardfork version.
     * @return True if successful, false otherwise.
     */
    bool InitializeContract(neo::smartcontract::ApplicationEngine& engine, uint32_t hardfork);

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
     * @brief Gets the maximum NotValidBefore delta.
     * @param snapshot The snapshot.
     * @return The maximum NotValidBefore delta.
     */
    uint32_t GetMaxNotValidBeforeDelta(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Sets the maximum NotValidBefore delta.
     * @param engine The engine.
     * @param value The value.
     * @return True if successful, false otherwise.
     */
    bool SetMaxNotValidBeforeDelta(neo::smartcontract::ApplicationEngine& engine, uint32_t value);

    /**
     * @brief Gets the expiration of the deposit for the specified account.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return The expiration.
     */
    uint32_t ExpirationOf(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const;

    /**
     * @brief Gets the balance of the deposit for the specified account.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return The balance.
     */
    int64_t BalanceOf(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const;

    /**
     * @brief Locks the deposit until the specified height.
     * @param engine The engine.
     * @param account The account.
     * @param till The height.
     * @return True if successful, false otherwise.
     */
    bool LockDepositUntil(ApplicationEngine& engine, const io::UInt160& account, uint32_t till);

    /**
     * @brief Withdraws the deposit.
     * @param engine The engine.
     * @param from The from account.
     * @param to The to account.
     * @return True if successful, false otherwise.
     */
    bool Withdraw(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to);

    /**
     * @brief Verifies the signature.
     * @param engine The engine.
     * @param signature The signature.
     * @return True if successful, false otherwise.
     */
    bool Verify(ApplicationEngine& engine, const io::ByteVector& signature);

   protected:
    /**
     * @brief Initializes the contract.
     */
    void Initialize() override;

    /**
     * @brief Called when a NEP-17 payment is received.
     * @param engine The engine.
     * @param from The from account.
     * @param amount The amount.
     * @param data The data.
     */
    void OnNEP17Payment(ApplicationEngine& engine, const io::UInt160& from, int64_t amount,
                        std::shared_ptr<vm::StackItem> data);

    // Method adapters for RegisterMethod calls
    std::shared_ptr<vm::StackItem> OnGetMaxNotValidBeforeDelta(ApplicationEngine& engine,
                                                               const std::vector<std::shared_ptr<vm::StackItem>>& args);
    std::shared_ptr<vm::StackItem> OnSetMaxNotValidBeforeDelta(ApplicationEngine& engine,
                                                               const std::vector<std::shared_ptr<vm::StackItem>>& args);
    std::shared_ptr<vm::StackItem> OnExpirationOf(ApplicationEngine& engine,
                                                  const std::vector<std::shared_ptr<vm::StackItem>>& args);
    std::shared_ptr<vm::StackItem> OnBalanceOf(ApplicationEngine& engine,
                                               const std::vector<std::shared_ptr<vm::StackItem>>& args);
    std::shared_ptr<vm::StackItem> OnLockDepositUntil(ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args);
    std::shared_ptr<vm::StackItem> OnWithdraw(ApplicationEngine& engine,
                                              const std::vector<std::shared_ptr<vm::StackItem>>& args);
    std::shared_ptr<vm::StackItem> OnVerify(ApplicationEngine& engine,
                                            const std::vector<std::shared_ptr<vm::StackItem>>& args);
    std::shared_ptr<vm::StackItem> OnNEP17PaymentAdapter(ApplicationEngine& engine,
                                                         const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Gets the notary nodes.
     * @param snapshot The snapshot.
     * @return The notary nodes.
     */
    std::vector<cryptography::ecc::ECPoint> GetNotaryNodes(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Calculates the notary reward.
     * @param snapshot The snapshot.
     * @param nFees The number of fees.
     * @param nNotaries The number of notaries.
     * @return The notary reward.
     */
    int64_t CalculateNotaryReward(std::shared_ptr<persistence::StoreView> snapshot, int64_t nFees, int nNotaries) const;

    /**
     * @brief Gets the deposit for the specified account.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return The deposit.
     */
    std::shared_ptr<Deposit> GetDepositFor(std::shared_ptr<persistence::StoreView> snapshot,
                                           const io::UInt160& account) const;

    /**
     * @brief Puts the deposit for the specified account.
     * @param engine The engine.
     * @param account The account.
     * @param deposit The deposit.
     */
    void PutDepositFor(ApplicationEngine& engine, const io::UInt160& account, std::shared_ptr<Deposit> deposit);

    /**
     * @brief Removes the deposit for the specified account.
     * @param snapshot The snapshot.
     * @param account The account.
     */
    void RemoveDepositFor(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account);

   public:
    /**
     * @brief The deposit class.
     */
    class Deposit : public vm::IInteroperable
    {
       public:
        /**
         * @brief The amount.
         */
        int64_t Amount;

        /**
         * @brief The till height.
         */
        uint32_t Till;

        /**
         * @brief Constructs a Deposit.
         */
        Deposit();

        /**
         * @brief Constructs a Deposit.
         * @param amount The amount.
         * @param till The till height.
         */
        Deposit(int64_t amount, uint32_t till);

        /**
         * @brief Deserializes the object from a stack item.
         * @param stackItem The stack item.
         */
        void FromStackItem(std::shared_ptr<vm::StackItem> stackItem) override;

        /**
         * @brief Serializes the object to a stack item.
         * @param referenceCounter The reference counter.
         * @return The stack item.
         */
        std::shared_ptr<vm::StackItem> ToStackItem(vm::IReferenceCounter* referenceCounter) override;
    };
};
}  // namespace neo::smartcontract::native
