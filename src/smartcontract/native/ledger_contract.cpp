#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/hardfork.h>
#include <sstream>

namespace neo::smartcontract::native
{
    std::shared_ptr<LedgerContract> LedgerContract::GetInstance()
    {
        static std::shared_ptr<LedgerContract> instance = std::make_shared<LedgerContract>();
        return instance;
    }

    LedgerContract::LedgerContract()
        : NativeContract(NAME, ID)
        , currentBlockKey_(CreateStorageKey(PREFIX_CURRENT_BLOCK))
    {
    }

    void LedgerContract::Initialize()
    {
        RegisterMethod("getHash", CallFlags::ReadStates, std::bind(&LedgerContract::OnGetHash, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getBlock", CallFlags::ReadStates, std::bind(&LedgerContract::OnGetBlock, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getTransaction", CallFlags::ReadStates, std::bind(&LedgerContract::OnGetTransaction, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getTransactionHeight", CallFlags::ReadStates, std::bind(&LedgerContract::OnGetTransactionHeight, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getCurrentIndex", CallFlags::ReadStates, std::bind(&LedgerContract::OnGetCurrentIndex, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("getCurrentHash", CallFlags::ReadStates, std::bind(&LedgerContract::OnGetCurrentHash, this, std::placeholders::_1, std::placeholders::_2));
    }

    std::shared_ptr<vm::StackItem> LedgerContract::OnGetHash(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto indexItem = args[0];
        auto index = indexItem->GetInteger();

        // Get block hash
        auto hash = GetBlockHash(engine.GetSnapshot(), index);
        if (hash.IsZero())
            return vm::StackItem::Create(nullptr);

        return vm::StackItem::Create(io::ByteVector(io::ByteSpan(hash.Data(), io::UInt256::Size)));
    }

    std::shared_ptr<vm::StackItem> LedgerContract::OnGetBlock(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto hashOrIndexItem = args[0];
        std::shared_ptr<ledger::Block> block;

        if (hashOrIndexItem->GetType() == vm::StackItemType::Integer)
        {
            // Get block by index
            auto index = static_cast<uint32_t>(hashOrIndexItem->GetInteger());

            // Check if block is traceable
            if (!IsTraceableBlock(engine, index))
                return vm::StackItem::Create(nullptr);

            auto hash = GetBlockHash(engine.GetSnapshot(), index);
            if (!hash.IsZero())
                block = GetBlock(engine.GetSnapshot(), hash);
        }
        else
        {
            // Get block by hash
            auto hashBytes = hashOrIndexItem->GetByteArray();
            if (hashBytes.Size() != 32)
                throw std::runtime_error("Invalid hash");

            io::UInt256 hash;
            std::memcpy(hash.Data(), hashBytes.Data(), 32);

            block = GetBlock(engine.GetSnapshot(), hash);

            // Check if block is traceable
            if (block && !IsTraceableBlock(engine, block->GetIndex()))
                return vm::StackItem::Create(nullptr);
        }

        if (!block)
            return vm::StackItem::Create(nullptr);

        return BlockToStackItem(block);
    }

    std::shared_ptr<vm::StackItem> LedgerContract::OnGetTransaction(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto hashItem = args[0];
        auto hashBytes = hashItem->GetByteArray();

        if (hashBytes.Size() != 32)
            throw std::runtime_error("Invalid hash");

        io::UInt256 hash;
        std::memcpy(hash.Data(), hashBytes.Data(), 32);

        // Get transaction
        auto tx = GetTransaction(engine.GetSnapshot(), hash);
        if (!tx)
            return vm::StackItem::Create(nullptr);

        // Check if transaction's block is traceable
        int32_t height = GetTransactionHeight(engine.GetSnapshot(), hash);
        if (height < 0 || !IsTraceableBlock(engine, height))
            return vm::StackItem::Create(nullptr);

        return TransactionToStackItem(tx);
    }

    std::shared_ptr<vm::StackItem> LedgerContract::OnGetTransactionHeight(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto hashItem = args[0];
        auto hashBytes = hashItem->GetByteArray();

        if (hashBytes.Size() != 32)
            throw std::runtime_error("Invalid hash");

        io::UInt256 hash;
        std::memcpy(hash.Data(), hashBytes.Data(), 32);

        // Get transaction height
        auto height = GetTransactionHeight(engine.GetSnapshot(), hash);
        if (height < 0)
            return vm::StackItem::Create(nullptr);

        // Check if transaction's block is traceable
        if (!IsTraceableBlock(engine, height))
            return vm::StackItem::Create(nullptr);

        return vm::StackItem::Create(static_cast<int64_t>(height));
    }

    std::shared_ptr<vm::StackItem> LedgerContract::OnGetCurrentIndex(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // Get current index
        auto index = GetCurrentIndex(engine.GetSnapshot());
        return vm::StackItem::Create(static_cast<int64_t>(index));
    }

    std::shared_ptr<vm::StackItem> LedgerContract::OnGetCurrentHash(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // Get current hash
        auto hash = GetCurrentHash(engine.GetSnapshot());
        return vm::StackItem::Create(io::ByteVector(io::ByteSpan(hash.Data(), io::UInt256::Size)));
    }

    std::shared_ptr<vm::StackItem> LedgerContract::BlockToStackItem(std::shared_ptr<ledger::Block> block) const
    {
        // Create block stack item
        std::vector<std::shared_ptr<vm::StackItem>> items;
        items.push_back(vm::StackItem::Create(block->GetHash()));
        items.push_back(vm::StackItem::Create(static_cast<int64_t>(block->GetVersion())));
        items.push_back(vm::StackItem::Create(block->GetPreviousHash()));
        items.push_back(vm::StackItem::Create(block->GetMerkleRoot()));
        items.push_back(vm::StackItem::Create(static_cast<int64_t>(block->GetTimestamp().time_since_epoch().count())));
        items.push_back(vm::StackItem::Create(static_cast<int64_t>(block->GetIndex())));
        items.push_back(vm::StackItem::Create(block->GetNextConsensus()));
        items.push_back(vm::StackItem::Create(static_cast<int64_t>(1))); // Block has single witness
        
        auto result = vm::StackItem::Create(items);

        return result;
    }

    std::shared_ptr<vm::StackItem> LedgerContract::TransactionToStackItem(std::shared_ptr<ledger::Transaction> tx) const
    {
        // Create transaction stack item
        auto result = vm::StackItem::Create(std::vector<std::shared_ptr<vm::StackItem>>{
            vm::StackItem::Create(tx->GetHash()),
            vm::StackItem::Create(static_cast<int64_t>(tx->GetVersion())),
            vm::StackItem::Create(static_cast<int64_t>(tx->GetNonce())),
            vm::StackItem::Create(tx->GetSender()),
            vm::StackItem::Create(static_cast<int64_t>(tx->GetSystemFee())),
            vm::StackItem::Create(static_cast<int64_t>(tx->GetNetworkFee())),
            vm::StackItem::Create(static_cast<int64_t>(tx->GetValidUntilBlock())),
            vm::StackItem::Create(tx->GetScript()),
            vm::StackItem::Create(static_cast<int64_t>(tx->GetWitnesses().size()))
        });

        return result;
    }

    bool LedgerContract::IsInitialized(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        if (!snapshot)
            throw std::invalid_argument("snapshot");

        auto prefix = CreateStorageKey(PREFIX_BLOCK);
        auto results = snapshot->Find(&prefix);
        return !results.empty();
    }

    bool LedgerContract::IsTraceableBlock(ApplicationEngine& engine, uint32_t index) const
    {
        uint32_t maxTraceableBlocks = engine.GetProtocolSettings()->GetMaxTraceableBlocks();

        // Check if Echidna hardfork is enabled
        if (engine.IsHardforkEnabled(static_cast<int>(Hardfork::HF_Echidna)))
        {
            // Get max traceable blocks from policy contract
            auto policyContract = PolicyContract::GetInstance();
            maxTraceableBlocks = policyContract->GetMaxTraceableBlocks(engine.GetSnapshot());
        }

        return IsTraceableBlock(engine.GetSnapshot(), index, maxTraceableBlocks);
    }

    bool LedgerContract::IsTraceableBlock(std::shared_ptr<persistence::DataCache> snapshot, uint32_t index, uint32_t maxTraceableBlocks) const
    {
        if (!snapshot)
            throw std::invalid_argument("snapshot");

        uint32_t currentIndex = GetCurrentIndex(snapshot);
        return index <= currentIndex && index + maxTraceableBlocks > currentIndex;
    }

    io::UInt256 LedgerContract::GetCurrentHash(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        if (!snapshot)
            throw std::invalid_argument("snapshot");

        auto item = snapshot->TryGet(currentBlockKey_);
        if (!item)
            return io::UInt256();

        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);

        HashIndexState state;
        state.Deserialize(reader);

        return state.GetHash();
    }

    uint32_t LedgerContract::GetCurrentIndex(std::shared_ptr<persistence::DataCache> snapshot) const
    {
        if (!snapshot)
            throw std::invalid_argument("snapshot");

        auto item = snapshot->TryGet(currentBlockKey_);
        if (!item)
            return 0;

        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);

        HashIndexState state;
        state.Deserialize(reader);

        return state.GetIndex();
    }

    io::UInt256 LedgerContract::GetBlockHash(std::shared_ptr<persistence::DataCache> snapshot, uint32_t index) const
    {
        if (!snapshot)
            throw std::invalid_argument("snapshot");

        auto key = CreateStorageKey(PREFIX_BLOCK_HASH, index);
        auto item = snapshot->TryGet(key);
        if (!item)
            return io::UInt256();

        io::UInt256 hash;
        std::memcpy(hash.Data(), item->GetValue().Data(), io::UInt256::Size);

        return hash;
    }

    std::shared_ptr<ledger::Block> LedgerContract::GetBlock(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt256& hash) const
    {
        if (!snapshot)
            throw std::invalid_argument("snapshot");

        auto key = CreateStorageKey(PREFIX_BLOCK, io::ByteVector(io::ByteSpan(hash.Data(), io::UInt256::Size)));
        auto item = snapshot->TryGet(key);
        if (!item)
            return nullptr;

        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);

        auto block = std::make_shared<ledger::Block>();
        block->Deserialize(reader);

        return block;
    }

    std::shared_ptr<ledger::Transaction> LedgerContract::GetTransaction(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt256& hash) const
    {
        if (!snapshot)
            throw std::invalid_argument("snapshot");

        auto key = CreateStorageKey(PREFIX_TRANSACTION, io::ByteVector(io::ByteSpan(hash.Data(), io::UInt256::Size)));
        auto item = snapshot->TryGet(key);
        if (!item)
            return nullptr;

        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);

        auto tx = std::make_shared<ledger::Transaction>();
        tx->Deserialize(reader);

        return tx;
    }

