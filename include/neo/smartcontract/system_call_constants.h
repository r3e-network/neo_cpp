/**
 * @file system_call_constants.h
 * @brief Constant definitions
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>

namespace neo::smartcontract
{
/**
 * @brief Constants for system call gas costs.
 *
 * These constants define the gas costs for various system calls.
 * They are used to ensure consistent gas accounting across the system.
 */
namespace gas_cost
{
// Base gas costs
constexpr int64_t VeryLow = 1 << 4;    // 16 gas
constexpr int64_t Low = 1 << 8;        // 256 gas
constexpr int64_t Medium = 1 << 10;    // 1,024 gas
constexpr int64_t High = 1 << 15;      // 32,768 gas
constexpr int64_t VeryHigh = 1 << 20;  // 1,048,576 gas

// Runtime gas costs
constexpr int64_t GetTrigger = VeryLow;
constexpr int64_t CheckWitness = Medium;
constexpr int64_t Notify = 1 << 9;  // 512 gas
constexpr int64_t Log = 1 << 9;     // 512 gas
constexpr int64_t GetTime = VeryLow;
constexpr int64_t GetPlatform = VeryLow;
constexpr int64_t GetNetwork = VeryLow;
constexpr int64_t GetRandom = VeryLow;
constexpr int64_t GasLeft = VeryLow;
constexpr int64_t GetInvocationCounter = VeryLow;
constexpr int64_t GetScriptContainer = VeryLow;
constexpr int64_t GetExecutingScriptHash = VeryLow;
constexpr int64_t GetCallingScriptHash = VeryLow;
constexpr int64_t GetEntryScriptHash = VeryLow;

// Storage gas costs
constexpr int64_t StorageGet = High;
constexpr int64_t StoragePut = High;
constexpr int64_t StorageDelete = High;
constexpr int64_t StorageFind = High;
constexpr int64_t IteratorNext = High;
constexpr int64_t IteratorValue = VeryLow;

// Contract gas costs
constexpr int64_t Call = High;
constexpr int64_t GetCallFlags = VeryLow;
constexpr int64_t CallNative = 0;
constexpr int64_t CreateStandardAccount = Medium;
constexpr int64_t CreateMultisigAccount = Medium;

// Crypto gas costs
constexpr int64_t VerifySignature = High;
constexpr int64_t CheckSig = High;
constexpr int64_t Hash160 = High;
constexpr int64_t Hash256 = High;

// JSON gas costs
constexpr int64_t JsonSerialize = High;
constexpr int64_t JsonDeserialize = High;
}  // namespace gas_cost

/**
 * @brief Constants for system call names.
 *
 * These constants define the names of various system calls.
 * They are used to ensure consistent naming across the system.
 */
namespace system_call
{
// Runtime system calls
constexpr const char* GetTrigger = "System.Runtime.GetTrigger";
constexpr const char* CheckWitness = "System.Runtime.CheckWitness";
constexpr const char* Notify = "System.Runtime.Notify";
constexpr const char* Log = "System.Runtime.Log";
constexpr const char* GetTime = "System.Runtime.GetTime";
constexpr const char* GetPlatform = "System.Runtime.GetPlatform";
constexpr const char* GetNetwork = "System.Runtime.GetNetwork";
constexpr const char* GetRandom = "System.Runtime.GetRandom";
constexpr const char* GasLeft = "System.Runtime.GasLeft";
constexpr const char* GetInvocationCounter = "System.Runtime.GetInvocationCounter";
constexpr const char* GetScriptContainer = "System.Runtime.GetScriptContainer";
constexpr const char* GetExecutingScriptHash = "System.Runtime.GetExecutingScriptHash";
constexpr const char* GetCallingScriptHash = "System.Runtime.GetCallingScriptHash";
constexpr const char* GetEntryScriptHash = "System.Runtime.GetEntryScriptHash";

// Storage system calls
constexpr const char* StorageGet = "System.Storage.Get";
constexpr const char* StoragePut = "System.Storage.Put";
constexpr const char* StorageDelete = "System.Storage.Delete";
constexpr const char* StorageFind = "System.Storage.Find";
constexpr const char* IteratorNext = "System.Iterator.Next";
constexpr const char* IteratorValue = "System.Iterator.Value";

// Contract system calls
constexpr const char* Call = "System.Contract.Call";
constexpr const char* GetCallFlags = "System.Contract.GetCallFlags";
constexpr const char* CallNative = "System.Contract.CallNative";
constexpr const char* CreateStandardAccount = "System.Contract.CreateStandardAccount";
constexpr const char* CreateMultisigAccount = "System.Contract.CreateMultisigAccount";

// Crypto system calls
constexpr const char* VerifySignature = "System.Crypto.VerifySignature";
constexpr const char* CheckSig = "System.Crypto.CheckSig";
constexpr const char* Hash160 = "System.Crypto.Hash160";
constexpr const char* Hash256 = "System.Crypto.Hash256";

// JSON system calls
constexpr const char* JsonSerialize = "System.Json.Serialize";
constexpr const char* JsonDeserialize = "System.Json.Deserialize";
}  // namespace system_call
}  // namespace neo::smartcontract
