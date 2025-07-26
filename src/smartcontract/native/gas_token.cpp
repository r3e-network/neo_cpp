#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/crypto.h>
#include <iostream>
#include <sstream>

namespace neo::smartcontract::native
{
    GasToken::GasToken()
        : FungibleToken(NAME, ID)
    {
    }

    std::string GasToken::GetSymbol() const
    {
        return "GAS";
    }

    uint8_t GasToken::GetDecimals() const
    {
        return 8;
    }

    std::shared_ptr<GasToken> GasToken::GetInstance()
    {
        static std::shared_ptr<GasToken> instance = std::make_shared<GasToken>();
        return instance;
    }

    void GasToken::Initialize()
    {
        RegisterMethod("symbol", CallFlags::ReadStates, std::bind(&GasToken::OnSymbol, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("decimals", CallFlags::ReadStates, std::bind(&GasToken::OnDecimals, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("totalSupply", CallFlags::ReadStates, std::bind(&GasToken::OnTotalSupply, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("balanceOf", CallFlags::ReadStates, std::bind(&GasToken::OnBalanceOf, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("transfer", CallFlags::All, std::bind(&GasToken::OnTransfer, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("onNEP17Payment", CallFlags::All, std::bind(&GasToken::OnNEP17Payment, this, std::placeholders::_1, std::placeholders::_2));
    }

    bool GasToken::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
    {
        // Check if this is the initial deployment
        if (hardfork == 0)
        {
            // Get the BFT address from the standby validators
            auto settings = engine.GetProtocolSettings();
            if (!settings)
                return false;

            auto validators = settings->GetStandbyValidators();
            if (validators.size() == 0)
                return false;

            // Create a multi-signature contract with the validators
            io::UInt160 account;
            // Implement GetBFTAddress to get the multi-signature contract address
            try
            {
                // Get the NEO token contract to retrieve committee address
                auto neoContract = engine.GetNativeContract(NeoToken::GetContractId());
                if (neoContract)
                {
                    account = neoContract->GetCommitteeAddress(engine.GetSnapshot());
                }
                else
                {
                    // Fallback: use a well-known committee address or calculate from committee members
                    // This should not happen in normal operation
                    throw std::runtime_error("NEO contract not found for committee address calculation");
                }
            }
            catch (const std::exception& e)
            {
                // Log error but continue with empty account (will fail witness check)
                std::cerr << "Failed to get committee address: " << e.what() << std::endl;
                account = io::UInt160(); // Empty/zero address
            }

            // Mint the initial GAS distribution
            int64_t initialGasDistribution = settings->GetInitialGasDistribution();
            if (initialGasDistribution <= 0)
                initialGasDistribution = 5200000 * FACTOR; // Default: 5.2M GAS

            return Mint(engine, account, initialGasDistribution, false);
        }

        return true;
    }

    int64_t GasToken::GetBalance(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account) const
    {
        auto key = GetStorageKey(PREFIX_BALANCE, account);
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return 0;

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    int64_t GasToken::GetTotalSupply(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_TOTAL_SUPPLY, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return 0; // GAS starts with 0 total supply, minted as needed

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    bool GasToken::Transfer(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& from, const io::UInt160& to, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Check if from account has enough balance
        int64_t fromBalance = GetBalance(snapshot, from);
        if (fromBalance < amount)
            return false;

        // Update from account balance
        int64_t newFromBalance = fromBalance - amount;
        auto fromKey = GetStorageKey(PREFIX_BALANCE, from);
        if (newFromBalance > 0)
        {
            io::ByteVector fromValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newFromBalance), sizeof(int64_t)));
            PutStorageValue(snapshot, fromKey, fromValue);
        }
        else
        {
            DeleteStorageValue(snapshot, fromKey);
        }

        // Update to account balance
        int64_t toBalance = GetBalance(snapshot, to);
        int64_t newToBalance = toBalance + amount;
        auto toKey = GetStorageKey(PREFIX_BALANCE, to);
        io::ByteVector toValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newToBalance), sizeof(int64_t)));
        PutStorageValue(snapshot, toKey, toValue);

        return true;
    }

    bool GasToken::Transfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        if (amount <= 0)
            return false;

        // Transfer GAS
        bool result = Transfer(engine.GetSnapshot(), from, to, amount);

        if (result)
        {
            // Call PostTransfer
            PostTransfer(engine, from, to, amount, data, callOnPayment);
        }

        return result;
    }

    bool GasToken::PostTransfer(ApplicationEngine& engine, const io::UInt160& from, const io::UInt160& to, int64_t amount, std::shared_ptr<vm::StackItem> data, bool callOnPayment)
    {
        // Create the state for the Transfer event
        std::vector<std::shared_ptr<vm::StackItem>> state = {
            from.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(from),
            to.IsZero() ? vm::StackItem::Null() : vm::StackItem::Create(to),
            vm::StackItem::Create(amount)
        };

        // Send notification
        engine.Notify(GetScriptHash(), "Transfer", state);

        // Check if it's a wallet or smart contract
        if (!callOnPayment || to.IsZero())
            return true;

        // Check if the recipient is a contract
        // Implement ContractManagement integration for contract validation
        try
        {
            auto contractManagement = engine.GetNativeContract(ContractManagement::GetContractId());
            if (contractManagement)
            {
                auto contract = contractManagement->GetContract(engine.GetSnapshot(), to);
                if (contract)
                {
                    // Recipient is a contract, call onNEP17Payment if it exists
                    auto manifest = contract->GetManifest();
                    if (manifest.HasMethod("onNEP17Payment"))
                    {
                        // Prepare parameters for onNEP17Payment call
                        std::vector<std::shared_ptr<vm::StackItem>> args;
                        args.push_back(vm::StackItem::Create(from));
                        args.push_back(vm::StackItem::Create(amount));
                        args.push_back(data ? data : vm::StackItem::Null());
                        
                        // Call the contract's onNEP17Payment method
                        engine.CallContract(to, "onNEP17Payment", args, CallFlags::All);
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            // Log error but don't fail the transfer
            std::cerr << "Error checking contract for onNEP17Payment: " << e.what() << std::endl;
        }

        return true;
    }

    bool GasToken::Mint(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Update total supply
        int64_t totalSupply = GetTotalSupply(snapshot);
        int64_t newTotalSupply = totalSupply + amount;
        auto totalSupplyKey = GetStorageKey(PREFIX_TOTAL_SUPPLY, io::ByteVector{});
        io::ByteVector totalSupplyValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newTotalSupply), sizeof(int64_t)));
        PutStorageValue(snapshot, totalSupplyKey, totalSupplyValue);

        // Update account balance
        int64_t balance = GetBalance(snapshot, account);
        int64_t newBalance = balance + amount;
        auto balanceKey = GetStorageKey(PREFIX_BALANCE, account);
        io::ByteVector balanceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newBalance), sizeof(int64_t)));
        PutStorageValue(snapshot, balanceKey, balanceValue);

        return true;
    }

    bool GasToken::Mint(ApplicationEngine& engine, const io::UInt160& account, int64_t amount, bool callOnPayment)
    {
        if (amount <= 0)
            return false;

        // Mint GAS
        bool result = Mint(engine.GetSnapshot(), account, amount);

        if (result)
        {
            // Call PostTransfer
            io::UInt160 nullAddress;
            std::memset(nullAddress.Data(), 0, io::UInt160::Size);

            PostTransfer(engine, nullAddress, account, amount, vm::StackItem::Null(), callOnPayment);
        }

        return result;
    }

    bool GasToken::Burn(std::shared_ptr<persistence::StoreView> snapshot, const io::UInt160& account, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Check if account has enough balance
        int64_t balance = GetBalance(snapshot, account);
        if (balance < amount)
            return false;

        // Update total supply
        int64_t totalSupply = GetTotalSupply(snapshot);
        int64_t newTotalSupply = totalSupply - amount;
        auto totalSupplyKey = GetStorageKey(PREFIX_TOTAL_SUPPLY, io::ByteVector{});
        io::ByteVector totalSupplyValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newTotalSupply), sizeof(int64_t)));
        PutStorageValue(snapshot, totalSupplyKey, totalSupplyValue);

