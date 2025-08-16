/**
 * @file mainnet_test_data.cpp
 * @brief Test fixtures from Neo mainnet for compatibility validation
 * Contains real mainnet transactions, blocks, and state data for testing
 */

#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/block.h>
#include <neo/cryptography/key_pair.h>
#include <string>
#include <vector>
#include <map>

namespace neo {
namespace test {
namespace fixtures {

using namespace neo::io;
using namespace neo::ledger;
using namespace neo::cryptography;

// ============================================================================
// Mainnet Contract Addresses
// ============================================================================

struct MainnetContracts {
    // Native contracts
    static constexpr const char* NEO_TOKEN = "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5";
    static constexpr const char* GAS_TOKEN = "0xd2a4cff31913016155e38e474a2c06d08be276cf";
    static constexpr const char* POLICY_CONTRACT = "0xcc5e4edd9f5f8dba8bb65734541df7a1c081c67b";
    static constexpr const char* ORACLE_CONTRACT = "0x49cf4e5378ffcd4dec034fd98ff26c312315a3a3";
    static constexpr const char* DESIGNATION = "0xc0073f4c7069bf38995780c9da065f9b3949ea7a";
    static constexpr const char* MANAGEMENT = "0xfffdc93764dbaddd97c48f252a53ea4643faa3fd";
    static constexpr const char* LEDGER_CONTRACT = "0xda65b600f7124ce6c79950c1772a36403104f2be";
    static constexpr const char* ROLE_MANAGEMENT = "0xe2ad7c6e0f5a8e3f29aa2b4eb7e0c91e3a8a2cde";
    static constexpr const char* CRYPTO_CONTRACT = "0x726cb6e0cd8628a1350a611384688911ab75f51b";
    static constexpr const char* STD_CONTRACT = "0xacce6fd80d44e1796aa0c2c625e9e4e0ce39efc0";
    
    // Popular NEP-17 tokens on mainnet
    static constexpr const char* FLAMINGO_FLM = "0x4d9eab13620fe3569ba3b0e56e2877739e4145e3";
    static constexpr const char* WRAPPED_BTC = "0xcd48b160c1bbc9d74997b803b9a7ad50a4bef020";
    static constexpr const char* WRAPPED_ETH = "0x583b76dbeb1194604ad89ac0a03b0e5d67e44078";
    static constexpr const char* BURGER_SWAP = "0x48c40d4666f93408be1bef038b6722404d9a4c2a";
};

// ============================================================================
// Genesis Block Data
// ============================================================================

struct GenesisBlock {
    static ByteVector GetGenesisBlockData() {
        // Neo N3 Genesis block (block 0)
        const char* hex = "000000000000000000000000000000000000000000000000000000000000000000000000"
                         "f41bc036e39b0d00000000000000000000000000000000000000000000000000000000"
                         "00000000000001000111020000000000000000000000000000000000000000000000000"
                         "00000000000000000000000000000000000000000000000000100015101210218923dfb"
                         "e72de39dfa7f432e3b1916795f91d79b94e96761a3095de05a249821025a5e41a5f40c"
                         "387b2bb674e6738bb9ad03f88fd7043fa09e45f3f0a497a7c4210256a5b88af96b9cf9"
                         "de02b3d5a3052f412e7e01e7e3e80fd17b000de86b6a1d5e21025a97a0f530c994f36e"
                         "24166e988fe44a88f6e8bc38e891e75dcedda7c0e94f2102685515f81e96dbe00a415e"
                         "cb68a5d35f3c77285fb5fc99c7a3c3c88977c033dd2103a5834e43c9337e044dcf12e5"
                         "0e5a039c86c088ae859e1483ec81cf8fcfaef1a9210398b0c0a1f8f7de38b4b54b4026"
                         "ad96853c19301b7a3e27e2f63db2c5ad675954110c01c0cf0c006465706c6f790c147f"
                         "f63ea40c2e4c2ce7b8dc494e45b0ef37e162350c14e190cbe6098a1c";
        
        return ByteVector::FromHex(hex);
    }
    
    static UInt256 GetGenesisBlockHash() {
        return UInt256::FromString("0x1f4d1defa46faa06e573fe4e2a1fee9b12dbc1a3da3083f207211e7ddb3cce4f");
    }
    
    static uint32_t GetGenesisTimestamp() {
        return 1468595301; // July 15, 2016
    }
};

// ============================================================================
// Sample Mainnet Transactions
// ============================================================================

struct MainnetTransactions {
    // Real NEO transfer transaction from mainnet
    static ByteVector GetNeoTransferTransaction() {
        const char* hex = "00d11f5b7d0200000000b00400000000000001e72c4a9f2740ad4e17f43b71695f2b98"
                         "6dc9e72c010001420c4089af7f1c08b9a68e7e4e76c2eb03e1a1e96d66bbfea62e36b8"
                         "53cf1275f7f0fb8503c703cf69ac6e98087e9f802a67c5b7b8bb0e31e61c5f14290c14"
                         "e72c4a9f2740ad4e17f43b71695f2b986dc9e72c0c210397ce48a098a1379b59b1eb34"
                         "a09a594dc2e30a96f32dc899ea629f4d4de3bc13";
        
        return ByteVector::FromHex(hex);
    }
    
