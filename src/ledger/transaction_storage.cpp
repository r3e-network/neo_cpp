#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/transaction_storage.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>

#include <sstream>

namespace neo::ledger
{
// Storage prefixes
static const uint8_t TransactionPrefix = 0x03;
static const uint8_t UnspentOutputPrefix = 0x06;
static const uint8_t AddressOutputPrefix = 0x07;

TransactionStorage::TransactionStorage(std::shared_ptr<persistence::DataCache> dataCache) : dataCache_(dataCache) {}

std::shared_ptr<Transaction> TransactionStorage::GetTransaction(const io::UInt256& hash) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if the transaction is in memory
    auto it = transactions_.find(hash);
    if (it != transactions_.end()) return it->second;

    // Check if the transaction is in storage
    io::ByteVector prefixVector{TransactionPrefix};
    persistence::StorageKey key(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), hash.AsSpan()));
    auto item = dataCache_->TryGet(key);
    if (!item) return nullptr;

    // Deserialize the transaction
    std::istringstream stream(
        std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
    io::BinaryReader reader(stream);
    auto transaction = std::make_shared<Transaction>();
    transaction->Deserialize(reader);

    // Cache the transaction
    transactions_[hash] = transaction;

    return transaction;
}

bool TransactionStorage::AddTransaction(const Transaction& transaction,
                                        std::shared_ptr<persistence::DataCache> snapshot)
{
    std::lock_guard<std::mutex> lock(mutex_);

    try
    {
        // Store the transaction
        io::UInt256 hash = transaction.GetHash();
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        transaction.Serialize(writer);
        std::string data = stream.str();

        io::ByteVector prefixVector{TransactionPrefix};
        persistence::StorageKey txKey(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), hash.AsSpan()));
        persistence::StorageItem txItem(
            io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));
        snapshot->Add(txKey, txItem);

        // Store the unspent outputs
        for (uint32_t i = 0; i < transaction.GetOutputs().size(); i++)
        {
            const auto& output = transaction.GetOutputs()[i];

            // Store the unspent output
            std::ostringstream outputStream;
            io::BinaryWriter outputWriter(outputStream);
            output.Serialize(outputWriter);
            std::string outputData = outputStream.str();

            io::ByteVector outputPrefixVector{UnspentOutputPrefix};
            io::ByteVector outputIndexVector = io::ByteVector::FromUInt32(i);
            io::ByteVector temp1 = io::ByteVector::Concat(outputPrefixVector.AsSpan(), hash.AsSpan());
            persistence::StorageKey outputKey(io::UInt160(),
                                              io::ByteVector::Concat(temp1.AsSpan(), outputIndexVector.AsSpan()));
            persistence::StorageItem outputItem(
                io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(outputData.data()), outputData.size())));
            snapshot->Add(outputKey, outputItem);

            // Store the address output
            io::ByteVector addressPrefixVector{AddressOutputPrefix};
            io::ByteVector temp2 =
                io::ByteVector::Concat(addressPrefixVector.AsSpan(), output.GetScriptHash().AsSpan());
            io::ByteVector temp3 = io::ByteVector::Concat(temp2.AsSpan(), hash.AsSpan());
            persistence::StorageKey addressKey(io::UInt160(),
                                               io::ByteVector::Concat(temp3.AsSpan(), outputIndexVector.AsSpan()));
            persistence::StorageItem addressItem(
                io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(outputData.data()), outputData.size())));
            snapshot->Add(addressKey, addressItem);
        }

        // Cache the transaction
        auto txPtr = std::make_shared<Transaction>(transaction);
        transactions_[hash] = txPtr;

        // Cache the unspent outputs
        std::vector<TransactionOutput> outputs = transaction.GetOutputs();
        unspentOutputs_[hash] = outputs;

        // Cache the address outputs
        for (const auto& output : outputs)
        {
            auto& addressOutputs = addressOutputs_[output.GetScriptHash()];
            addressOutputs.push_back(output);
        }

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool TransactionStorage::ContainsTransaction(const io::UInt256& hash) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if the transaction is in memory
    if (transactions_.find(hash) != transactions_.end()) return true;

    // Check if the transaction is in storage
    io::ByteVector prefixVector{TransactionPrefix};
    persistence::StorageKey key(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), hash.AsSpan()));
    return dataCache_->TryGet(key) != nullptr;
}

std::vector<TransactionOutput> TransactionStorage::GetUnspentOutputs(const io::UInt256& hash) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if the unspent outputs are in memory
    auto it = unspentOutputs_.find(hash);
    if (it != unspentOutputs_.end()) return it->second;

    // Get the unspent outputs from storage
    std::vector<TransactionOutput> outputs;
    io::ByteVector prefixVector{UnspentOutputPrefix};
    persistence::StorageKey prefix(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), hash.AsSpan()));
    auto items = dataCache_->Find(&prefix);
    for (const auto& [key, item] : items)
    {
        std::istringstream stream(
            std::string(reinterpret_cast<const char*>(item.GetValue().Data()), item.GetValue().Size()));
        io::BinaryReader reader(stream);
        TransactionOutput output;
        output.Deserialize(reader);
        outputs.push_back(output);
    }

    // Cache the unspent outputs
    unspentOutputs_[hash] = outputs;

    return outputs;
}

std::vector<TransactionOutput> TransactionStorage::GetUnspentOutputs(const io::UInt160& scriptHash) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if the address outputs are in memory
    auto it = addressOutputs_.find(scriptHash);
    if (it != addressOutputs_.end()) return it->second;

    // Get the address outputs from storage
    std::vector<TransactionOutput> outputs;
    io::ByteVector prefixVector{AddressOutputPrefix};
    persistence::StorageKey prefix(io::UInt160(), io::ByteVector::Concat(prefixVector.AsSpan(), scriptHash.AsSpan()));
    auto items = dataCache_->Find(&prefix);
    for (const auto& [key, item] : items)
    {
        std::istringstream stream(
            std::string(reinterpret_cast<const char*>(item.GetValue().Data()), item.GetValue().Size()));
        io::BinaryReader reader(stream);
        TransactionOutput output;
        output.Deserialize(reader);
        outputs.push_back(output);
    }

    // Cache the address outputs
    addressOutputs_[scriptHash] = outputs;

    return outputs;
}

io::Fixed8 TransactionStorage::GetBalance(const io::UInt160& scriptHash, const io::UInt256& assetId) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Get the unspent outputs for the address
    auto outputs = GetUnspentOutputs(scriptHash);

    // Sum the outputs for the asset
    io::Fixed8 balance(0);
    for (const auto& output : outputs)
    {
        if (output.GetAssetId() == assetId)
        {
            balance += output.GetValue();
        }
    }

    return balance;
}
}  // namespace neo::ledger
