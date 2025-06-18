#include <neo/smartcontract/native/name_service.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <sstream>
#include <regex>

namespace neo::smartcontract::native
{
    NameService::NameService()
        : NativeContract(NAME, ID)
    {
    }

    std::shared_ptr<NameService> NameService::GetInstance()
    {
        static std::shared_ptr<NameService> instance = std::make_shared<NameService>();
        return instance;
    }

    void NameService::Initialize()
    {
        RegisterMethod("getPrice", CallFlags::ReadStates, std::bind(&NameService::OnGetPrice, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("setPrice", CallFlags::States, std::bind(&NameService::OnSetPrice, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("isAvailable", CallFlags::ReadStates, std::bind(&NameService::OnIsAvailable, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("register", CallFlags::States | CallFlags::AllowCall | CallFlags::AllowNotify, std::bind(&NameService::OnRegister, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("renew", CallFlags::States | CallFlags::AllowCall | CallFlags::AllowNotify, std::bind(&NameService::OnRenew, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("transfer", CallFlags::States | CallFlags::AllowNotify, std::bind(&NameService::OnTransfer, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("delete", CallFlags::States | CallFlags::AllowNotify, std::bind(&NameService::OnDelete, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("resolve", CallFlags::ReadStates, std::bind(&NameService::OnResolve, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getOwner", CallFlags::ReadStates, std::bind(&NameService::OnGetOwner, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getExpiration", CallFlags::ReadStates, std::bind(&NameService::OnGetExpiration, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getRecord", CallFlags::ReadStates, std::bind(&NameService::OnGetRecord, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("setRecord", CallFlags::States | CallFlags::AllowNotify, std::bind(&NameService::OnSetRecord, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("deleteRecord", CallFlags::States | CallFlags::AllowNotify, std::bind(&NameService::OnDeleteRecord, this, std::placeholders::_1, std::placeholders::_2));
    }

    bool NameService::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
    {
        if (hardfork == 0)
        {
            // Initialize price (1000 GAS)
            auto priceKey = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
            int64_t price = DEFAULT_PRICE;
            io::ByteVector priceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&price), sizeof(int64_t)));
            PutStorageValue(engine.GetSnapshot(), priceKey, priceValue);
        }

        return true;
    }

    bool NameService::OnPersist(ApplicationEngine& engine)
    {
        // Initialize contract if needed
        auto key = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
        auto value = GetStorageValue(engine.GetSnapshot(), key);
        if (value.IsEmpty())
        {
            InitializeContract(engine, 0);
        }

        return true;
    }

    bool NameService::PostPersist(ApplicationEngine& engine)
    {
        // Nothing to do post persist
        return true;
    }

    int64_t NameService::GetPrice(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return DEFAULT_PRICE;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    bool NameService::CheckCommittee(ApplicationEngine& engine) const
    {
        // Get the current script hash
        auto currentScriptHash = engine.GetCurrentScriptHash();

        // Get the NEO token contract
        auto neoToken = NeoToken::GetInstance();

        // Get the committee address
        auto committeeAddress = neoToken->GetCommitteeAddress(engine.GetSnapshot());

        // Check if the current script hash is the committee address
        return currentScriptHash == committeeAddress;
    }

    std::tuple<io::UInt160, uint64_t> NameService::GetName(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name) const
    {
        if (!ValidateName(name))
            throw std::runtime_error("Invalid name");

        auto key = CreateStorageKey(PREFIX_NAME, io::ByteVector(reinterpret_cast<const uint8_t*>(name.data()), name.size()));
        auto value = GetStorageValue(snapshot, key.GetKey());
        if (value.IsEmpty())
            throw std::runtime_error("Name not found");

        std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
        io::BinaryReader reader(stream);
        io::UInt160 owner = reader.ReadSerializable<io::UInt160>();
        uint64_t expiration = reader.ReadUInt64();
        return std::make_tuple(owner, expiration);
    }

    bool NameService::ValidateName(const std::string& name) const
    {
        // Check if name is valid
        if (name.empty() || name.length() > MAX_NAME_LENGTH)
            return false;

        // Check if name matches the pattern
        std::regex pattern("^[a-z0-9][a-z0-9-]{2,62}$");
        return std::regex_match(name, pattern);
    }

    bool NameService::IsAvailable(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name) const
    {
        if (!ValidateName(name))
            return false;

        try
        {
            auto [owner, expiration] = GetName(snapshot, name);
            uint32_t currentHeight = snapshot->GetCurrentBlockIndex();
            return expiration <= currentHeight;
        }
        catch (...)
        {
            return true; // Name not found, so it's available
        }
    }

    bool NameService::ValidateRecordType(const std::string& type) const
    {
        // Check if type is empty
        if (type.empty())
            return false;

        // Check if type contains only valid characters
        // Valid characters are: A-Z, a-z, 0-9, -, _
        static const std::regex typeRegex("^[A-Za-z0-9-_]+$");
        if (!std::regex_match(type, typeRegex))
            return false;

        return true;
    }

    std::shared_ptr<vm::StackItem> NameService::OnGetPrice(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create(GetPrice(engine.GetSnapshot()));
    }

    std::shared_ptr<vm::StackItem> NameService::OnSetPrice(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        // Check if caller is committee
        if (!CheckCommittee(engine))
            throw std::runtime_error("Not authorized");

        auto priceItem = args[0];
        auto price = priceItem->GetInteger();

        if (price <= 0)
            throw std::runtime_error("Invalid price");

        // Set price
        auto key = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(&price), sizeof(int64_t)));
        PutStorageValue(engine.GetSnapshot(), key, value);

        return vm::StackItem::Create(true);
    }

    std::shared_ptr<vm::StackItem> NameService::OnIsAvailable(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto nameItem = args[0];
        auto name = nameItem->GetString();

        return vm::StackItem::Create(IsAvailable(engine.GetSnapshot(), name));
    }
}
