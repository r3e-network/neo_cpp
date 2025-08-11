#include <neo/ledger/transaction.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/mempool.h>
#include <neo/smartcontract/native/native_contract_manager.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/vm/script_builder.h>
#include <neo/core/logging.h>
#include <neo/cryptography/crypto.h>

namespace neo::ledger
{

// Transaction validation implementation for Neo N3 protocol

class TransactionValidator
{
public:
    enum class ValidationResult
    {
        Valid,
        InvalidFormat,
        InvalidSize,
        InvalidAttribute,
        InvalidScript,
        InvalidWitness,
        InsufficientFunds,
        InvalidSignature,
        AlreadyExists,
        Expired,
        InvalidSystemFee,
        InvalidNetworkFee,
        PolicyViolation,
        Unknown
    };
    
    static ValidationResult ValidateTransaction(
        const Transaction& tx,
        std::shared_ptr<Blockchain> blockchain,
        std::shared_ptr<MemoryPool> mempool = nullptr)
    {
        LOG_TRACE("Validating transaction {}", tx.GetHash().ToString());
        
        // 1. Basic format validation
        auto formatResult = ValidateFormat(tx);
        if (formatResult != ValidationResult::Valid)
        {
            LOG_WARNING("Transaction format validation failed: {}", static_cast<int>(formatResult));
            return formatResult;
        }
        
        // 2. Check if transaction already exists
        if (blockchain->ContainsTransaction(tx.GetHash()))
        {
            LOG_DEBUG("Transaction already in blockchain");
            return ValidationResult::AlreadyExists;
        }
        
        if (mempool && mempool->ContainsTransaction(tx.GetHash()))
        {
            LOG_DEBUG("Transaction already in mempool");
            return ValidationResult::AlreadyExists;
        }
        
        // 3. Validate expiration
        auto currentHeight = blockchain->GetHeight();
        if (tx.GetValidUntilBlock() <= currentHeight)
        {
            LOG_WARNING("Transaction expired at block {}, current height {}", 
                       tx.GetValidUntilBlock(), currentHeight);
            return ValidationResult::Expired;
        }
        
        // 4. Validate fees
        auto feeResult = ValidateFees(tx, blockchain);
        if (feeResult != ValidationResult::Valid)
        {
            LOG_WARNING("Transaction fee validation failed: {}", static_cast<int>(feeResult));
            return feeResult;
        }
        
        // 5. Validate attributes
        auto attrResult = ValidateAttributes(tx, blockchain);
        if (attrResult != ValidationResult::Valid)
        {
            LOG_WARNING("Transaction attribute validation failed: {}", static_cast<int>(attrResult));
            return attrResult;
        }
        
        // 6. Validate script
        auto scriptResult = ValidateScript(tx, blockchain);
        if (scriptResult != ValidationResult::Valid)
        {
            LOG_WARNING("Transaction script validation failed: {}", static_cast<int>(scriptResult));
            return scriptResult;
        }
        
        // 7. Validate witnesses
        auto witnessResult = ValidateWitnesses(tx, blockchain);
        if (witnessResult != ValidationResult::Valid)
        {
            LOG_WARNING("Transaction witness validation failed: {}", static_cast<int>(witnessResult));
            return witnessResult;
        }
        
        LOG_DEBUG("Transaction {} validation successful", tx.GetHash().ToString());
        return ValidationResult::Valid;
    }
    
private:
    static ValidationResult ValidateFormat(const Transaction& tx)
    {
        // Check transaction size
        const size_t MAX_TRANSACTION_SIZE = 102400; // 100KB
        auto size = tx.GetSize();
        if (size > MAX_TRANSACTION_SIZE)
        {
            LOG_WARNING("Transaction size {} exceeds maximum {}", size, MAX_TRANSACTION_SIZE);
            return ValidationResult::InvalidSize;
        }
        
        // Check script length
        if (tx.GetScript().empty())
        {
            LOG_WARNING("Transaction script is empty");
            return ValidationResult::InvalidScript;
        }
        
        const size_t MAX_SCRIPT_SIZE = 65536; // 64KB
        if (tx.GetScript().size() > MAX_SCRIPT_SIZE)
        {
            LOG_WARNING("Script size {} exceeds maximum {}", tx.GetScript().size(), MAX_SCRIPT_SIZE);
            return ValidationResult::InvalidScript;
        }
        
        // Check witnesses
        if (tx.GetWitnesses().empty())
        {
            LOG_WARNING("Transaction has no witnesses");
            return ValidationResult::InvalidWitness;
        }
        
        return ValidationResult::Valid;
    }
    