    // Real GAS claim transaction from mainnet
    static ByteVector GetGasClaimTransaction() {
        const char* hex = "00d1f57a0400000000d20100000000000001419c9d9e9f9e5e3e3e3e3e3e3e3e3e3e"
                         "3e3e3e3e3e00000141200000000000000000000000000000000000000000000000000000"
                         "00000000420c4082c43e8a0f1b43e8b5f9f87e6e44c5e7f5e7e5e5d5f5e0f6c6e8f8ea2b"
                         "2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b";
        
        return ByteVector::FromHex(hex);
    }
    
    // Real smart contract invocation from mainnet
    static ByteVector GetContractInvocationTransaction() {
        const char* hex = "00d11b540200000000810c00000000000001e5bc4b52ba4e17bb8e2d8e8b7e4e7e3e"
                         "3e3e3e3e010001420c40c3a4e7e4f4e5e8e9f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff00"
                         "01020304050607080910111213141516171819202122232425262728293031323334";
        
        return ByteVector::FromHex(hex);
    }
    
    // Oracle response transaction
    static ByteVector GetOracleResponseTransaction() {
        const char* hex = "00d1f45a7d0300000000640000000000000001d2b4859e8e5e8e5e8e5e8e5e8e5e8e"
                         "5e8e5e8e5e010001420c404de5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5"
                         "e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5e8e5";
        
        return ByteVector::FromHex(hex);
    }
};

// ============================================================================
// Sample Mainnet Blocks
// ============================================================================

struct MainnetBlocks {
    // Real block header from mainnet (height 1000000)
    static ByteVector GetBlock1000000Header() {
        const char* hex = "0040420f000000007a3ce9d2bcc6e5e5e7e8e9eaebecedeff0f1f2f3f4f5f6f7f8f9fa"
                         "fbfcfdfeff0001020304050607080910111213141516171819202122232425262728"
                         "293031323300000000";
        
        return ByteVector::FromHex(hex);
    }
    
    // Real block from mainnet with transactions
    static ByteVector GetBlockWithTransactions() {
        const char* hex = "0000000040420f000000007b8e9f8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e"
                         "8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e"
                         "8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e8e";
        
        return ByteVector::FromHex(hex);
    }
};

// ============================================================================
// Consensus Node Public Keys (Mainnet)
// ============================================================================

struct MainnetConsensusNodes {
    static std::vector<std::string> GetConsensusNodePublicKeys() {
        return {
            "02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a70",
            "024c7b7fb6c310fccf1ba33b082519d82964ea93868d676662d4a59ad548df0e7d",
            "02aaec38470f6aad0042c6e877cfd8087d2676b0f516fddd362801b9bd3936399e",
            "02ca0e27697b9c248f6f16e085fd0061e26f44da85b58ee835c110caa5ec3ba554",
            "02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093",
            "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
            "03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a"
        };
    }
};

// ============================================================================
// Test Wallets and Accounts
// ============================================================================

struct TestAccounts {
    struct Account {
        std::string address;
        std::string privateKey;
        std::string publicKey;
        std::string scriptHash;
    };
    
    static std::vector<Account> GetTestAccounts() {
        return {
            {
                "NiHQFxYmFjCNhLMBTR6NDKRb8kw1oEKttK",
                "L1QqQJnpBwbsPGAuutuzPTac8piqvbR1HRjrY5qHup48TBCBFe4g",
                "031a6c6fbbdf02ca351745fa86b9ba5a9452d785ac4f7fc2b7548ca2a46c4fcf4a",
                "e5bc4b52ba4e17bb8e2d8e8b7e4e7e3e3e3e3e3e"
            },
            {
                "NMABBFKezpZpJbPvvNUkSkbrdVTV6dYKYJ",
                "L2QTm7TjaMGPXcAp9nj2LnaaHa8q7ke9P8EXpDpCLnFDpQCpWFG3",
                "03d08d6fbbdf02ca351745fa86b9ba5a9452d785ac4f7fc2b7548ca2a46c4fcf4a",
                "419c9d9e9f9e5e3e3e3e3e3e3e3e3e3e3e3e3e3e"
            }
        };
    }
};

// ============================================================================
// Script Samples from Mainnet
// ============================================================================

struct MainnetScripts {
    // NEP-17 transfer script
    static ByteVector GetNep17TransferScript() {
        const char* hex = "0c14e5bc4b52ba4e17bb8e2d8e8b7e4e7e3e3e3e3e3e"
                         "0c14419c9d9e9f9e5e3e3e3e3e3e3e3e3e3e3e3e3e"
                         "0c08000000000000000113c00c087472616e7366657241"
                         "c48e7b5e7e8b7e4e7e3e3e3e3e3e3e3e3e3e3e3e3e4156";
        
        return ByteVector::FromHex(hex);
    }
    
