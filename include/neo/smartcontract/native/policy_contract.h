#pragma once

#include <memory>
#include <neo/smartcontract/native/native_contract.h>
#include <string>

namespace neo::smartcontract::native
{
/**
 * @brief Represents the policy native contract.
 */
class PolicyContract : public NativeContract
{
  public:
    /**
     * @brief The contract ID.
     */
    static constexpr int32_t ID = -7;

    /**
     * @brief The contract name.
     */
    static constexpr const char* NAME = "PolicyContract";

    /**
     * @brief The storage prefix for blocked accounts.
     */
    static constexpr uint8_t PREFIX_BLOCKED_ACCOUNT = 15;

    /**
     * @brief The storage prefix for fee per byte.
     */
    static constexpr uint8_t PREFIX_FEE_PER_BYTE = 10;

    /**
     * @brief The storage prefix for execution fee factor.
     */
    static constexpr uint8_t PREFIX_EXEC_FEE_FACTOR = 18;

    /**
     * @brief The storage prefix for storage price.
     */
    static constexpr uint8_t PREFIX_STORAGE_PRICE = 19;

    /**
     * @brief The storage prefix for attribute fee.
     */
    static constexpr uint8_t PREFIX_ATTRIBUTE_FEE = 20;

    /**
     * @brief The storage prefix for milliseconds per block.
     */
    static constexpr uint8_t PREFIX_MILLISECONDS_PER_BLOCK = 21;

    /**
     * @brief The storage prefix for max valid until block increment.
     */
    static constexpr uint8_t PREFIX_MAX_VALID_UNTIL_BLOCK_INCREMENT = 22;

    /**
     * @brief The storage prefix for max traceable blocks.
     */
    static constexpr uint8_t PREFIX_MAX_TRACEABLE_BLOCKS = 23;

    /**
     * @brief The default fee per byte.
     * In the unit of datoshi, 1 datoshi = 1e-8 GAS
     */
    static constexpr uint32_t DEFAULT_FEE_PER_BYTE = 1000;

    /**
     * @brief The default execution fee factor.
     */
    static constexpr uint32_t DEFAULT_EXEC_FEE_FACTOR = 30;

    /**
     * @brief The default storage price.
     */
    static constexpr uint32_t DEFAULT_STORAGE_PRICE = 100000;

    /**
     * @brief The default attribute fee.
     */
    static constexpr uint32_t DEFAULT_ATTRIBUTE_FEE = 0;

    /**
     * @brief The default notary assisted attribute fee.
     */
    static constexpr uint32_t DEFAULT_NOTARY_ASSISTED_ATTRIBUTE_FEE = 1000'0000;

    /**
     * @brief The event name for the block generation time changed.
     */
    static constexpr const char* MILLISECONDS_PER_BLOCK_CHANGED_EVENT = "MillisecondsPerBlockChanged";

    /**
     * @brief The maximum execution fee factor that the committee can set.
     */
    static constexpr uint32_t MAX_EXEC_FEE_FACTOR = 100;

    /**
     * @brief The maximum attribute fee that the committee can set.
     */
    static constexpr uint32_t MAX_ATTRIBUTE_FEE = 10'0000'0000;

    /**
     * @brief The maximum storage price that the committee can set.
     */
    static constexpr uint32_t MAX_STORAGE_PRICE = 10000000;

    /**
     * @brief The maximum block generation time that the committee can set in milliseconds.
     */
    static constexpr uint32_t MAX_MILLISECONDS_PER_BLOCK = 30000;

    /**
     * @brief The maximum max valid until block increment that the committee can set.
     */
    static constexpr uint32_t MAX_MAX_VALID_UNTIL_BLOCK_INCREMENT = 86400;

    /**
     * @brief The maximum MaxTraceableBlocks value that the committee can set.
     * It is set to be a year of 15-second blocks.
     */
    static constexpr uint32_t MAX_MAX_TRACEABLE_BLOCKS = 2102400;

    /**
     * @brief Constructs a PolicyContract.
     */
    PolicyContract();

