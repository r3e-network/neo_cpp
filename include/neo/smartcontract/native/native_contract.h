#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/persistence/data_cache.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/store_view.h>
#include <neo/smartcontract/call_flags.h>
#include <neo/vm/stack_item.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::smartcontract
{
// Forward declaration
class ApplicationEngine;
}  // namespace neo::smartcontract

namespace neo::smartcontract::native
{
/**
 * @brief Represents a native contract.
 */
class NativeContract
{
   public:
    /**
     * @brief Constructs a NativeContract.
     * @param name The name.
     * @param id The ID.
     */
    NativeContract(const std::string& name, uint32_t id);

    /**
     * @brief Destructor.
     */
    virtual ~NativeContract() = default;

    /**
     * @brief Gets the name.
     * @return The name.
     */
    const std::string& GetName() const;

    /**
     * @brief Gets the ID.
     * @return The ID.
     */
    uint32_t GetId() const;

    /**
     * @brief Gets the script hash.
     * @return The script hash.
     */
    const io::UInt160& GetScriptHash() const;

    /**
     * @brief Gets the methods.
     * @return The methods.
     */
    const std::unordered_map<
        std::string,
        std::pair<CallFlags, std::function<std::shared_ptr<vm::StackItem>(
                                 ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>>>&
    GetMethods() const;

    /**
     * @brief Invokes a method.
     * @param engine The engine.
     * @param method The method.
     * @param args The arguments.
     * @param callFlags The call flags.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> Invoke(ApplicationEngine& engine, const std::string& method,
                                          const std::vector<std::shared_ptr<vm::StackItem>>& args, CallFlags callFlags);

    /**
     * @brief Calls a method on the contract (alias for Invoke).
     * @param engine The engine.
     * @param method The method.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> Call(ApplicationEngine& engine, const std::string& method,
                                        const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Checks if the contract has the specified call flags.
     * @param method The method.
     * @param callFlags The call flags.
     * @return True if the contract has the specified call flags, false otherwise.
     */
    bool CheckCallFlags(const std::string& method, CallFlags callFlags) const;

    /**
     * @brief Gets the storage prefix.
     * @return The storage prefix.
     */
    virtual io::ByteVector GetStoragePrefix() const;

    /**
     * @brief Gets the storage key.
     * @param prefix The prefix.
     * @param key The key.
     * @return The storage key.
     */
    io::ByteVector GetStorageKey(uint8_t prefix, const io::ByteVector& key) const;

    /**
     * @brief Gets the storage key.
     * @param prefix The prefix.
     * @param key The key.
     * @return The storage key.
     */
    io::ByteVector GetStorageKey(uint8_t prefix, const io::UInt160& key) const;

    /**
     * @brief Gets the storage key.
     * @param prefix The prefix.
     * @param key The key.
     * @return The storage key.
     */
    io::ByteVector GetStorageKey(uint8_t prefix, const io::UInt256& key) const;

    /**
     * @brief Gets the storage key.
     * @param prefix The prefix.
     * @param key The key.
     * @return The storage key.
     */
    io::ByteVector GetStorageKey(uint8_t prefix, const std::string& key) const;

    /**
     * @brief Gets the storage value.
     * @param snapshot The snapshot.
     * @param key The key.
     * @return The value.
     */
    io::ByteVector GetStorageValue(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& key) const;

    /**
     * @brief Puts the storage value.
     * @param snapshot The snapshot.
     * @param key The key.
     * @param value The value.
     */
    void PutStorageValue(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& key,
                         const io::ByteVector& value) const;

    /**
     * @brief Deletes the storage value.
     * @param snapshot The snapshot.
     * @param key The key.
     */
    void DeleteStorageValue(std::shared_ptr<persistence::StoreView> snapshot, const io::ByteVector& key) const;

    /**
     * @brief Creates a storage key with the specified prefix.
     * @param prefix The prefix.
     * @return The storage key.
     */
    persistence::StorageKey CreateStorageKey(uint8_t prefix) const;

    /**
     * @brief Creates a storage key with the specified prefix and key.
     * @param prefix The prefix.
     * @param key The key.
     * @return The storage key.
     */
    persistence::StorageKey CreateStorageKey(uint8_t prefix, const io::ByteVector& key) const;

    /**
     * @brief Creates a storage key with the specified prefix and key.
     * @param prefix The prefix.
     * @param key The key.
     * @return The storage key.
     */
    persistence::StorageKey CreateStorageKey(uint8_t prefix, const io::UInt160& key) const;

    /**
     * @brief Creates a storage key with the specified prefix and key.
     * @param prefix The prefix.
     * @param key The key.
     * @return The storage key.
     */
    persistence::StorageKey CreateStorageKey(uint8_t prefix, const io::UInt256& key) const;

    /**
     * @brief Creates a storage key.
     * @param prefix The prefix.
     * @param key The key (uint32_t).
     * @return The storage key.
     */
    persistence::StorageKey CreateStorageKey(uint8_t prefix, uint32_t key) const;

    /**
     * @brief Initializes the contract.
     */
    virtual void Initialize() = 0;

   protected:
    /**
     * @brief Registers a method.
     * @param name The name.
     * @param callFlags The call flags.
     * @param handler The handler.
     */
    void RegisterMethod(const std::string& name, CallFlags callFlags,
                        std::function<std::shared_ptr<vm::StackItem>(
                            ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>
                            handler);

   private:
    std::string name_;
    uint32_t id_;
    io::UInt160 scriptHash_;
    std::unordered_map<
        std::string, std::pair<CallFlags, std::function<std::shared_ptr<vm::StackItem>(
                                              ApplicationEngine&, const std::vector<std::shared_ptr<vm::StackItem>>&)>>>
        methods_;
};
}  // namespace neo::smartcontract::native
