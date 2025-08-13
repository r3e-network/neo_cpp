/**
 * @file neo_account_state.h
 * @brief Neo Account State
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/native/account_state.h>
#include <neo/vm/stack_item.h>

#include <memory>

namespace neo::smartcontract::native
{
/**
 * @brief Represents the account state of the NEO token.
 */
class NeoAccountState : public AccountState
{
   public:
    /**
     * @brief Constructs a NeoAccountState.
     */
    NeoAccountState();

    /**
     * @brief Constructs a NeoAccountState with the specified balance.
     * @param balance The balance.
     */
    explicit NeoAccountState(int64_t balance);

    /**
     * @brief Destructs a NeoAccountState.
     */
    ~NeoAccountState() override;

    /**
     * @brief Gets the balance height.
     * @return The balance height.
     */
    uint32_t GetBalanceHeight() const;

    /**
     * @brief Sets the balance height.
     * @param height The balance height.
     */
    void SetBalanceHeight(uint32_t height);

    /**
     * @brief Gets the voting target.
     * @return The voting target.
     */
    const cryptography::ecc::ECPoint& GetVoteTo() const;

    /**
     * @brief Sets the voting target.
     * @param voteTo The voting target.
     */
    void SetVoteTo(const cryptography::ecc::ECPoint& voteTo);

    /**
     * @brief Gets the last gas per vote.
     * @return The last gas per vote.
     */
    int64_t GetLastGasPerVote() const;

    /**
     * @brief Sets the last gas per vote.
     * @param lastGasPerVote The last gas per vote.
     */
    void SetLastGasPerVote(int64_t lastGasPerVote);

    /**
     * @brief Deserializes the account state from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the account state to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Converts the account state to a stack item.
     * @return The stack item.
     */
    std::shared_ptr<vm::StackItem> ToStackItem() const override;

    /**
     * @brief Converts a stack item to an account state.
     * @param item The stack item.
     */
    void FromStackItem(const std::shared_ptr<vm::StackItem>& item) override;

   private:
    uint32_t balanceHeight_;
    cryptography::ecc::ECPoint voteTo_;
    int64_t lastGasPerVote_;
};
}  // namespace neo::smartcontract::native