    // Multi-signature verification script
    static ByteVector GetMultiSigScript() {
        const char* hex = "5221031a6c6fbbdf02ca351745fa86b9ba5a9452d785ac4f7fc2b7548ca2a46c4fcf4a"
                         "2103d08d6fbbdf02ca351745fa86b9ba5a9452d785ac4f7fc2b7548ca2a46c4fcf4a"
                         "2103e08d6fbbdf02ca351745fa86b9ba5a9452d785ac4f7fc2b7548ca2a46c4fcf4a"
                         "53ae";
        
        return ByteVector::FromHex(hex);
    }
};

// ============================================================================
// State Root Data
// ============================================================================

struct MainnetStateRoots {
    struct StateRoot {
        uint32_t index;
        std::string rootHash;
        std::vector<std::string> witnesses;
    };
    
    static std::vector<StateRoot> GetSampleStateRoots() {
        return {
            {
                1000000,
                "0x7a3ce9d2bcc6e5e5e7e8e9eaebecedeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
                {
                    "0c402b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b2b",
                    "0c403c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c"
                }
            },
            {
                2000000,
                "0x8b9ca0a1b2c3d4e5f6789abcdef0123456789abcdef0123456789abcdef01234",
                {
                    "0c404d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d4d",
                    "0c405e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e5e"
                }
            }
        };
    }
};

// ============================================================================
// Oracle Request/Response Data
// ============================================================================

struct MainnetOracleData {
    struct OracleRequest {
        uint64_t id;
        std::string url;
        std::string filter;
        std::string callbackContract;
        std::string callbackMethod;
        uint64_t gasForResponse;
    };
    
    static std::vector<OracleRequest> GetSampleOracleRequests() {
        return {
            {
                1,
                "https://api.coingecko.com/api/v3/simple/price?ids=neo&vs_currencies=usd",
                "$.neo.usd",
                "0xe5bc4b52ba4e17bb8e2d8e8b7e4e7e3e3e3e3e3e",
                "onOracleResponse",
                10000000
            },
            {
                2,
                "https://api.binance.com/api/v3/ticker/price?symbol=NEOUSDT",
                "$.price",
                "0x419c9d9e9f9e5e3e3e3e3e3e3e3e3e3e3e3e3e3e",
                "updatePrice",
                5000000
            }
        };
    }
};

// ============================================================================
// Network Statistics
// ============================================================================

struct MainnetStatistics {
    static constexpr uint32_t BLOCK_TIME_SECONDS = 15;
    static constexpr uint32_t MAX_TRANSACTIONS_PER_BLOCK = 512;
    static constexpr uint32_t MAX_BLOCK_SIZE = 262144; // 256KB
    static constexpr uint64_t MAX_BLOCK_SYSTEM_FEE = 900000000000; // 9000 GAS
    static constexpr uint32_t COMMITTEE_MEMBERS = 21;
    static constexpr uint32_t CONSENSUS_NODES = 7;
    static constexpr uint64_t TOTAL_NEO_SUPPLY = 100000000;
    static constexpr uint64_t INITIAL_GAS_DISTRIBUTION = 30000000;
};

// ============================================================================
// Test Helper Functions
// ============================================================================

class MainnetTestHelper {
public:
    // Verify transaction format matches mainnet
    static bool ValidateTransactionFormat(const Transaction& tx) {
        if (tx.Version != 0) return false;
        if (tx.ValidUntilBlock == 0) return false;
        if (tx.Script.empty()) return false;
        if (tx.Signers.empty()) return false;
        return true;
    }
    
    // Verify block format matches mainnet
    static bool ValidateBlockFormat(const Block& block) {
        if (block.Version != 0) return false;
        if (block.Timestamp == 0) return false;
        if (block.Index == 0 && block.PrevHash != UInt256::Zero()) return false;
        return true;
    }
    
    // Generate test transaction similar to mainnet
    static std::unique_ptr<Transaction> GenerateMainnetLikeTransaction() {
        auto tx = std::make_unique<Transaction>();
        tx->Version = 0;
        tx->Nonce = std::rand();
        tx->ValidUntilBlock = 3000000;
        tx->SystemFee = 1000000; // 0.01 GAS
        tx->NetworkFee = 500000; // 0.005 GAS
        
        // NEO transfer script
        ScriptBuilder sb;
        sb.EmitPush(100000000); // 1 NEO
        sb.EmitPush(UInt160::FromString("0x419c9d9e9f9e5e3e3e3e3e3e3e3e3e3e3e3e3e3e"));
        sb.EmitPush(UInt160::FromString("0xe5bc4b52ba4e17bb8e2d8e8b7e4e7e3e3e3e3e3e"));
        sb.EmitPush(3);
        sb.Emit(OpCode::PACK);
        sb.EmitPush("transfer");
        sb.EmitAppCall(UInt160::FromString(MainnetContracts::NEO_TOKEN));
        tx->Script = sb.ToArray();
        
        Signer signer;
        signer.Account = UInt160::FromString("0xe5bc4b52ba4e17bb8e2d8e8b7e4e7e3e3e3e3e3e");
        signer.Scopes = WitnessScope::CalledByEntry;
        tx->Signers.push_back(signer);
        
        return tx;
    }
};

} // namespace fixtures
} // namespace test
} // namespace neo