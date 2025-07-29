#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/ledger/transaction.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_cache.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>

using namespace neo::smartcontract;
using namespace neo::persistence;
using namespace neo::vm;
using namespace neo::io;
using namespace neo::ledger;

class ApplicationEngineTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        store_ = std::make_shared<MemoryStore>();
        snapshot_ = std::make_shared<StoreCache>(*store_);
        transaction_ = std::make_shared<Transaction>();
        engine_ = std::make_shared<ApplicationEngine>(TriggerType::Application, transaction_.get(), snapshot_);
    }

    std::shared_ptr<MemoryStore> store_;
    std::shared_ptr<StoreCache> snapshot_;
    std::shared_ptr<Transaction> transaction_;
    std::shared_ptr<ApplicationEngine> engine_;
};

TEST_F(ApplicationEngineTest, Constructor)
{
    EXPECT_EQ(engine_->GetTrigger(), TriggerType::Application);
    EXPECT_EQ(engine_->GetContainer(), transaction_.get());
    EXPECT_EQ(engine_->GetSnapshot(), snapshot_);
    EXPECT_EQ(engine_->GetPersistingBlock(), nullptr);
    EXPECT_EQ(engine_->GetGasConsumed(), 0);
    EXPECT_EQ(engine_->GetGasLeft(), -1);
    EXPECT_EQ(engine_->GetCurrentScriptHash(), UInt160());
    EXPECT_EQ(engine_->GetCallingScriptHash(), UInt160());
    EXPECT_EQ(engine_->GetEntryScriptHash(), UInt160());
    EXPECT_TRUE(engine_->GetNotifications().empty());
}

TEST_F(ApplicationEngineTest, LoadScript)
{
    // Create a simple script
    ByteVector script = ByteVector::Parse("0051");  // PUSH0, PUSH1

    // Load the script
    engine_->LoadScript(script);

    // Check the script hash
    UInt160 scriptHash = neo::cryptography::Hash::Hash160(script.AsSpan());
    EXPECT_EQ(engine_->GetCurrentScriptHash(), scriptHash);
    EXPECT_EQ(engine_->GetEntryScriptHash(), scriptHash);

    // Check the script
    auto actualScript = engine_->GetCurrentContext().GetScript().GetScript();
    EXPECT_EQ(actualScript.Size(), script.size());
    EXPECT_TRUE(std::equal(actualScript.begin(), actualScript.end(), script.Data()));
}

TEST_F(ApplicationEngineTest, Execute)
{
    // Create a simple script
    ByteVector script = ByteVector::Parse("0051");  // PUSH0, PUSH1

    // Load the script
    engine_->LoadScript(script);

    // Execute the script
    auto state = engine_->Execute();

    // Check the state
    EXPECT_EQ(state, VMState::Halt);

    // Check the result stack
    EXPECT_EQ(engine_->GetResultStack().size(), 2);
    EXPECT_EQ(engine_->GetResultStack()[0]->GetBoolean(), false);
    EXPECT_EQ(engine_->GetResultStack()[1]->GetInteger(), 1);
}

TEST_F(ApplicationEngineTest, HasFlag)
{
    // Check default flags
    EXPECT_TRUE(engine_->HasFlag(CallFlags::ReadStates));
    EXPECT_TRUE(engine_->HasFlag(CallFlags::WriteStates));
    EXPECT_TRUE(engine_->HasFlag(CallFlags::AllowCall));
    EXPECT_TRUE(engine_->HasFlag(CallFlags::AllowNotify));

    // Create an engine with specific flags
    auto engine = std::make_shared<ApplicationEngine>(TriggerType::Application, transaction_.get(), snapshot_);

    // Check flags
    EXPECT_TRUE(engine->HasFlag(CallFlags::ReadStates));
    EXPECT_TRUE(engine->HasFlag(CallFlags::WriteStates));
    EXPECT_TRUE(engine->HasFlag(CallFlags::AllowCall));
    EXPECT_TRUE(engine->HasFlag(CallFlags::AllowNotify));
}

