#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <neo/io/byte_vector.h>
#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/data_cache.h>
#include <neo/protocol_settings.h>
#include <neo/smartcontract/call_flags.h>
#include <neo/smartcontract/contract.h>
#include <neo/smartcontract/system_call_descriptor.h>
#include <neo/smartcontract/trigger_type.h>
#include <neo/smartcontract/vm_types.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
namespace neo::smartcontract::native
{
class NativeContract;
}

namespace neo::smartcontract
{
// Forward declarations
class Script;

/**
 * @brief Represents an application engine.
 */
class ApplicationEngine : public vm::ExecutionEngine
{
  public:
    /**
     * @brief The maximum gas that can be spent when a contract is executed in test mode.
     */
    static constexpr int64_t TestModeGas = 20'00000000;

    /**
     * @brief Constructs an ApplicationEngine.
     * @param trigger The trigger type.
     * @param container The container.
     * @param snapshot The snapshot.
     * @param persistingBlock The persisting block.
     * @param gas The gas.
     */
    ApplicationEngine(TriggerType trigger, const io::ISerializable* container,
                      std::shared_ptr<persistence::DataCache> snapshot, const ledger::Block* persistingBlock = nullptr,
                      int64_t gas = TestModeGas);

    /**
     * @brief Destructor.
     */
    ~ApplicationEngine() = default;

    /**
     * @brief Gets the VM state.
     * @return The VM state.
     */
    neo::vm::VMState GetState() const
    {
        return state_;
    }

    /**
     * @brief Gets the gas consumed.
     * @return The gas consumed.
     */
    int64_t GetGasConsumed() const
    {
        return gas_consumed_;
    }

    /**
     * @brief Gets the gas remaining.
     * @return The gas remaining.
     */
    int64_t GetGasLeft() const
    {
        return gas_limit_ - gas_consumed_;
    }

    /**
     * @brief Gets the trigger type.
     * @return The trigger type.
     */
    TriggerType GetTrigger() const
    {
        return trigger_;
    }

    /**
     * @brief Gets the log entries.
     * @return The log entries.
     */
    const std::vector<LogEntry>& GetLogs() const
    {
        return logs_;
    }

    /**
     * @brief Gets the notification entries.
     * @return The notification entries.
     */
    const std::vector<NotifyEntry>& GetNotifications() const
    {
        return notifications_;
    }

    /**
     * @brief Gets the container.
     * @return The container.
     */
    const io::ISerializable* GetContainer() const;

    /**
     * @brief Gets the script container.
     * @return The script container.
     */
    const io::ISerializable* GetScriptContainer() const;

    /**
     * @brief Gets the snapshot.
     * @return The snapshot.
     */
    std::shared_ptr<persistence::DataCache> GetSnapshot() const;

    /**
     * @brief Gets the persisting block.
     * @return The persisting block.
     */
    const ledger::Block* GetPersistingBlock() const;

    /**
     * @brief Gets the current script hash.
     * @return The current script hash.
     */
    io::UInt160 GetCurrentScriptHash() const;

    /**
     * @brief Sets the current script hash context for testing purposes.
     * @param scriptHash The script hash to set as current context.
     */
    void SetCurrentScriptHash(const io::UInt160& scriptHash);

    /**
     * @brief Gets the calling script hash.
     * @return The calling script hash.
     */
    io::UInt160 GetCallingScriptHash() const;

    /**
     * @brief Gets the entry script hash.
     * @return The entry script hash.
     */
    io::UInt160 GetEntryScriptHash() const;

    /**
     * @brief Executes a script.
     * @param script The script to execute.
     * @return The VM state after execution.
     */
    neo::vm::VMState Execute(const std::vector<uint8_t>& script);

    /**
     * @brief Executes the loaded script.
     * @return The VM state after execution.
     */
    neo::vm::VMState Execute();

    /**
     * @brief Loads a script.
     * @param script The script to load.
     */
    void LoadScript(const std::vector<uint8_t>& script);

    /**
     * @brief Adds a log entry.
     * @param entry The log entry.
     */
    void AddLog(const LogEntry& entry)
    {
        logs_.push_back(entry);
    }

    /**
     * @brief Adds a notification entry.
     * @param entry The notification entry.
     */
    void AddNotification(const NotifyEntry& entry)
    {
        notifications_.push_back(entry);
    }

    /**
     * @brief Checks if the engine has flag.
     * @param flag The flag.
     * @return True if the engine has the flag, false otherwise.
     */
    bool HasFlag(CallFlags flag) const;

    /**
     * @brief Adds gas.
     * @param gas The gas.
     */
    void AddGas(int64_t gas);

    /**
     * @brief Checks if the witness is valid.
     * @param hash The hash.
     * @return True if the witness is valid, false otherwise.
     */
    bool CheckWitness(const io::UInt160& hash) const;

