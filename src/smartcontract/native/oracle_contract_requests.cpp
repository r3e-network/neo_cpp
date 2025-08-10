#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/oracle_response.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/role_management.h>

#include <algorithm>
#include <iostream>
#include <sstream>

namespace neo::smartcontract::native
{
OracleRequest OracleContract::GetRequest(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id) const
{
    auto key = GetStorageKey(PREFIX_REQUEST,
                             io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
    auto value = GetStorageValue(snapshot, key);
    if (value.Size() == 0) throw std::runtime_error("Request not found");

    std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
    io::BinaryReader reader(stream);

    OracleRequest request;
    request.Deserialize(reader);

    return request;
}

std::vector<std::pair<uint64_t, OracleRequest>> OracleContract::GetRequests(
    std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto prefix = CreateStorageKey(PREFIX_REQUEST);
    auto results = snapshot->Find(&prefix);

    std::vector<std::pair<uint64_t, OracleRequest>> requests;
    for (const auto& entry : results)
    {
        auto key = entry.first.GetKey();
        auto value = entry.second.GetValue();

        // Extract ID from key (skip the prefix byte)
        if (key.Size() >= sizeof(uint64_t) + 1)
        {
            uint64_t id = *reinterpret_cast<const uint64_t*>(key.Data() + 1);

            // Deserialize request
            std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
            io::BinaryReader reader(stream);

            OracleRequest request;
            request.Deserialize(reader);

            requests.push_back(std::make_pair(id, request));
        }
    }

    return requests;
}

std::vector<std::pair<uint64_t, OracleRequest>> OracleContract::GetRequestsByUrl(
    std::shared_ptr<persistence::StoreView> snapshot, const std::string& url) const
{
    auto urlHash = GetUrlHash(url);
    auto idList = GetIdList(snapshot, urlHash);

    std::vector<std::pair<uint64_t, OracleRequest>> requests;
    for (const auto& id : idList.GetIds())
    {
        try
        {
            auto request = GetRequest(snapshot, id);
            requests.emplace_back(id, request);
        }
        catch (const std::exception&)
        {
            // Skip invalid requests
        }
    }

    return requests;
}

std::tuple<uint8_t, std::string> OracleContract::GetResponse(std::shared_ptr<persistence::StoreView> snapshot,
                                                             uint64_t id) const
{
    auto key = GetStorageKey(PREFIX_RESPONSE,
                             io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
    auto value = GetStorageValue(snapshot, key);
    if (value.Size() == 0) throw std::runtime_error("Response not found");

    std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
    io::BinaryReader reader(stream);
    uint8_t code = reader.ReadByte();
    std::string result = reader.ReadVarString();
    return std::make_tuple(code, result);
}

uint64_t OracleContract::GetNextRequestId(std::shared_ptr<persistence::StoreView> snapshot) const
{
    auto key = GetStorageKey(PREFIX_REQUEST_ID, io::ByteVector{});
    auto value = GetStorageValue(snapshot, key);
    if (value.Size() == 0)
    {
        uint64_t id = 1;
        PutStorageValue(snapshot, key,
                        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
        return id;
    }

    uint64_t id = *reinterpret_cast<const uint64_t*>(value.Data());
    uint64_t nextId = id + 1;
    PutStorageValue(snapshot, key,
                    io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&nextId), sizeof(uint64_t))));
    return id;
}

void OracleContract::AddRequestToIdList(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id)
{
    // Get the request
    auto request = GetRequest(snapshot, id);

    // Get the URL hash
    auto urlHash = GetUrlHash(request.GetUrl());

    // Get the ID list
    auto idList = GetIdList(snapshot, urlHash);

    // Add the ID to the list
    idList.Add(id);

    // Serialize the ID list
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    idList.Serialize(writer);
    std::string data = stream.str();

    // Store the ID list
    auto key = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(urlHash.AsSpan()));
    io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    PutStorageValue(snapshot, key, value);
}

void OracleContract::RemoveRequestFromIdList(std::shared_ptr<persistence::StoreView> snapshot, uint64_t id)
{
    // Get the request
    auto request = GetRequest(snapshot, id);

    // Get the URL hash
    auto urlHash = GetUrlHash(request.GetUrl());

    // Get the ID list
    auto idList = GetIdList(snapshot, urlHash);

    // Remove the ID from the list
    idList.Remove(id);

    // If the list is empty, delete it
    if (idList.GetCount() == 0)
    {
        auto key = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(urlHash.AsSpan()));
        DeleteStorageValue(snapshot, key);
        return;
    }

    // Serialize the ID list
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    idList.Serialize(writer);
    std::string data = stream.str();

    // Store the ID list
    auto key = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(urlHash.AsSpan()));
    io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    PutStorageValue(snapshot, key, value);
}

IdList OracleContract::GetIdList(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt256& urlHash) const
{
    auto key = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(urlHash.AsSpan()));
    auto value = GetStorageValue(snapshot, key);
    if (value.Size() == 0) return IdList();

    std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
    io::BinaryReader reader(stream);

    IdList idList;
    idList.Deserialize(reader);

    return idList;
}

io::UInt256 OracleContract::GetUrlHash(const std::string& url)
{
    return neo::cryptography::Hash::Hash256(io::ByteSpan(reinterpret_cast<const uint8_t*>(url.data()), url.size()));
}

uint64_t OracleContract::CreateRequest(std::shared_ptr<persistence::StoreView> snapshot, const std::string& url,
                                       const std::string& filter, const io::UInt160& callback,
                                       const std::string& callbackMethod, int64_t gasForResponse,
                                       const io::ByteVector& userData, const io::UInt256& originalTxid)
{
    // Validate inputs
    if (url.empty() || url.length() > MAX_URL_LENGTH) throw std::runtime_error("Invalid URL");

    if (filter.length() > MAX_FILTER_LENGTH) throw std::runtime_error("Filter too long");

    if (callbackMethod.empty() || callbackMethod.length() > MAX_CALLBACK_LENGTH)
        throw std::runtime_error("Invalid callback method");

    if (userData.Size() > MAX_USER_DATA_LENGTH) throw std::runtime_error("User data too large");

    if (gasForResponse < 0) throw std::runtime_error("Gas for response must be non-negative");

    // Create request
    uint64_t id = GetNextRequestId(snapshot);

    // Create OracleRequest object
    OracleRequest request(originalTxid, gasForResponse, url, filter, callback, callbackMethod, userData);

    // Serialize the request
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    request.Serialize(writer);
    std::string data = stream.str();

    // Store the request
    auto key = GetStorageKey(PREFIX_REQUEST,
                             io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(&id), sizeof(uint64_t))));
    io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    PutStorageValue(snapshot, key, value);

    // Add to ID list
    auto urlHash = GetUrlHash(url);
    auto idList = GetIdList(snapshot, urlHash);

    // Check if there are too many pending requests for this URL
    if (idList.GetCount() >= 256) throw std::runtime_error("Too many pending requests for this URL");

    // Add the ID to the list
    idList.Add(id);

    // Serialize the ID list
    std::ostringstream idListStream;
    io::BinaryWriter idListWriter(idListStream);
    idList.Serialize(idListWriter);
    std::string idListData = idListStream.str();

    // Store the ID list
    auto idListKey = GetStorageKey(PREFIX_ID_LIST, io::ByteVector(urlHash.AsSpan()));
    io::ByteVector idListValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(idListData.data()), idListData.size()));
    PutStorageValue(snapshot, idListKey, idListValue);

    return id;
}
}  // namespace neo::smartcontract::native