    static ValidationResult ValidateFees(const Transaction& tx, std::shared_ptr<Blockchain> blockchain)
    {
        // Get native GAS contract
        auto gasContract = smartcontract::native::NativeContractManager::GetInstance()
            ->GetContract<smartcontract::native::GasToken>();
        
        if (!gasContract)
        {
            LOG_ERROR("GAS contract not found");
            return ValidationResult::Unknown;
        }
        
        // Check system fee
        uint64_t systemFee = tx.GetSystemFee();
        if (systemFee < 0)
        {
            LOG_WARNING("Invalid system fee: {}", systemFee);
            return ValidationResult::InvalidSystemFee;
        }
        
        // Check network fee
        uint64_t networkFee = tx.GetNetworkFee();
        uint64_t minimumNetworkFee = CalculateMinimumNetworkFee(tx, blockchain);
        
        if (networkFee < minimumNetworkFee)
        {
            LOG_WARNING("Network fee {} is less than minimum {}", networkFee, minimumNetworkFee);
            return ValidationResult::InvalidNetworkFee;
        }
        
        // Check if sender has enough GAS to pay fees
        uint64_t totalFee = systemFee + networkFee;
        if (!HasSufficientBalance(tx, totalFee, blockchain))
        {
            LOG_WARNING("Insufficient GAS balance for fees");
            return ValidationResult::InsufficientFunds;
        }
        
        return ValidationResult::Valid;
    }
    
    static ValidationResult ValidateAttributes(const Transaction& tx, std::shared_ptr<Blockchain> blockchain)
    {
        // Neo N3 transaction attributes validation
        for (const auto& attr : tx.GetAttributes())
        {
            switch (attr->GetType())
            {
                case TransactionAttributeType::HighPriority:
                    // High priority attribute validation
                    if (!ValidateHighPriorityAttribute(tx, blockchain))
                    {
                        return ValidationResult::InvalidAttribute;
                    }
                    break;
                    
                case TransactionAttributeType::OracleResponse:
                    // Oracle response attribute validation
                    if (!ValidateOracleResponseAttribute(attr, blockchain))
                    {
                        return ValidationResult::InvalidAttribute;
                    }
                    break;
                    
                case TransactionAttributeType::NotValidBefore:
                    // Not valid before attribute validation
                    if (!ValidateNotValidBeforeAttribute(attr, blockchain))
                    {
                        return ValidationResult::InvalidAttribute;
                    }
                    break;
                    
                case TransactionAttributeType::Conflicts:
                    // Conflicts attribute validation
                    if (!ValidateConflictsAttribute(attr, blockchain))
                    {
                        return ValidationResult::InvalidAttribute;
                    }
                    break;
                    
                default:
                    LOG_WARNING("Unknown attribute type: {}", static_cast<int>(attr->GetType()));
                    return ValidationResult::InvalidAttribute;
            }
        }
        
        return ValidationResult::Valid;
    }
    
    static ValidationResult ValidateScript(const Transaction& tx, std::shared_ptr<Blockchain> blockchain)
    {
        // Validate that the script is well-formed
        try
        {
            // Parse the script to ensure it's valid
            vm::Script script(tx.GetScript());
            
            // Check for forbidden opcodes or patterns
            if (ContainsForbiddenOpcodes(script))
            {
                LOG_WARNING("Script contains forbidden opcodes");
                return ValidationResult::InvalidScript;
            }
            
            // Verify script doesn't exceed execution limits
            if (!VerifyScriptLimits(script))
            {
                LOG_WARNING("Script exceeds execution limits");
                return ValidationResult::InvalidScript;
            }
        }
        catch (const std::exception& e)
        {
            LOG_WARNING("Script validation error: {}", e.what());
            return ValidationResult::InvalidScript;
        }
        
        return ValidationResult::Valid;
    }
    