TEST_F(ApplicationEngineTest, AddGas)
{
    // Create an engine with gas limit
    auto engine =
        std::make_shared<ApplicationEngine>(TriggerType::Application, transaction_.get(), snapshot_, nullptr, 100);

    // Check initial gas
    EXPECT_EQ(engine->GetGasConsumed(), 0);
    EXPECT_EQ(engine->GetGasLeft(), 100);

    // Add gas
    engine->AddGas(10);

    // Check gas
    EXPECT_EQ(engine->GetGasConsumed(), 10);
    EXPECT_EQ(engine->GetGasLeft(), 90);

    // Add more gas
    engine->AddGas(20);

    // Check gas
    EXPECT_EQ(engine->GetGasConsumed(), 30);
    EXPECT_EQ(engine->GetGasLeft(), 70);

    // Add too much gas
    EXPECT_THROW(engine->AddGas(100), std::runtime_error);

    // Add negative gas
    EXPECT_THROW(engine->AddGas(-10), std::invalid_argument);
}

TEST_F(ApplicationEngineTest, SystemCalls)
{
    // Create a script that calls System.Runtime.GetTrigger
    ByteVector script = ByteVector::Parse("800000000000000000000000000000000000");  // SYSCALL (hash)

    // Load the script
    engine_->LoadScript(script);

    // Execute the script
    auto state = engine_->Execute();

    // Check the state
    EXPECT_EQ(state, VMState::Halt);

    // Check the result stack
    EXPECT_EQ(engine_->GetResultStack().size(), 1);
    EXPECT_EQ(engine_->GetResultStack()[0]->GetInteger(), static_cast<int64_t>(TriggerType::Application));
}

TEST_F(ApplicationEngineTest, StorageOperations)
{
    // Create a script that calls System.Storage.Put and System.Storage.Get
    ByteVector script = ByteVector::Parse(
        "0c0576616c75650c036b6579800000000000000000000000000000000000000000"  // PUSHDATA1 "value", PUSHDATA1 "key",
                                                                              // SYSCALL (System.Storage.Put)
        "0c036b6579800000000000000000000000000000000000000000"  // PUSHDATA1 "key", SYSCALL (System.Storage.Get)
    );

    // Load the script
    engine_->LoadScript(script);

    // Execute the script
    auto state = engine_->Execute();

    // Check the state
    EXPECT_EQ(state, VMState::Halt);

    // Check the result stack
    EXPECT_EQ(engine_->GetResultStack().size(), 1);
    EXPECT_EQ(engine_->GetResultStack()[0]->GetString(), "value");

    // Check the storage
    StorageKey key(engine_->GetEntryScriptHash(), ByteVector(ByteSpan(reinterpret_cast<const uint8_t*>("key"), 3)));
    auto item = snapshot_->TryGet(key);
    EXPECT_TRUE(item != nullptr);
    EXPECT_EQ(std::string(reinterpret_cast<const char*>(item->GetValue().Data()), item->GetValue().Size()), "value");
}

TEST_F(ApplicationEngineTest, Notifications)
{
    // Create a script that calls System.Runtime.Notify
    ByteVector script = ByteVector::Parse(
        "0c0576616c75650c036b6579800000000000000000000000000000000000000000"  // PUSHDATA1 "value", PUSHDATA1 "key",
                                                                              // SYSCALL (System.Runtime.Notify)
    );

    // Load the script
    engine_->LoadScript(script);

    // Execute the script
    auto state = engine_->Execute();

    // Check the state
    EXPECT_EQ(state, VMState::Halt);

    // Check the notifications
    EXPECT_EQ(engine_->GetNotifications().size(), 1);
    EXPECT_EQ(engine_->GetNotifications()[0].script_hash, engine_->GetEntryScriptHash());
    EXPECT_EQ(engine_->GetNotifications()[0].state.size(), 2);
    EXPECT_EQ(engine_->GetNotifications()[0].state[0]->GetString(), "key");
    EXPECT_EQ(engine_->GetNotifications()[0].state[1]->GetString(), "value");
}
