#include <neo/smartcontract/native/notary.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/policy_contract.h>
#include <neo/smartcontract/native/ledger_contract.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/transaction_attribute.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/hardfork.h>
#include <algorithm>
#include <functional>
#include <cstring>

namespace neo::smartcontract::native
{
    Notary::Notary()
        : NativeContract("Notary", 13)
    {
    }

    std::shared_ptr<Notary> Notary::GetInstance()
    {
        static std::shared_ptr<Notary> instance = std::make_shared<Notary>();
        return instance;
    }

    uint32_t Notary::GetActiveInHardfork() const
    {
        return static_cast<uint32_t>(Hardfork::HF_Echidna);
    }

    void Notary::Initialize()
    {
        RegisterMethod("getMaxNotValidBeforeDelta", CallFlags::ReadStates, std::bind(&Notary::OnGetMaxNotValidBeforeDelta, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("setMaxNotValidBeforeDelta", CallFlags::States, std::bind(&Notary::OnSetMaxNotValidBeforeDelta, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("expirationOf", CallFlags::ReadStates, std::bind(&Notary::OnExpirationOf, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("balanceOf", CallFlags::ReadStates, std::bind(&Notary::OnBalanceOf, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("lockDepositUntil", CallFlags::States, std::bind(&Notary::OnLockDepositUntil, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("withdraw", CallFlags::All, std::bind(&Notary::OnWithdraw, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("verify", CallFlags::ReadStates, std::bind(&Notary::OnVerify, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("onNEP17Payment", CallFlags::States, std::bind(&Notary::OnNEP17PaymentAdapter, this, std::placeholders::_1, std::placeholders::_2));
    }

    bool Notary::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
    {
        if (hardfork == GetActiveInHardfork())
        {
            auto key = GetStorageKey(PREFIX_MAX_NOT_VALID_BEFORE_DELTA, io::ByteVector{});
            int32_t value = DEFAULT_MAX_NOT_VALID_BEFORE_DELTA;
            io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(int32_t)));
            PutStorageValue(engine.GetSnapshot(), key, valueBytes);
        }
        return true;
    }

    bool Notary::OnPersist(ApplicationEngine& engine)
    {
        int64_t nFees = 0;
        std::vector<cryptography::ecc::ECPoint> notaries;

        auto block = engine.GetPersistingBlock();
        if (!block)
            return true;

        for (const auto& tx : block->GetTransactions())
        {
            // Find NotaryAssisted attribute
            auto attr_it = std::find_if(tx->GetAttributes().begin(), tx->GetAttributes().end(),
                [](const ledger::TransactionAttribute& a) { 
                    return a.GetUsage() == ledger::TransactionAttribute::Usage::NotaryAssisted; 
                });
            if (attr_it == tx->GetAttributes().end())
                continue;
            
            auto attr = &(*attr_it);
            if (attr)
            {
                if (notaries.empty())
                    notaries = GetNotaryNodes(engine.GetSnapshot());

                // Extract nKeys from NotaryAssisted attribute data
                auto notaryAssistedAttr = tx->GetAttribute<ledger::NotaryAssisted>();
                auto nKeys = notaryAssistedAttr ? notaryAssistedAttr->GetNKeys() : 1;
                nFees += static_cast<int64_t>(nKeys) + 1;

                if (tx->GetSender() == GetScriptHash())
                {
                    auto payer = tx->GetSigners()[1];
                    auto key = GetStorageKey(PREFIX_DEPOSIT, payer.GetAccount());
                    auto item = engine.GetSnapshot()->GetAndChange(key);
                    if (item)
                    {
                        auto deposit = item->GetInteroperable<Deposit>();
                        if (deposit)
                        {
                            deposit->Amount -= tx->GetSystemFee() + tx->GetNetworkFee();
                            if (deposit->Amount == 0)
                                RemoveDepositFor(engine.GetSnapshot(), payer.GetAccount());
                        }
                    }
                }
            }
        }

        if (nFees == 0 || notaries.empty())
            return true;

        auto singleReward = CalculateNotaryReward(engine.GetSnapshot(), nFees, notaries.size());
        for (const auto& notary : notaries)
        {
            auto scriptHash = cryptography::Crypto::CreateSignatureRedeemScript(notary).ToScriptHash();
            auto gasToken = GasToken::GetInstance();
            gasToken->Mint(engine, scriptHash, singleReward, false);
        }

        return true;
    }

    bool Notary::PostPersist(ApplicationEngine& engine)
    {
        return true;
    }

    uint32_t Notary::GetMaxNotValidBeforeDelta(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_MAX_NOT_VALID_BEFORE_DELTA, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.empty())
            return DEFAULT_MAX_NOT_VALID_BEFORE_DELTA;

        return *reinterpret_cast<const uint32_t*>(value.Data());
    }

    bool Notary::SetMaxNotValidBeforeDelta(ApplicationEngine& engine, uint32_t value)
    {
        auto neoToken = NeoToken::GetInstance();
        auto committeeAddress = neoToken->GetCommitteeAddress(engine.GetSnapshot());
        if (!engine.CheckWitness(committeeAddress))
            return false;

        auto policyContract = PolicyContract::GetInstance();
        auto maxVUBIncrement = policyContract->GetMaxValidUntilBlockIncrement(engine.GetSnapshot());
        if (value > maxVUBIncrement / 2 || value < 7)
            return false;

        auto key = GetStorageKey(PREFIX_MAX_NOT_VALID_BEFORE_DELTA, io::ByteVector{});
        io::ByteVector valueBytes(io::ByteSpan(reinterpret_cast<const uint8_t*>(&value), sizeof(uint32_t)));
        PutStorageValue(engine.GetSnapshot(), key, valueBytes);

        return true;
    }

    uint32_t Notary::ExpirationOf(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const
    {
        auto deposit = GetDepositFor(snapshot, account);
        if (!deposit)
            return 0;

        return deposit->Till;
    }

    int64_t Notary::BalanceOf(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const
    {
        auto deposit = GetDepositFor(snapshot, account);
        if (!deposit)
            return 0;

        return deposit->Amount;
    }

    bool Notary::LockDepositUntil(ApplicationEngine& engine, const io::UInt160& account, uint32_t till)
    {
        if (!engine.CheckWitness(account))
            return false;

        auto ledgerContract = LedgerContract::GetInstance();
        auto currentIndex = ledgerContract->GetCurrentIndex(engine.GetSnapshot());
        if (till < currentIndex + 2)
            return false;

        auto deposit = GetDepositFor(engine.GetSnapshot(), account);
        if (!deposit || till < deposit->Till)
            return false;

        deposit->Till = till;
        PutDepositFor(engine, account, deposit);

        return true;
    }

    bool Notary::Withdraw(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to)
    {
        if (!engine.CheckWitness(from))
            return false;

        auto receive = to.IsZero() ? from : to;
        auto deposit = GetDepositFor(engine.GetSnapshot(), from);
        if (!deposit)
            return false;

        auto ledgerContract = LedgerContract::GetInstance();
        auto currentIndex = ledgerContract->GetCurrentIndex(engine.GetSnapshot());
        if (currentIndex < deposit->Till)
            return false;

        RemoveDepositFor(engine.GetSnapshot(), from);

        auto gasToken = GasToken::GetInstance();
        if (!gasToken->Transfer(engine, GetScriptHash(), receive, deposit->Amount, vm::StackItem::Null(), true))
            return false;

        return true;
    }

    bool Notary::Verify(ApplicationEngine& engine, const io::ByteVector& signature)
    {
        if (signature.Size() != 64)
            return false;

        auto tx = dynamic_cast<ledger::Transaction*>(engine.GetScriptContainer());
        if (!tx)
            return false;

        // Find NotaryAssisted attribute
        auto attr_it = std::find_if(tx->GetAttributes().begin(), tx->GetAttributes().end(),
            [](const ledger::TransactionAttribute& a) { 
                return a.GetUsage() == ledger::TransactionAttribute::Usage::NotaryAssisted; 
            });
        if (attr_it == tx->GetAttributes().end())
            return false;

        for (const auto& signer : tx->GetSigners())
        {
            if (signer.GetAccount() == GetScriptHash())
            {
                if (tx->GetSigners().size() < 2)
                    return false;

                auto payer = tx->GetSigners()[1];
                auto deposit = GetDepositFor(engine.GetSnapshot(), payer.GetAccount());
                if (!deposit)
                    return false;

                auto policyContract = PolicyContract::GetInstance();
                auto feePerKey = policyContract->GetAttributeFee(engine.GetSnapshot(), static_cast<uint8_t>(ledger::TransactionAttribute::Usage::NotaryAssisted));
                // Extract nKeys from NotaryAssisted attribute data
                auto notaryAssistedAttr = tx->GetAttribute<ledger::NotaryAssisted>();
                auto nKeys = notaryAssistedAttr ? notaryAssistedAttr->GetNKeys() : 1;
                auto requiredFee = (static_cast<int64_t>(nKeys) + 1) * feePerKey;
                if (deposit->Amount < tx->GetSystemFee() + tx->GetNetworkFee() + requiredFee)
                    return false;

                auto notaries = GetNotaryNodes(engine.GetSnapshot());
                for (const auto& notary : notaries)
                {
                    auto pubKey = cryptography::ecc::ECPoint::FromBytes(signature.AsSpan().subspan(0, 33));
                    if (pubKey == notary)
                    {
                        auto message = tx->GetHashData();
                        auto signatureData = signature.AsSpan().subspan(33, 31);
                        if (cryptography::Crypto::VerifySignature(message, signatureData, pubKey))
                            return true;
                    }
                }
            }
        }

        return false;
    }

    void Notary::OnNEP17Payment(ApplicationEngine& engine, const io::UInt160& from, int64_t amount, std::shared_ptr<vm::StackItem> data)
    {
        if (engine.GetCallingScriptHash() != GasToken::GetInstance()->GetScriptHash())
            throw std::invalid_argument("only GAS can be accepted for deposit");

        if (!data || !data->IsArray() || data->GetArray().size() != 2)
            throw std::invalid_argument("`data` parameter should be an array of 2 elements");

        auto to = from;
        auto additionalParams = data->GetArray();
        if (!additionalParams[0]->IsNull())
            to = io::UInt160::FromBytes(additionalParams[0]->GetByteArray().AsSpan());

        auto till = static_cast<uint32_t>(additionalParams[1]->GetInteger());
        auto tx = dynamic_cast<ledger::Transaction*>(engine.GetScriptContainer());
        auto allowedChangeTill = tx && tx->GetSender() == to;

        auto ledgerContract = LedgerContract::GetInstance();
        auto currentHeight = ledgerContract->GetCurrentIndex(engine.GetSnapshot());
        if (till < currentHeight + 2)
            throw std::out_of_range("`till` shouldn't be less than the chain's height + 1");

        auto key = GetStorageKey(PREFIX_DEPOSIT, to);
        auto item = engine.GetSnapshot()->GetAndChange(key);
        std::shared_ptr<Deposit> deposit;
        if (item)
        {
            deposit = item->GetInteroperable<Deposit>();
            if (deposit && till < deposit->Till)
                throw std::out_of_range("`till` shouldn't be less than the previous value");
        }

        if (!deposit)
        {
            auto policyContract = PolicyContract::GetInstance();
            auto feePerKey = policyContract->GetAttributeFee(engine.GetSnapshot(), network::payloads::TransactionAttributeType::NotaryAssisted);
            if (amount < 2 * feePerKey)
                throw std::out_of_range("first deposit can not be less than 2 * feePerKey");

            deposit = std::make_shared<Deposit>(0, 0);
            if (!allowedChangeTill)
                till = currentHeight + DEFAULT_DEPOSIT_DELTA_TILL;
        }
        else if (!allowedChangeTill)
        {
            till = deposit->Till;
        }

        deposit->Amount += amount;
        deposit->Till = till;
        PutDepositFor(engine, to, deposit);
    }

    std::vector<cryptography::ecc::ECPoint> Notary::GetNotaryNodes(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto neoToken = NeoToken::GetInstance();
        return neoToken->GetCommittee(snapshot);
    }

    int64_t Notary::CalculateNotaryReward(std::shared_ptr<persistence::StoreView> snapshot, int64_t nFees, int nNotaries) const
    {
        auto policyContract = PolicyContract::GetInstance();
        auto feePerKey = policyContract->GetAttributeFee(snapshot, network::payloads::TransactionAttributeType::NotaryAssisted);
        return nFees * feePerKey / nNotaries;
    }

    std::shared_ptr<Notary::Deposit> Notary::GetDepositFor(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const
    {
        auto key = GetStorageKey(PREFIX_DEPOSIT, account);
        auto item = snapshot->TryGet(key);
        if (!item)
            return nullptr;

        return item->GetInteroperable<Deposit>();
    }

    void Notary::PutDepositFor(ApplicationEngine& engine, const io::UInt160& account, std::shared_ptr<Deposit> deposit)
    {
        auto key = GetStorageKey(PREFIX_DEPOSIT, account);
        auto item = engine.GetSnapshot()->GetAndChange(key, [deposit]() { return std::make_shared<persistence::StorageItem>(deposit); });
        item->SetInteroperable(deposit);
    }

    void Notary::RemoveDepositFor(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account)
    {
        auto key = GetStorageKey(PREFIX_DEPOSIT, account);
        snapshot->Delete(key);
    }

    Notary::Deposit::Deposit()
        : Amount(0), Till(0)
    {
    }

    Notary::Deposit::Deposit(int64_t amount, uint32_t till)
        : Amount(amount), Till(till)
    {
    }

    void Notary::Deposit::FromStackItem(std::shared_ptr<vm::StackItem> stackItem)
    {
        auto structure = stackItem->GetStruct();
        Amount = structure[0]->GetInteger();
        Till = static_cast<uint32_t>(structure[1]->GetInteger());
    }

    std::shared_ptr<vm::StackItem> Notary::Deposit::ToStackItem(vm::IReferenceCounter* referenceCounter)
    {
        auto structure = vm::StackItem::CreateStruct(*referenceCounter);
        structure->Add(vm::StackItem::Create(Amount));
        structure->Add(vm::StackItem::Create(static_cast<int64_t>(Till)));
        return structure;
    }

    // Adapter method implementations
    std::shared_ptr<vm::StackItem> Notary::OnGetMaxNotValidBeforeDelta(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        auto result = GetMaxNotValidBeforeDelta(engine.GetSnapshot());
        return vm::StackItem::Create(static_cast<int64_t>(result));
    }

    std::shared_ptr<vm::StackItem> Notary::OnSetMaxNotValidBeforeDelta(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto value = static_cast<uint32_t>(args[0]->GetInteger());
        auto result = SetMaxNotValidBeforeDelta(engine, value);
        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> Notary::OnExpirationOf(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto accountBytes = args[0]->GetByteArray();
        if (accountBytes.Size() != 20)
            throw std::runtime_error("Invalid account");

        io::UInt160 account;
        memcpy(account.Data(), accountBytes.Data(), 20);

        auto result = ExpirationOf(engine.GetSnapshot(), account);
        return vm::StackItem::Create(static_cast<int64_t>(result));
    }

    std::shared_ptr<vm::StackItem> Notary::OnBalanceOf(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto accountBytes = args[0]->GetByteArray();
        if (accountBytes.Size() != 20)
            throw std::runtime_error("Invalid account");

        io::UInt160 account;
        std::memcpy(account.Data(), accountBytes.Data(), 20);

        auto result = BalanceOf(engine.GetSnapshot(), account);
        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> Notary::OnLockDepositUntil(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 2)
            throw std::runtime_error("Invalid arguments");

        auto accountBytes = args[0]->GetByteArray();
        if (accountBytes.Size() != 20)
            throw std::runtime_error("Invalid account");

        io::UInt160 account;
        std::memcpy(account.Data(), accountBytes.Data(), 20);

        auto till = static_cast<uint32_t>(args[1]->GetInteger());
        auto result = LockDepositUntil(engine, account, till);
        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> Notary::OnWithdraw(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 2)
            throw std::runtime_error("Invalid arguments");

        auto fromBytes = args[0]->GetByteArray();
        auto toBytes = args[1]->GetByteArray();
        
        if (fromBytes.Size() != 20 || toBytes.Size() != 20)
            throw std::runtime_error("Invalid account");

        io::UInt160 from, to;
        std::memcpy(from.Data(), fromBytes.Data(), 20);
        std::memcpy(to.Data(), toBytes.Data(), 20);

        auto result = Withdraw(engine, from, to);
        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> Notary::OnVerify(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto signature = args[0]->GetByteArray();
        auto result = Verify(engine, signature);
        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> Notary::OnNEP17PaymentAdapter(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 3)
            throw std::runtime_error("Invalid arguments");

        auto fromBytes = args[0]->GetByteArray();
        if (fromBytes.Size() != 20)
            throw std::runtime_error("Invalid from account");

        io::UInt160 from;
        std::memcpy(from.Data(), fromBytes.Data(), 20);

        auto amount = args[1]->GetInteger();
        auto data = args[2];

        OnNEP17Payment(engine, from, amount, data);
        return vm::StackItem::Null();
    }
}
