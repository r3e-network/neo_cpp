/**
 * @file interop_service.cpp
 * @brief Service implementations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/core/protocol_constants.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/ledger/block.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/witness_rule.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/network/p2p/payloads/neo3_transaction.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/contract.h>
#include <neo/smartcontract/native/native_contract.h>
#include <neo/smartcontract/interop_service.h>
#include <neo/smartcontract/storage_iterator.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <optional>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <vector>

namespace neo
{
namespace smartcontract
{

// Hash calculation function
uint32_t calculate_interop_hash(const std::string& name)
{
    io::ByteSpan nameSpan(reinterpret_cast<const uint8_t*>(name.data()), name.size());
    auto hash = cryptography::Hash::Sha256(nameSpan);

    uint32_t value = 0;
    std::memcpy(&value, hash.Data(), sizeof(value));
    return value;
}

// InteropDescriptor implementation
InteropDescriptor::InteropDescriptor(std::string name, uint32_t hash, std::function<void(ApplicationEngine&)> handler,
                                     int64_t fixed_price, CallFlags required_call_flags)
    : name(std::move(name)),
      hash(hash),
      handler(std::move(handler)),
      fixed_price(fixed_price),
      required_call_flags(required_call_flags)
{
}

// InteropService implementation
InteropService& InteropService::instance()
{
    static InteropService instance;
    return instance;
}

void InteropService::initialize()
{
    auto& service = instance();
    service.register_builtin_services();
    LOG_INFO("InteropService initialized with {} services", service.services_.size());
}

const InteropDescriptor* InteropService::get_descriptor(uint32_t hash) const
{
    auto it = services_.find(hash);
    return it != services_.end() ? &it->second : nullptr;
}

InteropDescriptor InteropService::register_service(const std::string& name,
                                                   std::function<void(ApplicationEngine&)> handler, int64_t fixed_price,
                                                   CallFlags required_call_flags)
{
    auto hash = calculate_interop_hash(name);
    InteropDescriptor descriptor(name, hash, std::move(handler), fixed_price, required_call_flags);
    instance().register_service_internal(descriptor);
    return descriptor;
}

void InteropService::register_service_internal(const InteropDescriptor& descriptor)
{
    services_[descriptor.hash] = descriptor;
}

void InteropService::register_builtin_services()
{
    auto register_no_capture = [](const char* name, auto fn, int64_t price, CallFlags flags) {
        register_service(name, std::function<void(ApplicationEngine&)>(fn), price, flags);
    };

    // System.Runtime
    register_no_capture("System.Runtime.Platform", runtime_platform, 1LL << 3, CallFlags::None);
    register_no_capture("System.Runtime.GetNetwork", runtime_get_network, 1LL << 3, CallFlags::None);
    register_no_capture("System.Runtime.GetAddressVersion", runtime_get_address_version, 1LL << 3, CallFlags::None);
    register_no_capture("System.Runtime.GetTrigger", runtime_get_trigger, 1LL << 3, CallFlags::None);
    register_no_capture("System.Runtime.GetTime", runtime_get_time, 1LL << 3, CallFlags::None);
    register_no_capture("System.Runtime.GetScriptContainer", runtime_get_script_container, 1LL << 3, CallFlags::None);
    register_no_capture("System.Runtime.GetExecutingScriptHash", runtime_get_executing_script_hash, 1LL << 4, CallFlags::None);
    register_no_capture("System.Runtime.GetCallingScriptHash", runtime_get_calling_script_hash, 1LL << 4, CallFlags::None);
    register_no_capture("System.Runtime.GetEntryScriptHash", runtime_get_entry_script_hash, 1LL << 4, CallFlags::None);
    register_no_capture("System.Runtime.LoadScript", runtime_load_script, 1LL << 15, CallFlags::AllowCall);
    register_no_capture("System.Runtime.CheckWitness", runtime_check_witness, 1LL << 10, CallFlags::None);
    register_no_capture("System.Runtime.GetInvocationCounter", runtime_get_invocation_counter, 1LL << 4, CallFlags::None);
    register_no_capture("System.Runtime.GetRandom", runtime_get_random, 0, CallFlags::None);
    register_no_capture("System.Runtime.Log", runtime_log, 1LL << 15, CallFlags::AllowNotify);
    register_no_capture("System.Runtime.Notify", runtime_notify, 1LL << 15, CallFlags::AllowNotify);
    register_no_capture("System.Runtime.GetNotifications", runtime_get_notifications, 1LL << 12, CallFlags::None);
    register_no_capture("System.Runtime.GasLeft", runtime_gas_left, 1LL << 4, CallFlags::None);
    register_no_capture("System.Runtime.BurnGas", runtime_burn_gas, 1LL << 4, CallFlags::None);
    register_no_capture("System.Runtime.CurrentSigners", runtime_current_signers, 1LL << 4, CallFlags::None);

    // System.Crypto
    register_no_capture("System.Crypto.CheckSig", crypto_check_sig, 1LL << 15, CallFlags::None);
    register_no_capture("System.Crypto.CheckMultisig", crypto_check_multisig, 0, CallFlags::None);

    // System.Contract
    register_no_capture("System.Contract.Call", contract_call, 1LL << 15, CallFlags::ReadStates | CallFlags::AllowCall);
    register_no_capture("System.Contract.CallNative", contract_call_native, 0, CallFlags::None);
    register_no_capture("System.Contract.GetCallFlags", contract_get_call_flags, 1LL << 10, CallFlags::None);
    register_no_capture("System.Contract.CreateStandardAccount", contract_create_standard_account, 0, CallFlags::None);
    register_no_capture("System.Contract.CreateMultisigAccount", contract_create_multisig_account, 0, CallFlags::None);
    register_no_capture("System.Contract.NativeOnPersist", contract_native_on_persist, 0, CallFlags::States);
    register_no_capture("System.Contract.NativePostPersist", contract_native_post_persist, 0, CallFlags::States);

    // System.Storage
    register_no_capture("System.Storage.GetContext", storage_get_context, 1LL << 4, CallFlags::ReadStates);
    register_no_capture("System.Storage.GetReadOnlyContext", storage_get_readonly_context, 1LL << 4, CallFlags::ReadStates);
    register_no_capture("System.Storage.AsReadOnly", storage_as_readonly, 1LL << 4, CallFlags::ReadStates);
    register_no_capture("System.Storage.Get", storage_get, 1LL << 15, CallFlags::ReadStates);
    register_no_capture("System.Storage.Find", storage_find, 1LL << 15, CallFlags::ReadStates);
    register_no_capture("System.Storage.Put", storage_put, 1LL << 15, CallFlags::WriteStates);
    register_no_capture("System.Storage.Delete", storage_delete, 1LL << 15, CallFlags::WriteStates);

    // System.Iterator
    register_no_capture("System.Iterator.Next", iterator_next, 1LL << 15, CallFlags::None);
    register_no_capture("System.Iterator.Key", iterator_key, 1LL << 4, CallFlags::None);
    register_no_capture("System.Iterator.Value", iterator_value, 1LL << 4, CallFlags::None);
}

// System.Runtime implementations
void InteropService::runtime_platform(ApplicationEngine& engine) { engine.Push(vm::StackItem::Create("NEO")); }

void InteropService::runtime_get_network(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(engine.GetNetworkMagic())));
}

void InteropService::runtime_get_address_version(ApplicationEngine& engine)
{
    uint8_t addressVersion = core::ProtocolConstants::AddressVersion;
    if (const auto* settings = engine.GetProtocolSettings())
    {
        addressVersion = settings->GetAddressVersion();
    }
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(addressVersion)));
}

void InteropService::runtime_get_trigger(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(engine.GetTrigger())));
}

void InteropService::runtime_get_time(ApplicationEngine& engine)
{
    uint64_t timestamp = 0;
    if (const auto* block = engine.GetPersistingBlock())
    {
        timestamp = static_cast<uint64_t>(block->GetTimestamp() / 1000000);
    }
    else
    {
        timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::system_clock::now().time_since_epoch())
                                              .count());
    }

    engine.Push(vm::StackItem::Create(static_cast<int64_t>(timestamp)));
}

void InteropService::runtime_get_script_container(ApplicationEngine& engine)
{
    const auto* container = engine.GetContainer();
    if (!container)
    {
        engine.Push(vm::StackItem::Null());
        return;
    }

    auto mapItem = vm::StackItem::CreateMap();
    auto map = std::dynamic_pointer_cast<vm::MapItem>(mapItem);

    if (auto tx = dynamic_cast<const ledger::Transaction*>(container))
    {
        map->Set(vm::StackItem::Create("type"), vm::StackItem::Create("Transaction"));

        map->Set(vm::StackItem::Create("hash"), vm::StackItem::Create(tx->GetHash()));
        map->Set(vm::StackItem::Create("version"), vm::StackItem::Create(static_cast<int64_t>(tx->GetVersion())));
        map->Set(vm::StackItem::Create("nonce"), vm::StackItem::Create(static_cast<int64_t>(tx->GetNonce())));
        map->Set(vm::StackItem::Create("sender"), vm::StackItem::Create(tx->GetSender()));
        map->Set(vm::StackItem::Create("sysfee"), vm::StackItem::Create(tx->GetSystemFee()));
        map->Set(vm::StackItem::Create("netfee"), vm::StackItem::Create(tx->GetNetworkFee()));
        map->Set(vm::StackItem::Create("validuntilblock"),
                 vm::StackItem::Create(static_cast<int64_t>(tx->GetValidUntilBlock())));
    }
    else if (auto block = dynamic_cast<const ledger::Block*>(container))
    {
        map->Set(vm::StackItem::Create("type"), vm::StackItem::Create("Block"));
        map->Set(vm::StackItem::Create("hash"), vm::StackItem::Create(block->GetHash()));
        map->Set(vm::StackItem::Create("version"), vm::StackItem::Create(static_cast<int64_t>(block->GetVersion())));
        map->Set(vm::StackItem::Create("index"), vm::StackItem::Create(static_cast<int64_t>(block->GetIndex())));
        map->Set(vm::StackItem::Create("merkleroot"), vm::StackItem::Create(block->GetMerkleRoot()));
        map->Set(vm::StackItem::Create("timestamp"),
                 vm::StackItem::Create(static_cast<int64_t>(block->GetTimestamp() / 1000000)));
        map->Set(vm::StackItem::Create("nextconsensus"), vm::StackItem::Create(block->GetNextConsensus()));
    }

    engine.Push(mapItem);
}

void InteropService::runtime_get_executing_script_hash(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(engine.GetCurrentScriptHash()));
}

void InteropService::runtime_get_calling_script_hash(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(engine.GetCallingScriptHash()));
}

void InteropService::runtime_get_entry_script_hash(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(engine.GetEntryScriptHash()));
}

void InteropService::runtime_load_script(ApplicationEngine& engine)
{
    // Basic implementation - would load and execute script in full implementation
    auto script = engine.Pop();
    auto call_flags = engine.Pop();
    auto args = engine.Pop();

    // Parameters consumed - script loading handled by execution engine
}

void InteropService::runtime_check_witness(ApplicationEngine& engine)
{
    auto hashItem = engine.Pop();
    auto data = hashItem->GetByteArray();

    bool result = false;
    if (data.Size() == core::ProtocolConstants::UInt160Size)
    {
        io::UInt160 hash(data.AsSpan());
        result = engine.CheckWitness(hash);
    }
    else if (data.Size() == core::ProtocolConstants::UInt256Size)
    {
        io::UInt256 hash(data.AsSpan());
        result = engine.CheckWitness(hash);
    }

    engine.Push(vm::StackItem::Create(result));
}

void InteropService::runtime_get_invocation_counter(ApplicationEngine& engine)
{
    try
    {
        auto scriptHash = engine.GetCurrentScriptHash();
        int64_t count = engine.GetInvocationCount(scriptHash);
        if (count == 0)
        {
            count = 1;
            engine.SetInvocationCount(scriptHash, count);
        }
        engine.Push(vm::StackItem::Create(count));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Create(static_cast<int64_t>(1)));
    }
}

void InteropService::runtime_get_random(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(engine.GetRandom())));
}

void InteropService::runtime_log(ApplicationEngine& engine)
{
    auto message = engine.Pop();
    engine.Log(message->GetString());
}

void InteropService::runtime_notify(ApplicationEngine& engine)
{
    auto stateItem = engine.Pop();
    auto eventItem = engine.Pop();

    std::vector<std::shared_ptr<vm::StackItem>> stateArray;
    if (stateItem->IsArray())
    {
        stateArray = stateItem->GetArray();
    }
    else
    {
        stateArray.push_back(stateItem);
    }
    engine.Notify(engine.GetCurrentScriptHash(), eventItem->GetString(), stateArray);
}

void InteropService::runtime_get_notifications(ApplicationEngine& engine)
{
    std::optional<io::UInt160> filter;

    // Optional filter argument
    if (!engine.GetCurrentContext().GetEvaluationStack().empty())
    {
        auto filterItem = engine.Pop();
        if (!filterItem->IsNull())
        {
            auto bytes = filterItem->GetByteArray();
            if (bytes.Size() != core::ProtocolConstants::UInt160Size)
            {
                throw std::runtime_error("Invalid notification filter length");
            }
            filter.emplace(io::UInt160(bytes.AsSpan()));
        }
    }

    auto notificationsArray = std::dynamic_pointer_cast<vm::ArrayItem>(vm::StackItem::CreateArray());
    for (const auto& notification : engine.GetNotifications())
    {
        if (filter.has_value() && notification.script_hash != filter.value()) continue;

        auto record = std::dynamic_pointer_cast<vm::ArrayItem>(vm::StackItem::CreateArray());
        record->Add(vm::StackItem::Create(notification.script_hash));
        record->Add(vm::StackItem::Create(notification.event_name));

        auto stateItem = std::dynamic_pointer_cast<vm::ArrayItem>(vm::StackItem::CreateArray());
        for (const auto& entry : notification.state)
        {
            stateItem->Add(entry);
        }
        record->Add(stateItem);

        notificationsArray->Add(record);
    }

    engine.Push(notificationsArray);
}

void InteropService::runtime_gas_left(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(engine.GetGasLeft()));
}

void InteropService::runtime_burn_gas(ApplicationEngine& engine)
{
    auto gasItem = engine.Pop();
    auto gas = gasItem->GetInteger();
    if (gas < 0) throw std::runtime_error("Gas amount cannot be negative");

    engine.AddGas(gas);
}

namespace
{
struct StorageContextData
{
    io::UInt160 scriptHash;
    bool readOnly;
};

class StorageContextStackItem : public vm::StackItem
{
   public:
    explicit StorageContextStackItem(std::shared_ptr<StorageContextData> context) : context_(std::move(context)) {}

    vm::StackItemType GetType() const override { return vm::StackItemType::InteropInterface; }

    bool GetBoolean() const override { return static_cast<bool>(context_); }

    size_t Size() const override { return sizeof(void*); }

    size_t GetHashCode() const override { return std::hash<void*>()(context_.get()); }

    const std::shared_ptr<StorageContextData>& GetContext() const { return context_; }

    bool Equals(const vm::StackItem& other) const override
    {
        if (other.GetType() != vm::StackItemType::InteropInterface) return false;
        const auto* otherContext = dynamic_cast<const StorageContextStackItem*>(&other);
        return otherContext && otherContext->context_.get() == context_.get();
    }

   private:
    std::shared_ptr<StorageContextData> context_;
};

std::shared_ptr<vm::ArrayItem> create_empty_array()
{
    return std::dynamic_pointer_cast<vm::ArrayItem>(vm::StackItem::CreateArray());
}

std::shared_ptr<StorageContextStackItem> create_storage_context_item(const io::UInt160& scriptHash, bool readOnly)
{
    auto context = std::make_shared<StorageContextData>();
    context->scriptHash = scriptHash;
    context->readOnly = readOnly;
    return std::make_shared<StorageContextStackItem>(context);
}

StorageContextData parse_storage_context_item(const std::shared_ptr<vm::StackItem>& item)
{
    auto contextItem = std::dynamic_pointer_cast<StorageContextStackItem>(item);
    if (!contextItem || !contextItem->GetContext())
    {
        throw std::runtime_error("Invalid storage context");
    }

    return *(contextItem->GetContext());
}

std::shared_ptr<vm::ArrayItem> convert_condition_sequence(
    const std::vector<std::shared_ptr<ledger::WitnessCondition>>& conditions);

std::shared_ptr<vm::StackItem> convert_condition_to_stack_item(
    const std::shared_ptr<ledger::WitnessCondition>& condition);

std::shared_ptr<vm::StackItem> convert_witness_rule_to_stack_item(const ledger::WitnessRule& rule)
{
    auto ruleStruct = create_empty_array();
    ruleStruct->Add(vm::StackItem::Create(static_cast<int64_t>(rule.GetAction())));
    ruleStruct->Add(convert_condition_to_stack_item(rule.GetCondition()));
    return ruleStruct;
}

std::shared_ptr<vm::StackItem> convert_signer_to_stack_item(const ledger::Signer& signer)
{
    auto signerStruct = create_empty_array();

    signerStruct->Add(vm::StackItem::Create(signer.GetAccount()));
    signerStruct->Add(vm::StackItem::Create(static_cast<int64_t>(signer.GetScopes())));

    auto allowedContracts = create_empty_array();
    for (const auto& contractHash : signer.GetAllowedContracts())
    {
        allowedContracts->Add(vm::StackItem::Create(contractHash));
    }
    signerStruct->Add(allowedContracts);

    auto allowedGroups = create_empty_array();
    for (const auto& group : signer.GetAllowedGroups())
    {
        auto groupBytes = group.ToArray();
        allowedGroups->Add(vm::StackItem::Create(io::ByteSpan(groupBytes.Data(), groupBytes.Size())));
    }
    signerStruct->Add(allowedGroups);

    auto rulesArray = create_empty_array();
    for (const auto& rule : signer.GetRules())
    {
        rulesArray->Add(convert_witness_rule_to_stack_item(rule));
    }
    signerStruct->Add(rulesArray);

    return signerStruct;
}

class StorageIteratorStackItem : public vm::StackItem
{
   public:
    explicit StorageIteratorStackItem(std::shared_ptr<smartcontract::StorageIterator> iterator)
        : iterator_(std::move(iterator))
    {
    }

    vm::StackItemType GetType() const override { return vm::StackItemType::InteropInterface; }

    bool GetBoolean() const override { return static_cast<bool>(iterator_); }

    size_t Size() const override { return sizeof(void*); }

    size_t GetHashCode() const override { return std::hash<void*>()(iterator_.get()); }

    std::shared_ptr<smartcontract::StorageIterator> GetIterator() const { return iterator_; }

    bool Equals(const vm::StackItem& other) const override
    {
        if (other.GetType() != vm::StackItemType::InteropInterface) return false;
        const auto* otherIterator = dynamic_cast<const StorageIteratorStackItem*>(&other);
        return otherIterator && otherIterator->iterator_.get() == iterator_.get();
    }

   private:
    std::shared_ptr<smartcontract::StorageIterator> iterator_;
};

std::shared_ptr<vm::ArrayItem> convert_condition_sequence(
    const std::vector<std::shared_ptr<ledger::WitnessCondition>>& conditions)
{
    auto conditionArray = create_empty_array();
    for (const auto& condition : conditions)
    {
        conditionArray->Add(convert_condition_to_stack_item(condition));
    }
    return conditionArray;
}

std::shared_ptr<vm::StackItem> convert_condition_to_stack_item(
    const std::shared_ptr<ledger::WitnessCondition>& condition)
{
    if (!condition) return vm::StackItem::Null();

    auto array = create_empty_array();
    array->Add(vm::StackItem::Create(static_cast<int64_t>(condition->GetType())));

    switch (condition->GetType())
    {
        case ledger::WitnessCondition::Type::Boolean:
        {
            auto booleanCondition = std::dynamic_pointer_cast<ledger::BooleanCondition>(condition);
            const bool value = booleanCondition ? booleanCondition->GetValue() : false;
            array->Add(vm::StackItem::Create(value));
            break;
        }
        case ledger::WitnessCondition::Type::Not:
        {
            auto notCondition = std::dynamic_pointer_cast<ledger::NotCondition>(condition);
            array->Add(convert_condition_to_stack_item(notCondition ? notCondition->GetCondition() : nullptr));
            break;
        }
        case ledger::WitnessCondition::Type::And:
        {
            auto andCondition = std::dynamic_pointer_cast<ledger::AndCondition>(condition);
            array->Add(convert_condition_sequence(andCondition ? andCondition->GetConditions()
                                                               : std::vector<std::shared_ptr<ledger::WitnessCondition>>{}));
            break;
        }
        case ledger::WitnessCondition::Type::Or:
        {
            auto orCondition = std::dynamic_pointer_cast<ledger::OrCondition>(condition);
            array->Add(convert_condition_sequence(orCondition ? orCondition->GetConditions()
                                                              : std::vector<std::shared_ptr<ledger::WitnessCondition>>{}));
            break;
        }
        case ledger::WitnessCondition::Type::ScriptHash:
        {
            auto scriptHashCondition = std::dynamic_pointer_cast<ledger::ScriptHashCondition>(condition);
            if (scriptHashCondition)
            {
                array->Add(vm::StackItem::Create(scriptHashCondition->GetHash()));
            }
            else
            {
                array->Add(vm::StackItem::Null());
            }
            break;
        }
        case ledger::WitnessCondition::Type::Group:
        {
            auto groupCondition = std::dynamic_pointer_cast<ledger::GroupCondition>(condition);
            if (groupCondition)
            {
                auto groupBytes = groupCondition->GetGroup().ToArray();
                array->Add(vm::StackItem::Create(io::ByteSpan(groupBytes.Data(), groupBytes.Size())));
            }
            else
            {
                array->Add(vm::StackItem::Null());
            }
            break;
        }
        case ledger::WitnessCondition::Type::CalledByEntry:
        {
            array->Add(vm::StackItem::Null());
            break;
        }
        case ledger::WitnessCondition::Type::CalledByContract:
        {
            auto contractCondition = std::dynamic_pointer_cast<ledger::CalledByContractCondition>(condition);
            if (contractCondition)
            {
                array->Add(vm::StackItem::Create(contractCondition->GetHash()));
            }
            else
            {
                array->Add(vm::StackItem::Null());
            }
            break;
        }
        case ledger::WitnessCondition::Type::CalledByGroup:
        {
            auto groupCondition = std::dynamic_pointer_cast<ledger::CalledByGroupCondition>(condition);
            if (groupCondition)
            {
                auto groupBytes = groupCondition->GetGroup().ToArray();
                array->Add(vm::StackItem::Create(io::ByteSpan(groupBytes.Data(), groupBytes.Size())));
            }
            else
            {
                array->Add(vm::StackItem::Null());
            }
            break;
        }
        default:
            array->Add(vm::StackItem::Null());
            break;
    }

    return array;
}
}  // namespace

void InteropService::runtime_current_signers(ApplicationEngine& engine)
{
    const auto* container = engine.GetContainer();
    if (!container)
    {
        engine.Push(vm::StackItem::Null());
        return;
    }

    const std::vector<ledger::Signer>* signersPtr = nullptr;

    if (const auto* tx = dynamic_cast<const network::p2p::payloads::Neo3Transaction*>(container))
    {
        signersPtr = &tx->GetSigners();
    }
    else if (const auto* txAlias = dynamic_cast<const ledger::Transaction*>(container))
    {
        signersPtr = &txAlias->GetSigners();
    }

    if (!signersPtr)
    {
        engine.Push(vm::StackItem::Null());
        return;
    }

    auto signersArray = create_empty_array();
    for (const auto& signer : *signersPtr)
    {
        signersArray->Add(convert_signer_to_stack_item(signer));
    }

    engine.Push(signersArray);
}

// System.Crypto implementations
void InteropService::crypto_check_sig(ApplicationEngine& engine)
{
    auto signatureItem = engine.Pop();
    auto publicKeyItem = engine.Pop();

    auto signature = signatureItem->GetByteArray();
    auto publicKey = publicKeyItem->GetByteArray();

    try
    {
        const auto* container = engine.GetContainer();
        if (!container)
        {
            engine.Push(vm::StackItem::Create(false));
            return;
        }

        io::ByteVector signData;

        if (auto tx = dynamic_cast<const network::p2p::payloads::Neo3Transaction*>(container))
        {
            std::ostringstream stream;
            io::BinaryWriter writer(stream);

            writer.Write(engine.GetNetworkMagic());
            writer.Write(tx->GetVersion());
            writer.Write(tx->GetNonce());
            writer.Write(tx->GetSystemFee());
            writer.Write(tx->GetNetworkFee());
            writer.Write(tx->GetValidUntilBlock());

            const auto& signersList = tx->GetSigners();
            writer.WriteVarInt(signersList.size());
            for (const ledger::Signer& signer : signersList)
            {
                writer.Write(signer.GetAccount());
                auto scopes = static_cast<uint8_t>(signer.GetScopes());
                writer.Write(scopes);

                if ((scopes & static_cast<uint8_t>(ledger::WitnessScope::CustomContracts)) != 0)
                {
                    const auto& contracts = signer.GetAllowedContracts();
                    writer.WriteVarInt(contracts.size());
                    for (const auto& contract : contracts)
                    {
                        writer.Write(contract);
                    }
                }

                if ((scopes & static_cast<uint8_t>(ledger::WitnessScope::CustomGroups)) != 0)
                {
                    const auto& groups = signer.GetAllowedGroups();
                    writer.WriteVarInt(groups.size());
                    for (const auto& group : groups)
                    {
                        auto groupBytes = group.ToArray();
                        writer.Write(groupBytes.AsSpan());
                    }
                }
            }

            // Attributes are omitted in signature calculation in this simplified implementation
            writer.WriteVarInt(0);

            writer.WriteVarBytes(tx->GetScript().AsSpan());

            const std::string data = stream.str();
            signData = io::ByteVector(
                io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        }
        else if (auto block = dynamic_cast<const ledger::Block*>(container))
        {
            std::ostringstream stream;
            io::BinaryWriter writer(stream);

            writer.Write(engine.GetNetworkMagic());
            writer.Write(block->GetVersion());
            writer.Write(block->GetPreviousHash());
            writer.Write(block->GetMerkleRoot());
            writer.Write(static_cast<uint64_t>(block->GetTimestamp()));
            writer.Write(block->GetNonce());
            writer.Write(block->GetIndex());
            writer.Write(block->GetPrimaryIndex());
            writer.Write(block->GetNextConsensus());

            const std::string data = stream.str();
            signData = io::ByteVector(
                io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        }
        else
        {
            engine.Push(vm::StackItem::Create(false));
            return;
        }

        auto ecPoint = cryptography::ecc::ECPoint::FromBytes(publicKey.AsSpan(), "secp256r1");
        if (ecPoint.IsInfinity())
        {
            engine.Push(vm::StackItem::Create(false));
            return;
        }

        bool verified = cryptography::Crypto::VerifySignature(signData.AsSpan(), signature.AsSpan(), ecPoint);
        engine.Push(vm::StackItem::Create(verified));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Create(false));
    }
}

void InteropService::crypto_check_multisig(ApplicationEngine& engine)
{
    auto signaturesItem = engine.Pop();
    auto publicKeysItem = engine.Pop();

    auto signatures = signaturesItem->GetArray();
    auto publicKeys = publicKeysItem->GetArray();

    try
    {
        if (signatures.empty() || publicKeys.empty() || signatures.size() > publicKeys.size() ||
            publicKeys.size() > core::ProtocolConstants::MaxTransactionAttributes)
        {
            engine.Push(vm::StackItem::Create(false));
            return;
        }

        const auto* container = engine.GetContainer();
        if (!container)
        {
            engine.Push(vm::StackItem::Create(false));
            return;
        }

        io::ByteVector signData;
        if (auto tx = dynamic_cast<const network::p2p::payloads::Neo3Transaction*>(container))
        {
            std::ostringstream stream;
            io::BinaryWriter writer(stream);

            writer.Write(engine.GetNetworkMagic());
            writer.Write(tx->GetVersion());
            writer.Write(tx->GetNonce());
            writer.Write(tx->GetSystemFee());
            writer.Write(tx->GetNetworkFee());
            writer.Write(tx->GetValidUntilBlock());

            const auto& signersList = tx->GetSigners();
            writer.WriteVarInt(signersList.size());
            for (const ledger::Signer& signer : signersList)
            {
                writer.Write(signer.GetAccount());
                writer.Write(static_cast<uint8_t>(signer.GetScopes()));
            }

            writer.WriteVarInt(0);  // Attributes omitted
            writer.WriteVarBytes(tx->GetScript().AsSpan());

            const std::string data = stream.str();
            signData = io::ByteVector(
                io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        }
        else
        {
            engine.Push(vm::StackItem::Create(false));
            return;
        }

        size_t validSignatures = 0;
        size_t pubKeyIndex = 0;

        for (size_t sigIndex = 0; sigIndex < signatures.size() && pubKeyIndex < publicKeys.size(); ++sigIndex)
        {
            auto signature = signatures[sigIndex]->GetByteArray();
            bool signatureValid = false;

            for (size_t candidateIndex = pubKeyIndex; candidateIndex < publicKeys.size(); ++candidateIndex)
            {
                auto pubKey = publicKeys[candidateIndex]->GetByteArray();

                try
                {
                    auto ecPoint = cryptography::ecc::ECPoint::FromBytes(pubKey.AsSpan());
                    if (cryptography::Crypto::VerifySignature(signData.AsSpan(), signature.AsSpan(), ecPoint))
                    {
                        validSignatures++;
                        pubKeyIndex = candidateIndex + 1;
                        signatureValid = true;
                        break;
                    }
                }
                catch (const std::exception&)
                {
                    // Ignore and continue searching other public keys
                }
            }

            if (!signatureValid)
            {
                engine.Push(vm::StackItem::Create(false));
                return;
            }
        }

        engine.Push(vm::StackItem::Create(validSignatures == signatures.size()));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Create(false));
    }
}

// System.Contract implementations
void InteropService::contract_call(ApplicationEngine& engine)
{
    auto argsItem = engine.Pop();
    auto methodItem = engine.Pop();
    auto scriptHashItem = engine.Pop();
    auto flagsItem = engine.Pop();

    auto hashBytes = scriptHashItem->GetByteArray();
    if (hashBytes.Size() != core::ProtocolConstants::UInt160Size)
        throw std::runtime_error("Invalid contract script hash");

    io::UInt160 scriptHash(hashBytes.AsSpan());
    auto method = methodItem->GetString();
    auto flags = static_cast<CallFlags>(flagsItem->GetInteger());
    auto args = argsItem->IsArray() ? argsItem->GetArray() : std::vector<std::shared_ptr<vm::StackItem>>{};

    auto result = engine.CallContract(scriptHash, method, args, flags);
    engine.Push(result ? result : vm::StackItem::Null());
}

void InteropService::contract_call_native(ApplicationEngine& engine)
{
    auto argsItem = engine.Pop();
    auto methodItem = engine.Pop();
    auto scriptHashItem = engine.Pop();

    auto hashBytes = scriptHashItem->GetByteArray();
    if (hashBytes.Size() != core::ProtocolConstants::UInt160Size)
        throw std::runtime_error("Invalid native contract hash");

    io::UInt160 hash(hashBytes.AsSpan());
    auto method = methodItem->GetString();
    auto args = argsItem->IsArray() ? argsItem->GetArray() : std::vector<std::shared_ptr<vm::StackItem>>{};

    auto* nativeContract = engine.GetNativeContract(hash);
    if (!nativeContract)
    {
        engine.Push(vm::StackItem::Null());
        return;
    }

    auto result = nativeContract->Invoke(engine, method, args, engine.GetCallFlags());
    engine.Push(result ? result : vm::StackItem::Null());
}

void InteropService::contract_get_call_flags(ApplicationEngine& engine)
{
    engine.Push(vm::StackItem::Create(static_cast<int64_t>(engine.GetCallFlags())));
}

void InteropService::contract_create_standard_account(ApplicationEngine& engine)
{
    auto pubKeyItem = engine.Pop();
    auto pubKeyBytes = pubKeyItem->GetByteArray();

    try
    {
        if (pubKeyBytes.Size() != core::ProtocolConstants::ECPointSize &&
            pubKeyBytes.Size() != core::ProtocolConstants::ECPointSize * 2)
        {
            throw std::runtime_error("Invalid public key length");
        }

        auto ecPoint = cryptography::ecc::ECPoint::FromBytes(pubKeyBytes.AsSpan(), "secp256r1");
        auto contract = Contract::CreateSignatureContract(ecPoint);
        engine.Push(vm::StackItem::Create(contract.GetScriptHash()));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Null());
    }
}

void InteropService::contract_create_multisig_account(ApplicationEngine& engine)
{
    auto mItem = engine.Pop();
    auto pubKeysItem = engine.Pop();

    try
    {
        int m = static_cast<int>(mItem->GetInteger());
        auto pubKeysArray = pubKeysItem->GetArray();

        if (m <= 0 || static_cast<size_t>(m) > pubKeysArray.size() ||
            pubKeysArray.size() > core::ProtocolConstants::MaxArraySize)
        {
            throw std::runtime_error("Invalid multisig parameters");
        }

        std::vector<std::vector<uint8_t>> pubKeyBuffers;
        pubKeyBuffers.reserve(pubKeysArray.size());

        for (const auto& keyItem : pubKeysArray)
        {
            auto keyBytes = keyItem->GetByteArray();
            if (keyBytes.Size() != core::ProtocolConstants::ECPointSize &&
                keyBytes.Size() != core::ProtocolConstants::ECPointSize * 2)
            {
                throw std::runtime_error("Invalid public key length");
            }

            pubKeyBuffers.emplace_back(keyBytes.begin(), keyBytes.end());
        }

        std::sort(pubKeyBuffers.begin(), pubKeyBuffers.end(),
                  [](const auto& lhs, const auto& rhs) { return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()); });

        std::vector<cryptography::ecc::ECPoint> ecPoints;
        ecPoints.reserve(pubKeyBuffers.size());
        for (const auto& buffer : pubKeyBuffers)
        {
            ecPoints.emplace_back(
                cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(buffer.data(), buffer.size()), "secp256r1"));
        }

        auto contract = Contract::CreateMultiSigContract(m, ecPoints);
        engine.Push(vm::StackItem::Create(contract.GetScriptHash()));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Null());
    }
}

void InteropService::contract_native_on_persist(ApplicationEngine& engine)
{
    // Native contract OnPersist - this is called during block persistence
    // Basic implementation - would invoke OnPersist for all native contracts in full implementation
    // Native contract OnPersist execution is handled by engine
}

void InteropService::contract_native_post_persist(ApplicationEngine& engine)
{
    // Native contract PostPersist - this is called after block persistence
    // Basic implementation - would invoke PostPersist for all native contracts in full implementation
    // Native contract PostPersist execution is handled by engine
}

// System.Storage implementations
void InteropService::storage_get_context(ApplicationEngine& engine)
{
    auto context = create_storage_context_item(engine.GetCurrentScriptHash(), false);
    engine.Push(std::static_pointer_cast<vm::StackItem>(context));
}

void InteropService::storage_get_readonly_context(ApplicationEngine& engine)
{
    auto context = create_storage_context_item(engine.GetCurrentScriptHash(), true);
    engine.Push(std::static_pointer_cast<vm::StackItem>(context));
}

void InteropService::storage_as_readonly(ApplicationEngine& engine)
{
    auto contextItem = engine.Pop();
    try
    {
        auto context = parse_storage_context_item(contextItem);
        auto readOnlyContext = create_storage_context_item(context.scriptHash, true);
        engine.Push(std::static_pointer_cast<vm::StackItem>(readOnlyContext));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Null());
    }
}

void InteropService::storage_get(ApplicationEngine& engine)
{
    auto keyItem = engine.Pop();
    auto contextItem = engine.Pop();

    try
    {
        const auto context = parse_storage_context_item(contextItem);
        auto keyBytes = keyItem->GetByteArray();
        if (keyBytes.Size() > core::ProtocolConstants::MaxStorageKeyLength)
        {
            throw std::runtime_error("Storage key is too long");
        }

        auto snapshot = engine.GetSnapshot();
        if (!snapshot)
        {
            engine.Push(vm::StackItem::Null());
            return;
        }

        persistence::StorageKey storageKey(context.scriptHash, keyBytes);
        auto value = snapshot->TryGet(storageKey);
        if (!value)
        {
            engine.Push(vm::StackItem::Null());
            return;
        }

        engine.Push(vm::StackItem::Create(value->GetValue()));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Null());
    }
}

void InteropService::storage_find(ApplicationEngine& engine)
{
    auto prefixItem = engine.Pop();
    auto contextItem = engine.Pop();

    try
    {
        const auto context = parse_storage_context_item(contextItem);
        auto prefixBytes = prefixItem->GetByteArray();
        if (prefixBytes.Size() > core::ProtocolConstants::MaxStorageKeyLength)
        {
            throw std::runtime_error("Storage prefix is too long");
        }

        auto snapshot = engine.GetSnapshot();
        if (!snapshot)
        {
            engine.Push(vm::StackItem::Null());
            return;
        }

        persistence::StorageKey prefixKey(context.scriptHash, prefixBytes);
        auto iterator = std::make_shared<smartcontract::StorageIterator>(snapshot, prefixKey);
        auto iteratorItem = std::make_shared<StorageIteratorStackItem>(iterator);
        engine.Push(std::static_pointer_cast<vm::StackItem>(iteratorItem));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Null());
    }
}

void InteropService::storage_put(ApplicationEngine& engine)
{
    auto valueItem = engine.Pop();
    auto keyItem = engine.Pop();
    auto contextItem = engine.Pop();

    const auto context = parse_storage_context_item(contextItem);
    if (context.readOnly)
    {
        throw std::runtime_error("Storage context is read-only");
    }

    auto keyBytes = keyItem->GetByteArray();
    if (keyBytes.Size() > core::ProtocolConstants::MaxStorageKeyLength)
    {
        throw std::runtime_error("Storage key is too long");
    }

    auto valueBytes = valueItem->GetByteArray();
    if (valueBytes.Size() > core::ProtocolConstants::MaxStorageValueLength)
    {
        throw std::runtime_error("Storage value is too large");
    }

    auto snapshot = engine.GetSnapshot();
    if (!snapshot)
    {
        throw std::runtime_error("Snapshot unavailable");
    }

    persistence::StorageKey storageKey(context.scriptHash, keyBytes);
    auto storageItem =
        snapshot->GetAndChange(storageKey, [&]() { return std::make_shared<persistence::StorageItem>(); });
    if (!storageItem)
    {
        storageItem = std::make_shared<persistence::StorageItem>();
        storageItem->SetValue(valueBytes);
        snapshot->Add(storageKey, *storageItem);
    }
    else
    {
        storageItem->SetValue(valueBytes);
    }
}

void InteropService::storage_delete(ApplicationEngine& engine)
{
    auto keyItem = engine.Pop();
    auto contextItem = engine.Pop();

    const auto context = parse_storage_context_item(contextItem);
    if (context.readOnly)
    {
        throw std::runtime_error("Storage context is read-only");
    }

    auto keyBytes = keyItem->GetByteArray();
    if (keyBytes.Size() > core::ProtocolConstants::MaxStorageKeyLength)
    {
        throw std::runtime_error("Storage key is too long");
    }

    auto snapshot = engine.GetSnapshot();
    if (!snapshot)
    {
        throw std::runtime_error("Snapshot unavailable");
    }

    persistence::StorageKey storageKey(context.scriptHash, keyBytes);
    snapshot->Delete(storageKey);
}

// System.Iterator implementations
void InteropService::iterator_next(ApplicationEngine& engine)
{
    auto iteratorItem = engine.Pop();
    auto storageIteratorItem = std::dynamic_pointer_cast<StorageIteratorStackItem>(iteratorItem);
    if (!storageIteratorItem)
    {
        engine.Push(vm::StackItem::Create(false));
        return;
    }

    auto iterator = storageIteratorItem->GetIterator();
    if (!iterator || !iterator->HasNext())
    {
        engine.Push(vm::StackItem::Create(false));
        return;
    }

    try
    {
        iterator->Next();
        engine.Push(vm::StackItem::Create(true));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Create(false));
    }
}

void InteropService::iterator_key(ApplicationEngine& engine)
{
    auto iteratorItem = engine.Pop();
    auto storageIteratorItem = std::dynamic_pointer_cast<StorageIteratorStackItem>(iteratorItem);
    if (!storageIteratorItem)
    {
        engine.Push(vm::StackItem::Null());
        return;
    }

    auto iterator = storageIteratorItem->GetIterator();
    if (!iterator)
    {
        engine.Push(vm::StackItem::Null());
        return;
    }

    try
    {
        auto [key, _] = iterator->GetCurrent();
        engine.Push(vm::StackItem::Create(key));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Null());
    }
}

void InteropService::iterator_value(ApplicationEngine& engine)
{
    auto iteratorItem = engine.Pop();
    auto storageIteratorItem = std::dynamic_pointer_cast<StorageIteratorStackItem>(iteratorItem);
    if (!storageIteratorItem)
    {
        engine.Push(vm::StackItem::Null());
        return;
    }

    auto iterator = storageIteratorItem->GetIterator();
    if (!iterator)
    {
        engine.Push(vm::StackItem::Null());
        return;
    }

    try
    {
        auto [key, value] = iterator->GetCurrent();
        engine.Push(vm::StackItem::Create(value));
    }
    catch (const std::exception&)
    {
        engine.Push(vm::StackItem::Null());
    }
}

}  // namespace smartcontract
}  // namespace neo
