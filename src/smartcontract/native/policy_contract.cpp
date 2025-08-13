/**
 * @file policy_contract.cpp
 * @brief Policy Contract
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/extensions/biginteger_extensions.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/name_service.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/notary.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/role_management.h>

#include <iostream>
#include <sstream>

namespace neo::smartcontract::native
{
using BigInteger = extensions::BigIntegerExtensions::BigInteger;
PolicyContract::PolicyContract() : NativeContract(NAME, ID) {}

std::shared_ptr<PolicyContract> PolicyContract::GetInstance()
{
    static std::shared_ptr<PolicyContract> instance = std::make_shared<PolicyContract>();
    return instance;
}

void PolicyContract::Initialize()
{
    RegisterMethod("getFeePerByte", CallFlags::ReadStates,
                   std::bind(&PolicyContract::OnGetFeePerByte, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("setFeePerByte", CallFlags::States,
                   std::bind(&PolicyContract::OnSetFeePerByte, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("getExecFeeFactor", CallFlags::ReadStates,
                   std::bind(&PolicyContract::OnGetExecFeeFactor, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("setExecFeeFactor", CallFlags::States,
                   std::bind(&PolicyContract::OnSetExecFeeFactor, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("getStoragePrice", CallFlags::ReadStates,
                   std::bind(&PolicyContract::OnGetStoragePrice, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("setStoragePrice", CallFlags::States,
                   std::bind(&PolicyContract::OnSetStoragePrice, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("isBlocked", CallFlags::ReadStates,
                   std::bind(&PolicyContract::OnIsBlocked, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("blockAccount", CallFlags::States,
                   std::bind(&PolicyContract::OnBlockAccount, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("unblockAccount", CallFlags::States,
                   std::bind(&PolicyContract::OnUnblockAccount, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("getAttributeFee", CallFlags::ReadStates,
                   std::bind(&PolicyContract::OnGetAttributeFee, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("setAttributeFee", CallFlags::States,
                   std::bind(&PolicyContract::OnSetAttributeFee, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod(
        "getMillisecondsPerBlock", CallFlags::ReadStates,
        std::bind(&PolicyContract::OnGetMillisecondsPerBlock, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod(
        "setMillisecondsPerBlock", CallFlags::States | CallFlags::AllowNotify,
        std::bind(&PolicyContract::OnSetMillisecondsPerBlock, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("getMaxValidUntilBlockIncrement", CallFlags::ReadStates,
                   std::bind(&PolicyContract::OnGetMaxValidUntilBlockIncrement, this, std::placeholders::_1,
                             std::placeholders::_2));
    RegisterMethod("setMaxValidUntilBlockIncrement", CallFlags::States,
                   std::bind(&PolicyContract::OnSetMaxValidUntilBlockIncrement, this, std::placeholders::_1,
                             std::placeholders::_2));
    RegisterMethod(
        "getMaxTraceableBlocks", CallFlags::ReadStates,
        std::bind(&PolicyContract::OnGetMaxTraceableBlocks, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod(
        "setMaxTraceableBlocks", CallFlags::States,
        std::bind(&PolicyContract::OnSetMaxTraceableBlocks, this, std::placeholders::_1, std::placeholders::_2));
}

bool PolicyContract::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
{
    // For base initialization (hardfork == ActiveIn)
    if (hardfork == 0)  // ActiveIn hardfork
    {
        // Initialize fee per byte (1000 datoshi)
        auto feePerByteKey = GetStorageKey(PREFIX_FEE_PER_BYTE, io::ByteVector{});
        PutStorageValue(
            engine.GetSnapshot(), feePerByteKey,
            extensions::BigIntegerExtensions::ToByteArray(BigInteger(static_cast<uint64_t>(DEFAULT_FEE_PER_BYTE))));

        // Initialize execution fee factor (30)
        auto execFeeFactorKey = GetStorageKey(PREFIX_EXEC_FEE_FACTOR, io::ByteVector{});
        PutStorageValue(
            engine.GetSnapshot(), execFeeFactorKey,
            extensions::BigIntegerExtensions::ToByteArray(BigInteger(static_cast<uint64_t>(DEFAULT_EXEC_FEE_FACTOR))));

        // Initialize storage price (100000)
        auto storagePriceKey = GetStorageKey(PREFIX_STORAGE_PRICE, io::ByteVector{});
        PutStorageValue(
            engine.GetSnapshot(), storagePriceKey,
            extensions::BigIntegerExtensions::ToByteArray(BigInteger(static_cast<uint64_t>(DEFAULT_STORAGE_PRICE))));
    }

    // For Echidna hardfork initialization
    if (hardfork == 1)  // Hardfork::HF_Echidna
    {
        // Initialize NotaryAssisted attribute fee
        io::ByteVector notaryAssistedType{0x20};  // TransactionAttributeType.NotaryAssisted
        auto notaryAssistedKey = GetStorageKey(PREFIX_ATTRIBUTE_FEE, notaryAssistedType);
        PutStorageValue(engine.GetSnapshot(), notaryAssistedKey,
                        extensions::BigIntegerExtensions::ToByteArray(
                            BigInteger(static_cast<uint64_t>(DEFAULT_NOTARY_ASSISTED_ATTRIBUTE_FEE))));

        // Initialize milliseconds per block from protocol settings
        auto millisecondsPerBlockKey = GetStorageKey(PREFIX_MILLISECONDS_PER_BLOCK, io::ByteVector{});
        auto protocolSettings = engine.GetProtocolSettings();
        uint32_t millisecondsPerBlock = protocolSettings ? protocolSettings->GetMillisecondsPerBlock() : 15000;
        PutStorageValue(
            engine.GetSnapshot(), millisecondsPerBlockKey,
            extensions::BigIntegerExtensions::ToByteArray(BigInteger(static_cast<uint64_t>(millisecondsPerBlock))));

        // Initialize max valid until block increment from protocol settings
        auto maxValidUntilBlockIncrementKey = GetStorageKey(PREFIX_MAX_VALID_UNTIL_BLOCK_INCREMENT, io::ByteVector{});
        uint32_t maxValidUntilBlockIncrement =
            protocolSettings ? protocolSettings->GetMaxValidUntilBlockIncrement() : 5760;
        PutStorageValue(engine.GetSnapshot(), maxValidUntilBlockIncrementKey,
                        extensions::BigIntegerExtensions::ToByteArray(
                            BigInteger(static_cast<uint64_t>(maxValidUntilBlockIncrement))));

        // Initialize max traceable blocks from protocol settings
        auto maxTraceableBlocksKey = GetStorageKey(PREFIX_MAX_TRACEABLE_BLOCKS, io::ByteVector{});
        uint32_t maxTraceableBlocks = protocolSettings ? protocolSettings->GetMaxTraceableBlocks() : 2102400;
        PutStorageValue(
            engine.GetSnapshot(), maxTraceableBlocksKey,
            extensions::BigIntegerExtensions::ToByteArray(BigInteger(static_cast<uint64_t>(maxTraceableBlocks))));
    }

    return true;
}

bool PolicyContract::OnPersist(ApplicationEngine& engine)
{
    // Initialize contract if needed
    auto key = GetStorageKey(PREFIX_FEE_PER_BYTE, io::ByteVector{});
    auto value = GetStorageValue(engine.GetSnapshot(), key);
    if (value.IsEmpty())
    {
        InitializeContract(engine, 0);
    }

    return true;
}

bool PolicyContract::PostPersist(ApplicationEngine& engine)
{
    // Nothing to do post persist
    return true;
}

int64_t PolicyContract::GetFeePerByte(std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto key = GetStorageKey(PREFIX_FEE_PER_BYTE, io::ByteVector{});
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty()) return DEFAULT_FEE_PER_BYTE;

    return extensions::BigIntegerExtensions::FromByteArray(value).ToInt64();
}

uint32_t PolicyContract::GetExecFeeFactor(std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto key = GetStorageKey(PREFIX_EXEC_FEE_FACTOR, io::ByteVector{});
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty()) return DEFAULT_EXEC_FEE_FACTOR;

    return static_cast<uint32_t>(extensions::BigIntegerExtensions::FromByteArray(value).ToUInt64());
}

uint32_t PolicyContract::GetStoragePrice(std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto key = GetStorageKey(PREFIX_STORAGE_PRICE, io::ByteVector{});
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty()) return DEFAULT_STORAGE_PRICE;

    return static_cast<uint32_t>(extensions::BigIntegerExtensions::FromByteArray(value).ToUInt64());
}

bool PolicyContract::IsBlocked(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const
{
    auto key = GetStorageKey(PREFIX_BLOCKED_ACCOUNT, account);
    auto value = GetStorageValue(snapshot, key);
    return !value.IsEmpty();
}

uint32_t PolicyContract::GetAttributeFee(std::shared_ptr<persistence::StoreView> snapshot, uint8_t attributeType) const
{
    auto key = GetStorageKey(PREFIX_ATTRIBUTE_FEE, io::ByteVector{&attributeType, 1});
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty())
        return attributeType == 0x20 ? DEFAULT_NOTARY_ASSISTED_ATTRIBUTE_FEE
                                     : DEFAULT_ATTRIBUTE_FEE;  // 0x20 is NotaryAssisted

    return static_cast<uint32_t>(extensions::BigIntegerExtensions::FromByteArray(value).ToUInt64());
}

uint32_t PolicyContract::GetMillisecondsPerBlock(std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto key = GetStorageKey(PREFIX_MILLISECONDS_PER_BLOCK, io::ByteVector{});
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty())
    {
        // Return from protocol settings if available
        // Note: This would require accessing engine context, which we don't have here
        // Return reasonable default value
        return 15000;
    }

    return static_cast<uint32_t>(extensions::BigIntegerExtensions::FromByteArray(value).ToUInt64());
}

uint32_t PolicyContract::GetMaxValidUntilBlockIncrement(std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto key = GetStorageKey(PREFIX_MAX_VALID_UNTIL_BLOCK_INCREMENT, io::ByteVector{});
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty()) return 5760;  // Default value

    return static_cast<uint32_t>(extensions::BigIntegerExtensions::FromByteArray(value).ToUInt64());
}

uint32_t PolicyContract::GetMaxTraceableBlocks(std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto key = GetStorageKey(PREFIX_MAX_TRACEABLE_BLOCKS, io::ByteVector{});
    auto value = GetStorageValue(snapshot, key);
    if (value.IsEmpty()) return 2102400;  // Default value

    return static_cast<uint32_t>(extensions::BigIntegerExtensions::FromByteArray(value).ToUInt64());
}

std::shared_ptr<vm::StackItem> PolicyContract::OnGetFeePerByte(ApplicationEngine& engine,
                                                               const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    return vm::StackItem::Create(GetFeePerByte(engine.GetSnapshot()));
}

std::shared_ptr<vm::StackItem> PolicyContract::OnSetFeePerByte(ApplicationEngine& engine,
                                                               const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::invalid_argument("Invalid arguments");

    auto valueItem = args[0];
    auto value = valueItem->GetInteger();

    if (value < 0 || value > 100000000)  // 1_00000000
        throw std::out_of_range("Value out of range");

    // Check committee witness
    if (!CheckCommittee(engine)) throw std::runtime_error("Invalid committee signature");

    // Set fee per byte
    auto key = GetStorageKey(PREFIX_FEE_PER_BYTE, io::ByteVector{});
    PutStorageValue(engine.GetSnapshot(), key, extensions::BigIntegerExtensions::ToByteArray(BigInteger(value)));

    return vm::StackItem::Null();
}

std::shared_ptr<vm::StackItem> PolicyContract::OnGetExecFeeFactor(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    return vm::StackItem::Create(static_cast<int64_t>(GetExecFeeFactor(engine.GetSnapshot())));
}

std::shared_ptr<vm::StackItem> PolicyContract::OnSetExecFeeFactor(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::invalid_argument("Invalid arguments");

    auto valueItem = args[0];
    auto value = valueItem->GetInteger();

    if (value == 0 || value > MAX_EXEC_FEE_FACTOR) throw std::out_of_range("Value out of range");

    // Check committee witness
    if (!CheckCommittee(engine)) throw std::runtime_error("Invalid committee signature");

    // Set execution fee factor
    auto key = GetStorageKey(PREFIX_EXEC_FEE_FACTOR, io::ByteVector{});
    PutStorageValue(engine.GetSnapshot(), key, extensions::BigIntegerExtensions::ToByteArray(BigInteger(value)));

    return vm::StackItem::Null();
}

std::shared_ptr<vm::StackItem> PolicyContract::OnGetStoragePrice(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    return vm::StackItem::Create(static_cast<int64_t>(GetStoragePrice(engine.GetSnapshot())));
}

std::shared_ptr<vm::StackItem> PolicyContract::OnSetStoragePrice(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::invalid_argument("Invalid arguments");

    auto valueItem = args[0];
    auto value = valueItem->GetInteger();

    if (value == 0 || value > MAX_STORAGE_PRICE) throw std::out_of_range("Value out of range");

    // Check committee witness
    if (!CheckCommittee(engine)) throw std::runtime_error("Invalid committee signature");

    // Set storage price
    auto key = GetStorageKey(PREFIX_STORAGE_PRICE, io::ByteVector{});
    PutStorageValue(engine.GetSnapshot(), key, extensions::BigIntegerExtensions::ToByteArray(BigInteger(value)));

    return vm::StackItem::Null();
}

std::shared_ptr<vm::StackItem> PolicyContract::OnIsBlocked(ApplicationEngine& engine,
                                                           const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto accountItem = args[0];
    auto accountBytes = accountItem->GetByteArray();

    if (accountBytes.Size() != 20) throw std::runtime_error("Invalid account");

    io::UInt160 account;
    std::memcpy(account.Data(), accountBytes.Data(), 20);

    return vm::StackItem::Create(IsBlocked(engine.GetSnapshot(), account));
}

std::shared_ptr<vm::StackItem> PolicyContract::OnBlockAccount(ApplicationEngine& engine,
                                                              const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::invalid_argument("Invalid arguments");

    // Check committee witness
    if (!CheckCommittee(engine)) throw std::runtime_error("Invalid committee signature");

    auto accountItem = args[0];
    auto accountBytes = accountItem->GetByteArray();

    if (accountBytes.Size() != 20) throw std::runtime_error("Invalid account");

    io::UInt160 account;
    std::memcpy(account.Data(), accountBytes.Data(), 20);

    // Check if it's a native contract - cannot block native contracts
    // Native contracts have specific known hashes we can check
    if (IsNativeContract(account))
    {
        throw std::runtime_error("Cannot block native contracts");
    }

    // Check if account is already blocked
    auto key = GetStorageKey(PREFIX_BLOCKED_ACCOUNT, account);
    if (!GetStorageValue(engine.GetSnapshot(), key).IsEmpty()) return vm::StackItem::Create(false);

    // Block account
    PutStorageValue(engine.GetSnapshot(), key, io::ByteVector{});

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> PolicyContract::OnUnblockAccount(ApplicationEngine& engine,
                                                                const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::invalid_argument("Invalid arguments");

    // Check committee witness
    if (!CheckCommittee(engine)) throw std::runtime_error("Invalid committee signature");

    auto accountItem = args[0];
    auto accountBytes = accountItem->GetByteArray();

    if (accountBytes.Size() != 20) throw std::runtime_error("Invalid account");

    io::UInt160 account;
    std::memcpy(account.Data(), accountBytes.Data(), 20);

    // Check if account is blocked
    auto key = GetStorageKey(PREFIX_BLOCKED_ACCOUNT, account);
    if (GetStorageValue(engine.GetSnapshot(), key).IsEmpty()) return vm::StackItem::Create(false);

    // Unblock account
    DeleteStorageValue(engine.GetSnapshot(), key);

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> PolicyContract::OnGetAttributeFee(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto attributeTypeItem = args[0];
    auto attributeType = static_cast<uint8_t>(attributeTypeItem->GetInteger());

    return vm::StackItem::Create(static_cast<int64_t>(GetAttributeFee(engine.GetSnapshot(), attributeType)));
}

std::shared_ptr<vm::StackItem> PolicyContract::OnSetAttributeFee(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2) throw std::invalid_argument("Invalid arguments");

    auto attributeTypeItem = args[0];
    auto valueItem = args[1];

    auto attributeType = static_cast<uint8_t>(attributeTypeItem->GetInteger());
    auto value = valueItem->GetInteger();

    // Validate attribute type - check if it's a valid TransactionAttributeType
    // Accept common values until full enum validation is implemented
    if (attributeType > 0x20 && attributeType != 0x20)  // 0x20 is NotaryAssisted
        throw std::runtime_error("Unsupported attribute type");

    if (value > MAX_ATTRIBUTE_FEE) throw std::out_of_range("Value out of range");

    // Check committee witness
    if (!CheckCommittee(engine)) throw std::runtime_error("Invalid committee signature");

    // Set attribute fee
    auto key = GetStorageKey(PREFIX_ATTRIBUTE_FEE, io::ByteVector{&attributeType, 1});
    auto currentValue = GetStorageValue(engine.GetSnapshot(), key);
    if (currentValue.IsEmpty())
    {
        // Create new entry with default value first
        PutStorageValue(
            engine.GetSnapshot(), key,
            extensions::BigIntegerExtensions::ToByteArray(BigInteger(static_cast<uint64_t>(DEFAULT_ATTRIBUTE_FEE))));
    }
    PutStorageValue(engine.GetSnapshot(), key, extensions::BigIntegerExtensions::ToByteArray(BigInteger(value)));

    return vm::StackItem::Null();
}

std::shared_ptr<vm::StackItem> PolicyContract::OnGetMillisecondsPerBlock(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    return vm::StackItem::Create(static_cast<int64_t>(GetMillisecondsPerBlock(engine.GetSnapshot())));
}

std::shared_ptr<vm::StackItem> PolicyContract::OnSetMillisecondsPerBlock(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::invalid_argument("Invalid arguments");

    auto valueItem = args[0];
    auto value = valueItem->GetInteger();

    if (value == 0 || value > MAX_MILLISECONDS_PER_BLOCK)
        throw std::out_of_range("MillisecondsPerBlock value should be between 1 and " +
                                std::to_string(MAX_MILLISECONDS_PER_BLOCK) + ", got " + std::to_string(value));

    // Check committee witness
    if (!CheckCommittee(engine)) throw std::runtime_error("invalid committee signature");

    // Get old value for event
    auto oldValue = GetMillisecondsPerBlock(engine.GetSnapshot());

    // Set milliseconds per block
    auto key = GetStorageKey(PREFIX_MILLISECONDS_PER_BLOCK, io::ByteVector{});
    PutStorageValue(engine.GetSnapshot(), key, extensions::BigIntegerExtensions::ToByteArray(BigInteger(value)));

    // Emit event with old and new values
    std::vector<std::shared_ptr<vm::StackItem>> state = {vm::StackItem::Create(static_cast<int64_t>(oldValue)),
                                                         vm::StackItem::Create(value)};
    engine.Notify(GetScriptHash(), MILLISECONDS_PER_BLOCK_CHANGED_EVENT, state);

    return vm::StackItem::Null();
}

std::shared_ptr<vm::StackItem> PolicyContract::OnGetMaxValidUntilBlockIncrement(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    return vm::StackItem::Create(static_cast<int64_t>(GetMaxValidUntilBlockIncrement(engine.GetSnapshot())));
}

std::shared_ptr<vm::StackItem> PolicyContract::OnSetMaxValidUntilBlockIncrement(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::invalid_argument("Invalid arguments");

    auto valueItem = args[0];
    auto value = valueItem->GetInteger();

    if (value == 0 || value > MAX_MAX_VALID_UNTIL_BLOCK_INCREMENT) throw std::out_of_range("Value out of range");

    // Check if value is less than max traceable blocks
    auto mtb = GetMaxTraceableBlocks(engine.GetSnapshot());
    if (value >= mtb)
        throw std::runtime_error("MaxValidUntilBlockIncrement must be lower than MaxTraceableBlocks (" +
                                 std::to_string(value) + " vs " + std::to_string(mtb) + ")");

    // Check committee witness
    if (!CheckCommittee(engine)) throw std::runtime_error("Invalid committee signature");

    // Set max valid until block increment
    auto key = GetStorageKey(PREFIX_MAX_VALID_UNTIL_BLOCK_INCREMENT, io::ByteVector{});
    PutStorageValue(engine.GetSnapshot(), key, extensions::BigIntegerExtensions::ToByteArray(BigInteger(value)));

    return vm::StackItem::Null();
}

std::shared_ptr<vm::StackItem> PolicyContract::OnGetMaxTraceableBlocks(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    return vm::StackItem::Create(static_cast<int64_t>(GetMaxTraceableBlocks(engine.GetSnapshot())));
}

std::shared_ptr<vm::StackItem> PolicyContract::OnSetMaxTraceableBlocks(
    ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::invalid_argument("Invalid arguments");

    auto valueItem = args[0];
    auto value = valueItem->GetInteger();

    if (value == 0 || value > MAX_MAX_TRACEABLE_BLOCKS)
        throw std::out_of_range("MaxTraceableBlocks value should be between 1 and " +
                                std::to_string(MAX_MAX_TRACEABLE_BLOCKS) + ", got " + std::to_string(value));

    auto oldVal = GetMaxTraceableBlocks(engine.GetSnapshot());
    if (value > oldVal)
        throw std::runtime_error("MaxTraceableBlocks can not be increased (old " + std::to_string(oldVal) + ", new " +
                                 std::to_string(value) + ")");

    auto mVUBIncrement = GetMaxValidUntilBlockIncrement(engine.GetSnapshot());
    if (value <= mVUBIncrement)
        throw std::runtime_error("MaxTraceableBlocks must be larger than MaxValidUntilBlockIncrement (" +
                                 std::to_string(value) + " vs " + std::to_string(mVUBIncrement) + ")");

    // Check committee witness
    if (!CheckCommittee(engine)) throw std::runtime_error("Invalid committee signature");

    // Set max traceable blocks
    auto key = GetStorageKey(PREFIX_MAX_TRACEABLE_BLOCKS, io::ByteVector{});
    PutStorageValue(engine.GetSnapshot(), key, extensions::BigIntegerExtensions::ToByteArray(BigInteger(value)));

    return vm::StackItem::Null();
}

bool PolicyContract::CheckCommittee(ApplicationEngine& engine) const
{
    try
    {
        // Get committee address from NEO token contract
        auto neoToken = NeoToken::GetInstance();
        auto committeeAddress = neoToken->GetCommitteeAddress(engine.GetSnapshot());

        // Check witness for committee address
        return engine.CheckWitness(committeeAddress);
    }
    catch (const std::exception& e)
    {
        // Error during committee verification - deny access for security
        return false;
    }
}

bool PolicyContract::IsNativeContract(const io::UInt160& scriptHash) const
{
    // Check against all known native contract hashes
    // Core token contracts
    auto neoToken = NeoToken::GetInstance();
    if (neoToken && neoToken->GetScriptHash() == scriptHash) return true;

    auto gasToken = GasToken::GetInstance();
    if (gasToken && gasToken->GetScriptHash() == scriptHash) return true;

    // System contracts
    auto contractManagement = ContractManagement::GetInstance();
    if (contractManagement && contractManagement->GetScriptHash() == scriptHash) return true;

    auto policyContract = PolicyContract::GetInstance();
    if (policyContract && policyContract->GetScriptHash() == scriptHash) return true;

    auto ledgerContract = LedgerContract::GetInstance();
    if (ledgerContract && ledgerContract->GetScriptHash() == scriptHash) return true;

    auto roleManagement = RoleManagement::GetInstance();
    if (roleManagement && roleManagement->GetScriptHash() == scriptHash) return true;

    // Service contracts
    auto oracleContract = OracleContract::GetInstance();
    if (oracleContract && oracleContract->GetScriptHash() == scriptHash) return true;

    auto notary = Notary::GetInstance();
    if (notary && notary->GetScriptHash() == scriptHash) return true;

    auto nameService = NameService::GetInstance();
    if (nameService && nameService->GetScriptHash() == scriptHash) return true;

    return false;
}

}  // namespace neo::smartcontract::native