    /**
     * @brief Checks if the witness is valid.
     * @param hash The hash.
     * @return True if the witness is valid, false otherwise.
     */
    bool CheckWitness(const io::UInt256& hash) const;

    /**
     * @brief Internal witness checking method.
     * @param hash The hash.
     * @return True if the witness is valid, false otherwise.
     */
    bool CheckWitnessInternal(const io::UInt160& hash) const;

    /**
     * @brief Creates a contract.
     * @param script The script.
     * @param manifest The manifest.
     * @param offset The offset.
     * @return The contract state.
     */
    ContractState CreateContract(const io::ByteVector& script, const std::string& manifest, uint32_t offset);

    /**
     * @brief Calls a contract.
     * @param scriptHash The script hash.
     * @param method The method.
     * @param args The arguments.
     * @param flags The flags.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> CallContract(const io::UInt160& scriptHash, const std::string& method,
                                                const std::vector<std::shared_ptr<vm::StackItem>>& args,
                                                CallFlags flags);

    /**
     * @brief Logs a message.
     * @param message The message to log.
     */
    void Log(const std::string& message);

    /**
     * @brief Notifies an event.
     * @param scriptHash The script hash.
     * @param eventName The event name.
     * @param state The state.
     */
    void Notify(const io::UInt160& scriptHash, const std::string& eventName,
                const std::vector<std::shared_ptr<vm::StackItem>>& state);

    /**
     * @brief Gets the transaction that is being executed.
     * @return The transaction.
     */
    const ledger::Transaction* GetTransaction() const;

    /**
     * @brief Gets the invocation gas price.
     * @return The gas price.
     */
    int64_t GetGasPrice() const;

    /**
     * @brief Gets the platform version.
     * @return The platform version.
     */
    uint32_t GetPlatformVersion() const;

    /**
     * @brief Gets the random number.
     * @return The random number.
     */
    uint64_t GetRandom() const;

    /**
     * @brief Gets the network fee per byte.
     * @return The network fee per byte.
     */
    int64_t GetNetworkFeePerByte() const;

    /**
     * @brief Gets the execution limits.
     * @return The execution limits.
     */
    vm::ExecutionEngineLimits GetLimits() const;

    /**
     * @brief Gets the protocol settings.
     * @return The protocol settings.
     */
    const ProtocolSettings* GetProtocolSettings() const;

    /**
     * @brief Gets the current block height.
     * @return The current block height.
     */
    uint32_t GetCurrentBlockHeight() const;

    /**
     * @brief Checks if a hardfork is enabled.
     * @param hardfork The hardfork to check.
     * @return True if the hardfork is enabled, false otherwise.
     */
    bool IsHardforkEnabled(int hardfork) const;

    /**
     * @brief Gets a native contract by its hash.
     * @param hash The contract hash.
     * @return The native contract, or nullptr if not found.
     */
    native::NativeContract* GetNativeContract(const io::UInt160& hash) const;

    /**
     * @brief Creates a new instance of the ApplicationEngine class.
     * @param trigger The trigger type.
     * @param container The container.
     * @param snapshot The snapshot.
     * @param persistingBlock The persisting block.
     * @param gas The gas.
     * @return The engine instance created.
     */
    static std::unique_ptr<ApplicationEngine> Create(TriggerType trigger, const io::ISerializable* container,
                                                     std::shared_ptr<persistence::DataCache> snapshot,
                                                     const ledger::Block* persistingBlock = nullptr,
                                                     int64_t gas = TestModeGas);

    /**
     * @brief Runs a script.
     * @param script The script.
     * @param snapshot The snapshot.
     * @param container The container.
     * @param persistingBlock The persisting block.
     * @param offset The offset.
     * @param gas The gas.
     * @return The engine instance created.
     */
    static std::unique_ptr<ApplicationEngine> Run(const io::ByteVector& script,
                                                  std::shared_ptr<persistence::DataCache> snapshot,
                                                  const io::ISerializable* container = nullptr,
                                                  const ledger::Block* persistingBlock = nullptr, int32_t offset = 0,
                                                  int64_t gas = TestModeGas);

    /**
     * @brief Gets the current call flags.
     * @return The current call flags.
     */
    CallFlags GetCallFlags() const
    {
        return flags_;
    }

    /**
     * @brief Gets the contracts map for system call implementations.
     * @return Reference to the contracts map.
     */
    const std::unordered_map<io::UInt160, std::unordered_map<std::string, std::function<bool(ApplicationEngine&)>>>&
    GetContracts() const
    {
        return contracts_;
    }

