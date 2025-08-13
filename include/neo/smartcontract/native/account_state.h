/**
 * @file account_state.h
 * @brief Account State
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/vm/stack_item.h>

#include <cstdint>
#include <memory>

namespace neo::smartcontract::native
{
/**
 * @brief The base class of account state for all native tokens.
 * This class matches the C# AccountState class.
 */
class AccountState
{
   public:
    /**
     * @brief Constructs an AccountState.
     */
    AccountState();

    /**
     * @brief Constructs an AccountState with the specified balance.
     * @param balance The balance.
     */
    explicit AccountState(int64_t balance);

    /**
     * @brief Virtual destructor.
     */
    virtual ~AccountState() = default;

    /**
     * @brief Gets the balance of the account.
     * @return The balance.
     */
    int64_t GetBalance() const;

    /**
     * @brief Sets the balance of the account.
     * @param balance The balance.
     */
    void SetBalance(int64_t balance);

    /**
     * @brief Deserializes the account state from a binary reader.
     * @param reader The binary reader.
     */
    virtual void Deserialize(io::BinaryReader& reader);

    /**
     * @brief Serializes the account state to a binary writer.
     * @param writer The binary writer.
     */
    virtual void Serialize(io::BinaryWriter& writer) const;

    /**
     * @brief Converts the account state to a stack item.
     * @return The stack item.
     */
    virtual std::shared_ptr<vm::StackItem> ToStackItem() const;

    /**
     * @brief Converts a stack item to an account state.
     * @param item The stack item.
     */
    virtual void FromStackItem(const std::shared_ptr<vm::StackItem>& item);

   private:
    int64_t balance_;
};
}  // namespace neo::smartcontract::native