    /**
     * @brief Gets the fee per byte.
     * @param snapshot The snapshot.
     * @return The fee per byte.
     */
    int64_t GetFeePerByte(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Gets the execution fee factor. This is a multiplier that can be adjusted by the committee to adjust the
     * system fees for transactions.
     * @param snapshot The snapshot.
     * @return The execution fee factor.
     */
    uint32_t GetExecFeeFactor(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Gets the storage price.
     * @param snapshot The snapshot.
     * @return The storage price.
     */
    uint32_t GetStoragePrice(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Checks if an account is blocked.
     * @param snapshot The snapshot.
     * @param account The account.
     * @return True if the account is blocked, false otherwise.
     */
    bool IsBlocked(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const;

    /**
     * @brief Gets the attribute fee.
     * @param snapshot The snapshot.
     * @param attributeType The attribute type.
     * @return The attribute fee.
     */
    uint32_t GetAttributeFee(std::shared_ptr<persistence::StoreView> snapshot, uint8_t attributeType) const;

    /**
     * @brief Gets the milliseconds per block.
     * @param snapshot The snapshot.
     * @return The milliseconds per block.
     */
    uint32_t GetMillisecondsPerBlock(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Gets the max valid until block increment.
     * @param snapshot The snapshot.
     * @return The max valid until block increment.
     */
    uint32_t GetMaxValidUntilBlockIncrement(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Gets the max traceable blocks.
     * @param snapshot The snapshot.
     * @return The max traceable blocks.
     */
    uint32_t GetMaxTraceableBlocks(std::shared_ptr<persistence::StoreView> snapshot) const;

    /**
     * @brief Gets the instance.
     * @return The instance.
     */
    static std::shared_ptr<PolicyContract> GetInstance();

    /**
     * @brief Initializes the contract.
     * @param engine The engine.
     * @param hardfork The hardfork version.
     * @return True if successful, false otherwise.
     */
    bool InitializeContract(ApplicationEngine& engine, uint32_t hardfork);

    /**
     * @brief Handles the OnPersist event.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    bool OnPersist(ApplicationEngine& engine);

    /**
     * @brief Handles the PostPersist event.
     * @param engine The engine.
     * @return True if successful, false otherwise.
     */
    bool PostPersist(ApplicationEngine& engine);

  protected:
    /**
     * @brief Initializes the contract.
     */
    void Initialize() override;

  private:
    /**
     * @brief Handles the getFeePerByte method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetFeePerByte(ApplicationEngine& engine,
                                                   const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setFeePerByte method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetFeePerByte(ApplicationEngine& engine,
                                                   const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getExecFeeFactor method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetExecFeeFactor(ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setExecFeeFactor method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetExecFeeFactor(ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getStoragePrice method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetStoragePrice(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setStoragePrice method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetStoragePrice(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the isBlocked method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnIsBlocked(ApplicationEngine& engine,
                                               const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the blockAccount method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnBlockAccount(ApplicationEngine& engine,
                                                  const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the unblockAccount method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnUnblockAccount(ApplicationEngine& engine,
                                                    const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getAttributeFee method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetAttributeFee(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setAttributeFee method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetAttributeFee(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getMillisecondsPerBlock method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetMillisecondsPerBlock(ApplicationEngine& engine,
                                                             const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setMillisecondsPerBlock method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetMillisecondsPerBlock(ApplicationEngine& engine,
                                                             const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getMaxValidUntilBlockIncrement method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem>
    OnGetMaxValidUntilBlockIncrement(ApplicationEngine& engine,
                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setMaxValidUntilBlockIncrement method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem>
    OnSetMaxValidUntilBlockIncrement(ApplicationEngine& engine,
                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the getMaxTraceableBlocks method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetMaxTraceableBlocks(ApplicationEngine& engine,
                                                           const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the setMaxTraceableBlocks method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnSetMaxTraceableBlocks(ApplicationEngine& engine,
                                                           const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Checks if the committee witness is present.
     * @param engine The engine.
     * @return True if the committee witness is present, false otherwise.
     */
    bool CheckCommittee(ApplicationEngine& engine) const;
    
    /**
     * @brief Checks if the given script hash belongs to a native contract.
     * @param scriptHash The script hash to check.
     * @return True if it's a native contract, false otherwise.
     */
    bool IsNativeContract(const io::UInt160& scriptHash) const;
};
}  // namespace neo::smartcontract::native