    /**
     * @brief Gets mutable access to the contracts map for system call implementations.
     * @return Reference to the contracts map.
     */
    std::unordered_map<io::UInt160, std::unordered_map<std::string, std::function<bool(ApplicationEngine&)>>>&
    GetContracts()
    {
        return contracts_;
    }

    /**
     * @brief Sets the call flags.
     * @param flags The new call flags.
     */
    void SetCallFlags(CallFlags flags)
    {
        flags_ = flags;
    }

    /**
     * @brief Pops an item from the evaluation stack.
     * @return The popped stack item.
     */
    std::shared_ptr<vm::StackItem> Pop();

    /**
     * @brief Pushes an item onto the evaluation stack.
     * @param item The item to push.
     */
    void Push(std::shared_ptr<vm::StackItem> item);

    /**
     * @brief Gets the top item from the evaluation stack without removing it.
     * @return The top stack item.
     */
    std::shared_ptr<vm::StackItem> Peek() const;

    /**
     * @brief Gets the script being executed.
     * @return The script bytes.
     */
    io::ByteVector GetScript() const;

    /**
     * @brief Gets the exception message if execution failed.
     * @return The exception message.
     */
    std::string GetException() const;

    /**
     * @brief Gets the result stack items.
     * @return The result stack.
     */
    std::vector<std::shared_ptr<vm::StackItem>> GetResultStack() const;

    /**
     * @brief Gets the network magic value.
     * @return The network magic value.
     */
    uint32_t GetNetworkMagic() const;

    /**
     * @brief Gets the invocation count for a script.
     * @param scriptHash The script hash.
     * @return The invocation count.
     */
    int64_t GetInvocationCount(const io::UInt160& scriptHash) const;

    /**
     * @brief Sets the invocation count for a script.
     * @param scriptHash The script hash.
     * @param count The invocation count.
     */
    void SetInvocationCount(const io::UInt160& scriptHash, int64_t count);

    // These members are made protected to allow access from system call implementations
  protected:
    std::unordered_map<io::UInt160, std::unordered_map<std::string, std::function<bool(ApplicationEngine&)>>>
        contracts_;
    CallFlags flags_;

  public:
    /**
     * @brief Registers a system call.
     * @param name The name of the system call.
     * @param handler The handler function.
     * @param gasCost The gas cost of the system call.
     * @param requiredFlags The required call flags.
     */
    void RegisterSystemCall(const std::string& name, std::function<bool(vm::ExecutionEngine&)> handler,
                            int64_t gasCost = 0, CallFlags requiredFlags = CallFlags::None);

  private:
    TriggerType trigger_;
    const io::ISerializable* container_;
    std::shared_ptr<persistence::DataCache> snapshot_;
    const ledger::Block* persisting_block_;
    const ledger::Block* persistingBlock_;  // Alternative naming for compatibility
    int64_t gas_limit_;
    int64_t gas_consumed_;
    int64_t gasConsumed_;  // Alternative naming for compatibility
    int64_t gasLeft_;
    neo::vm::VMState state_;
    std::vector<LogEntry> logs_;
    std::vector<NotifyEntry> notifications_;
    int64_t gasPrice_ = 1000;
    uint32_t platformVersion_ = 0;
    uint64_t random_ = 0;
    int64_t networkFeePerByte_ = 1000;
    ProtocolSettings protocolSettings_;
    std::string exception_;
    std::vector<io::UInt160> scriptHashes_;                      // Stack of script hashes for context tracking
    std::unordered_map<io::UInt160, int64_t> invocationCounts_;  // Track invocation counts per script

    std::unordered_map<std::string, SystemCallDescriptor> systemCalls_;

    /**
     * @brief Registers all system calls.
     */
    void RegisterSystemCalls();

    // Helper methods for witness verification
    bool IsCalledByEntry() const;
    bool IsInAllowedContracts(const ledger::Signer& signer, const io::UInt160& calling_script) const;
    bool IsInAllowedGroups(const ledger::Signer& signer, const io::UInt160& calling_script) const;
    bool IsCommitteeHash(const io::UInt256& hash) const;
    bool VerifyCommitteeConsensus(const io::UInt256& hash) const;
    bool VerifyMultiSignatureHash(const io::UInt256& hash) const;

    // Helper methods for contract operations
    std::vector<cryptography::ecc::ECPoint> GetCommittee() const;
    io::UInt160 GetScriptHashFromPublicKey(const cryptography::ecc::ECPoint& pubkey) const;
    std::shared_ptr<ContractState> GetContract(const io::UInt160& scriptHash) const;
    io::ByteVector CreateCommitteeMultiSigScript(const std::vector<cryptography::ecc::ECPoint>& committee) const;
    bool IsMultiSignatureContract(const io::ByteVector& script) const;
};
}  // namespace neo::smartcontract