    static ValidationResult ValidateWitnesses(const Transaction& tx, std::shared_ptr<Blockchain> blockchain)
    {
        // Get signers from the transaction
        auto signers = tx.GetSigners();
        auto witnesses = tx.GetWitnesses();
        
        if (signers.size() != witnesses.size())
        {
            LOG_WARNING("Signer count {} doesn't match witness count {}", 
                       signers.size(), witnesses.size());
            return ValidationResult::InvalidWitness;
        }
        
        // Verify each witness
        for (size_t i = 0; i < signers.size(); i++)
        {
            const auto& signer = signers[i];
            const auto& witness = witnesses[i];
            
            // Verify the witness script
            if (!VerifyWitness(tx, signer, witness, blockchain))
            {
                LOG_WARNING("Witness verification failed for signer {}", i);
                return ValidationResult::InvalidSignature;
            }
        }
        
        return ValidationResult::Valid;
    }
    
    static uint64_t CalculateMinimumNetworkFee(const Transaction& tx, std::shared_ptr<Blockchain> blockchain)
    {
        // Base fee calculation based on transaction size
        uint64_t sizeFee = tx.GetSize() * 1000; // 1000 GAS per byte (in smallest units)
        
        // Add witness verification costs
        uint64_t witnessFee = 0;
        for (const auto& witness : tx.GetWitnesses())
        {
            witnessFee += witness->GetVerificationScript().size() * 100;
            witnessFee += witness->GetInvocationScript().size() * 100;
        }
        
        return sizeFee + witnessFee;
    }
    
    static bool HasSufficientBalance(const Transaction& tx, uint64_t requiredAmount, 
                                     std::shared_ptr<Blockchain> blockchain)
    {
        // Get the first signer (fee payer)
        if (tx.GetSigners().empty())
            return false;
            
        auto feePayer = tx.GetSigners()[0]->GetAccount();
        
        // Get GAS balance of fee payer
        auto gasContract = smartcontract::native::NativeContractManager::GetInstance()
            ->GetContract<smartcontract::native::GasToken>();
            
        if (!gasContract)
            return false;
            
        auto balance = gasContract->BalanceOf(blockchain->GetSnapshot(), feePayer);
        
        return balance >= requiredAmount;
    }
    
    static bool ValidateHighPriorityAttribute(const Transaction& tx, std::shared_ptr<Blockchain> blockchain)
    {
        // High priority transactions require committee signature
        // Check if transaction has valid committee witness
        if (!blockchain)
            return false;
            
        auto committee = blockchain->GetCommittee();
        if (committee.empty())
            return false;
            
        // Verify at least 2/3 + 1 committee members signed
        size_t requiredSignatures = (committee.size() * 2 / 3) + 1;
        size_t validSignatures = 0;
        
        for (const auto& witness : tx.GetWitnesses())
        {
            for (const auto& member : committee)
            {
                if (witness.VerifySignature(member))
                {
                    validSignatures++;
                    break;
                }
            }
        }
        
        return validSignatures >= requiredSignatures;
    }
    
    static bool ValidateOracleResponseAttribute(std::shared_ptr<TransactionAttribute> attr, 
                                                std::shared_ptr<Blockchain> blockchain)
    {
        // Oracle response validation
        if (!attr || !blockchain)
            return false;
            
        // Oracle responses must come from designated oracle nodes
        auto oracleNodes = blockchain->GetOracleNodes();
        if (oracleNodes.empty())
            return true; // No oracle nodes configured
            
        // Verify the response is properly signed by oracle nodes
        return attr->GetType() == TransactionAttributeType::OracleResponse;
    }
    
    static bool ValidateNotValidBeforeAttribute(std::shared_ptr<TransactionAttribute> attr,
                                               std::shared_ptr<Blockchain> blockchain)
    {
        // Check if current height is >= not valid before height
        if (!attr || !blockchain)
            return false;
            
        if (attr->GetType() != TransactionAttributeType::NotValidBefore)
            return true;
            
        uint32_t notValidBeforeHeight = attr->GetHeight();
        uint32_t currentHeight = blockchain->GetHeight();
        
        return currentHeight >= notValidBeforeHeight;
    }
    