    int32_t LedgerContract::GetTransactionHeight(std::shared_ptr<persistence::DataCache> snapshot, const io::UInt256& hash) const
    {
        if (!snapshot)
            throw std::invalid_argument("snapshot");

        auto key = CreateStorageKey(PREFIX_TRANSACTION, io::ByteVector(io::ByteSpan(hash.Data(), io::UInt256::Size)));
        auto item = snapshot->TryGet(key);
        if (!item)
            return -1;

        std::istringstream stream(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()));
        io::BinaryReader reader(stream);

        // Skip transaction data
        auto tx = std::make_shared<ledger::Transaction>();
        tx->Deserialize(reader);

        // Read block index
        uint32_t blockIndex = reader.ReadUInt32();

        return static_cast<int32_t>(blockIndex);
    }

    bool LedgerContract::OnPersist(ApplicationEngine& engine)
    {
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Store block hash
        auto blockHashKey = CreateStorageKey(PREFIX_BLOCK_HASH, block->GetIndex());
        auto blockHashItem = persistence::StorageItem(io::ByteVector(io::ByteSpan(block->GetHash().Data(), io::UInt256::Size)));
        engine.GetSnapshot()->Add(blockHashKey, blockHashItem);

        // Store block
        auto blockKey = CreateStorageKey(PREFIX_BLOCK, io::ByteVector(io::ByteSpan(block->GetHash().Data(), io::UInt256::Size)));

        std::ostringstream blockStream;
        io::BinaryWriter blockWriter(blockStream);
        block->Serialize(blockWriter);
        std::string blockData = blockStream.str();

        auto blockItem = persistence::StorageItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(blockData.data()), blockData.size())));
        engine.GetSnapshot()->Add(blockKey, blockItem);

        // Store transactions
        for (const auto& tx : block->GetTransactions())
        {
            auto txKey = CreateStorageKey(PREFIX_TRANSACTION, io::ByteVector(io::ByteSpan(tx.GetHash().Data(), io::UInt256::Size)));

            std::ostringstream txStream;
            io::BinaryWriter txWriter(txStream);
            tx.Serialize(txWriter);
            txWriter.Write(block->GetIndex());
            std::string txData = txStream.str();

            auto txItem = persistence::StorageItem(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(txData.data()), txData.size())));
            engine.GetSnapshot()->Add(txKey, txItem);
        }

        return true;
    }

    bool LedgerContract::PostPersist(ApplicationEngine& engine)
    {
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Update current block
        auto item = engine.GetSnapshot()->GetAndChange(currentBlockKey_, []() {
            return std::make_shared<persistence::StorageItem>(io::ByteVector{});
        });

        HashIndexState state(block->GetHash(), block->GetIndex());

        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        state.Serialize(writer);
        std::string data = stream.str();

        item->SetValue(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size())));

        return true;
    }
}
