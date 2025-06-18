#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/ledger/oracle_response.h>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace neo::smartcontract::native
{
    OracleContract::OracleContract()
        : NativeContract(NAME, ID)
    {
    }

    std::shared_ptr<OracleContract> OracleContract::GetInstance()
    {
        static std::shared_ptr<OracleContract> instance = std::make_shared<OracleContract>();
        return instance;
    }

    void OracleContract::Initialize()
    {
        RegisterMethod("getPrice", CallFlags::ReadStates, std::bind(&OracleContract::OnGetPrice, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("setPrice", CallFlags::States, std::bind(&OracleContract::OnSetPrice, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getOracles", CallFlags::ReadStates, std::bind(&OracleContract::OnGetOracles, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("setOracles", CallFlags::States, std::bind(&OracleContract::OnSetOracles, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("request", CallFlags::States | CallFlags::AllowCall | CallFlags::AllowNotify, std::bind(&OracleContract::OnRequest, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("finish", CallFlags::States | CallFlags::AllowCall | CallFlags::AllowNotify, std::bind(&OracleContract::OnFinish, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("verify", CallFlags::ReadStates, std::bind(&OracleContract::OnVerify, this, std::placeholders::_1, std::placeholders::_2));
    }

    int64_t OracleContract::GetPrice(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.Size() == 0)
            return DEFAULT_PRICE;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    std::vector<io::UInt160> OracleContract::GetOracles(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_ORACLE, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.Size() == 0)
            return {};

        std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
        io::BinaryReader reader(stream);
        uint32_t count = static_cast<uint32_t>(reader.ReadVarInt());
        std::vector<io::UInt160> oracles;
        oracles.reserve(count);
        for (uint32_t i = 0; i < count; i++)
        {
            oracles.push_back(reader.ReadSerializable<io::UInt160>());
        }
        return oracles;
    }

    void OracleContract::SetPrice(std::shared_ptr<persistence::StoreView> snapshot, int64_t price)
    {
        if (price <= 0)
            throw std::runtime_error("Price must be positive");

        auto key = GetStorageKey(PREFIX_PRICE, io::ByteVector{});
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(&price), sizeof(int64_t)));
        PutStorageValue(snapshot, key, value);
    }

    void OracleContract::SetOracles(std::shared_ptr<persistence::StoreView> snapshot, const std::vector<io::UInt160>& oracles)
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        writer.WriteVarInt(oracles.size());
        for (const auto& oracle : oracles)
        {
            writer.Write(oracle);
        }
        std::string data = stream.str();

        auto key = GetStorageKey(PREFIX_ORACLE, io::ByteVector{});
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
        PutStorageValue(snapshot, key, value);
    }
}