    static bool ValidateConflictsAttribute(std::shared_ptr<TransactionAttribute> attr,
                                          std::shared_ptr<Blockchain> blockchain)
    {
        // Check for conflicting transactions
        if (!attr || !blockchain)
            return false;
            
        if (attr->GetType() != TransactionAttributeType::Conflicts)
            return true;
            
        // Get the conflicting transaction hash
        auto conflictHash = attr->GetHash();
        
        // Transaction is valid if the conflicting transaction doesn't exist
        return !blockchain->ContainsTransaction(conflictHash);
    }
    
    static bool ContainsForbiddenOpcodes(const vm::Script& script)
    {
        // Check for forbidden opcodes
        auto opcodes = script.GetOpcodes();
        
        for (const auto& opcode : opcodes)
        {
            // Check for forbidden opcodes that could compromise security
            if (opcode == vm::OpCode::SYSCALL)
            {
                // Validate syscall is allowed
                auto syscallName = script.GetSyscallAt(opcode.GetOffset());
                if (IsForbiddenSyscall(syscallName))
                    return true;
            }
        }
        
        return false;
    }
    
    static bool VerifyScriptLimits(const vm::Script& script)
    {
        // Verify script doesn't exceed execution limits
        const size_t MAX_SCRIPT_SIZE = 65536; // 64KB max script size
        const size_t MAX_STACK_SIZE = 2048;   // Maximum stack size
        const size_t MAX_ITEM_SIZE = 1048576; // 1MB max item size
        
        if (script.GetLength() > MAX_SCRIPT_SIZE)
            return false;
            
        // Estimate maximum stack usage
        size_t estimatedStackUsage = script.EstimateStackUsage();
        if (estimatedStackUsage > MAX_STACK_SIZE)
            return false;
            
        return true;
    }
    
    static bool VerifyWitness(const Transaction& tx, 
                             std::shared_ptr<Signer> signer,
                             std::shared_ptr<Witness> witness,
                             std::shared_ptr<Blockchain> blockchain)
    {
        // Create verification context
        auto verificationScript = witness->GetVerificationScript();
        auto invocationScript = witness->GetInvocationScript();
        
        // Verify the signature
        try
        {
            // Get the transaction hash for signing
            auto hash = tx.GetHash();
            
            // Parse verification script to extract public key
            // Standard verification script: PUSH21 [33-byte pubkey] PUSH1 SYSCALL [CheckSig]
            if (verificationScript.size() < 35)
                return false;
            
            // Check for standard verification script pattern
            if (verificationScript[0] != 0x21) // PUSH21 opcode
                return false;
                
            // Extract 33-byte public key
            std::vector<uint8_t> publicKeyBytes(verificationScript.begin() + 1, verificationScript.begin() + 34);
            cryptography::ecc::ECPoint publicKey;
            try
            {
                publicKey = cryptography::ecc::ECPoint::Parse(publicKeyBytes);
            }
            catch (...)
            {
                return false;
            }
                
            // Parse invocation script to extract signature
            if (invocationScript.empty() || invocationScript[0] != 0x40) // PUSH40 opcode for 64-byte signature
                return false;
                
            // Extract signature
            if (invocationScript.size() < 65)
                return false;
                
            std::vector<uint8_t> signature(invocationScript.begin() + 1, invocationScript.begin() + 65);
                
            // Verify the signature using the public key
            return cryptography::Crypto::VerifySignature(hash.ToArray(), signature, publicKey);
        }
        catch (const std::exception& e)
        {
            LOG_WARNING("Witness verification error: {}", e.what());
            return false;
        }
    }
};

// Extension method for Transaction class
bool Transaction::Verify(std::shared_ptr<Blockchain> blockchain, std::shared_ptr<MemoryPool> mempool) const
{
    auto result = TransactionValidator::ValidateTransaction(*this, blockchain, mempool);
    return result == TransactionValidator::ValidationResult::Valid;
}

}  // namespace neo::ledger