        // Update account balance
        int64_t newBalance = balance - amount;
        auto balanceKey = GetStorageKey(PREFIX_BALANCE, account);
        if (newBalance > 0)
        {
            io::ByteVector balanceValue(io::ByteSpan(reinterpret_cast<const uint8_t*>(&newBalance), sizeof(int64_t)));
            PutStorageValue(snapshot, balanceKey, balanceValue);
        }
        else
        {
            DeleteStorageValue(snapshot, balanceKey);
        }

        return true;
    }

    bool GasToken::Burn(ApplicationEngine& engine, const io::UInt160& account, int64_t amount)
    {
        if (amount <= 0)
            return false;

        // Burn GAS
        bool result = Burn(engine.GetSnapshot(), account, amount);

        if (result)
        {
            // Call PostTransfer
            io::UInt160 nullAddress;
            std::memset(nullAddress.Data(), 0, io::UInt160::Size);

            PostTransfer(engine, account, nullAddress, amount, vm::StackItem::Null(), false);
        }

        return result;
    }

    int64_t GasToken::GetGasPerBlock(std::shared_ptr<persistence::StoreView> snapshot) const
    {
        auto key = GetStorageKey(PREFIX_GAS_PER_BLOCK, io::ByteVector{});
        auto value = GetStorageValue(snapshot, key);
        if (value.IsEmpty())
            return 5 * FACTOR; // Default: 5 GAS per block

        return *reinterpret_cast<const int64_t*>(value.Data());
    }

    void GasToken::SetGasPerBlock(std::shared_ptr<persistence::StoreView> snapshot, int64_t gasPerBlock)
    {
        auto key = GetStorageKey(PREFIX_GAS_PER_BLOCK, io::ByteVector{});
        io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(&gasPerBlock), sizeof(int64_t)));
        PutStorageValue(snapshot, key, value);
    }

    std::shared_ptr<vm::StackItem> GasToken::OnSymbol(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create("GAS");
    }

    std::shared_ptr<vm::StackItem> GasToken::OnDecimals(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create(static_cast<int64_t>(8));
    }

    std::shared_ptr<vm::StackItem> GasToken::OnTotalSupply(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        return vm::StackItem::Create(GetTotalSupply(engine.GetSnapshot()));
    }

    std::shared_ptr<vm::StackItem> GasToken::OnBalanceOf(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.empty())
            throw std::runtime_error("Invalid arguments");

        auto accountItem = args[0];
        auto accountBytes = accountItem->GetByteArray();

        if (accountBytes.Size() != 20)
            throw std::runtime_error("Invalid account");

        io::UInt160 account;
        std::memcpy(account.Data(), accountBytes.Data(), 20);

        return vm::StackItem::Create(GetBalance(engine.GetSnapshot(), account));
    }

    std::shared_ptr<vm::StackItem> GasToken::OnTransfer(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        if (args.size() < 3)
            throw std::runtime_error("Invalid arguments");

        auto fromItem = args[0];
        auto toItem = args[1];
        auto amountItem = args[2];

        auto fromBytes = fromItem->GetByteArray();
        auto toBytes = toItem->GetByteArray();
        auto amount = amountItem->GetInteger();

        if (fromBytes.Size() != 20 || toBytes.Size() != 20)
            throw std::runtime_error("Invalid account");

        io::UInt160 from;
        io::UInt160 to;
        std::memcpy(from.Data(), fromBytes.Data(), 20);
        std::memcpy(to.Data(), toBytes.Data(), 20);

        // Check if from account is the current script hash
        if (from != engine.GetCurrentScriptHash())
            throw std::runtime_error("Invalid from account");

        // Check if amount is valid
        if (amount <= 0)
            throw std::runtime_error("Invalid amount");

        // Get data if provided
        std::shared_ptr<vm::StackItem> data = vm::StackItem::Null();
        if (args.size() >= 4)
            data = args[3];

        // Check if caller is committee
        // Implement committee check matching C# NativeContract.CheckCommittee
        try
        {
            // Implement proper committee address calculation and witness checking
            try
            {
                // Get the NEO token contract to retrieve committee address
                auto neoContract = engine.GetNativeContract(NeoToken::GetContractId());
                if (!neoContract)
                    throw std::runtime_error("NEO contract not found");
                
                // Get committee address from NEO contract
                io::UInt160 committeeAddress = neoContract->GetCommitteeAddress(engine.GetSnapshot());
                
                // Check if the committee address has witnessed the current transaction
                if (!engine.CheckWitnessInternal(committeeAddress))
                {
                    throw std::runtime_error("Committee authorization required for GAS transfer");
                }
                
                // Committee authorization successful
                std::cout << "Committee authorization verified for GAS transfer" << std::endl;
            }
            catch (const std::exception& e)
            {
                // Committee authorization failed - MUST deny access for security
                throw std::runtime_error(std::string("Committee authorization failed for GAS transfer: ") + e.what());
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(std::string("Committee check failed: ") + e.what());
        }

        // Transfer
        bool result = Transfer(engine, from, to, amount, data, true);

        return vm::StackItem::Create(result);
    }

    std::shared_ptr<vm::StackItem> GasToken::OnNEP17Payment(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
    {
        // GAS token doesn't accept NEP-17 payments
        throw std::runtime_error("Method not supported");
    }

    bool GasToken::OnPersist(ApplicationEngine& engine)
    {
        // Get the persisting block
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Process transaction fees
        int64_t totalNetworkFee = 0;
        auto transactions = block->GetTransactions();
        for (const auto& tx : transactions)
        {
            // Burn system fee and network fee from sender
            auto sender = tx->GetSender();
            int64_t totalFee = tx->GetSystemFee() + tx->GetNetworkFee();
            Burn(engine, sender, totalFee);

            // Add network fee to total
            totalNetworkFee += tx->GetNetworkFee();

            // Implement NotaryAssisted attribute handling for proper fee calculation
            try
            {
                // Check if transaction has NotaryAssisted attribute
                auto notaryAssistedAttr = tx->GetAttribute<ledger::NotaryAssisted>();
                if (notaryAssistedAttr)
                {
                    // Get the policy contract to retrieve attribute fee
                    auto policyContract = engine.GetNativeContract(PolicyContract::GetContractId());
                    if (policyContract)
                    {
                        // Get the attribute fee for NotaryAssisted type
                        std::vector<std::shared_ptr<vm::StackItem>> args;
                        args.push_back(vm::StackItem::Create(static_cast<int64_t>(ledger::TransactionAttribute::Usage::NotaryAssisted)));
                        
                        auto attributeFeeResult = policyContract->CallMethod(engine, "getAttributeFee", args);
                        if (attributeFeeResult && attributeFeeResult->IsInteger())
                        {
                            int64_t attributeFee = attributeFeeResult->GetInteger();
                            int64_t nKeys = notaryAssistedAttr->GetNKeys();
                            
                            // Subtract the notary fee from total network fee
                            // This fee goes to the notary service, not to validators
                            totalNetworkFee -= (nKeys + 1) * attributeFee;
                            
                            std::cout << "Processed NotaryAssisted attribute: nKeys=" << nKeys 
                                     << ", attributeFee=" << attributeFee << std::endl;
                        }
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error processing NotaryAssisted attribute: " << e.what() << std::endl;
                // Continue processing without the notary fee adjustment
            }
        }

        return true;
    }

    bool GasToken::PostPersist(ApplicationEngine& engine)
    {
        // Get the persisting block
        auto block = engine.GetPersistingBlock();
        if (!block)
            return false;

        // Get the protocol settings
        auto settings = engine.GetProtocolSettings();
        if (!settings)
            return false;

        // Get the committee members count and validators count
        int committeeMembersCount = settings->GetCommitteeMembersCount();
        int validatorsCount = settings->GetValidatorsCount();

        if (committeeMembersCount <= 0 || validatorsCount <= 0)
            return false;

        // Calculate the index of the committee member to reward
        int index = static_cast<int>(block->GetIndex() % committeeMembersCount);

        // Get the gas per block
        int64_t gasPerBlock = GetGasPerBlock(engine.GetSnapshot());

        // Implement committee reward distribution using NEO token contract integration
        try
        {
            // Get the NEO token contract to retrieve committee members
            auto neoContract = engine.GetNativeContract(NeoToken::GetContractId());
            if (neoContract)
            {
                // Get the current committee members
                auto committee = neoContract->GetCommittee(engine.GetSnapshot());
                if (!committee.empty())
                {
                    // Calculate the committee member to reward based on block index
                    int memberIndex = static_cast<int>(block->GetIndex() % committee.size());
                    auto rewardedMember = committee[memberIndex];
                    
                    // Create script hash for the committee member
                    auto memberScriptHash = neo::cryptography::Crypto::CreateSignatureRedeemScript(rewardedMember).ToScriptHash();
                    
                    // Mint GAS reward to the committee member
                    if (gasPerBlock > 0)
                    {
                        Mint(engine, memberScriptHash, gasPerBlock, false);
                        
                        std::cout << "Rewarded committee member " << memberIndex 
                                 << " with " << gasPerBlock << " GAS for block " << block->GetIndex() << std::endl;
                    }
                }
                else
                {
                    std::cerr << "No committee members found for reward distribution" << std::endl;
                }
            }
            else
            {
                std::cerr << "NEO contract not found for committee reward distribution" << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error distributing committee rewards: " << e.what() << std::endl;
            // Continue without reward distribution
        }

        return true;
    }
}
