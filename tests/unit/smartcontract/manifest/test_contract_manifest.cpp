#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include "neo/smartcontract/manifest/contract_manifest.h"
#include "neo/smartcontract/manifest/contract_group.h"
#include "neo/smartcontract/manifest/contract_permission.h"
#include "neo/smartcontract/manifest/contract_method_descriptor.h"
#include "neo/smartcontract/manifest/contract_event_descriptor.h"
#include "neo/smartcontract/manifest/wildcard_container.h"
#include "neo/io/uint160.h"
#include "neo/io/binary_writer.h"
#include "neo/io/binary_reader.h"
#include "neo/cryptography/ecc/ecpoint.h"
#include "neo/vm/stack_item.h"

using namespace neo::smartcontract::manifest;
using namespace neo::io;
using namespace neo::cryptography::ecc;
using namespace neo::vm;
using json = nlohmann::json;

class ContractManifestTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default manifest JSON for testing
        default_manifest_json_ = R"({
            "name": "TestContract",
            "group": [],
            "supportedstandards": [],
            "abi": {
                "methods": [
                    {
                        "name": "testMethod",
                        "parameters": [],
                        "returntype": "Any",
                        "offset": 0,
                        "safe": false
                    }
                ],
                "events": []
            },
            "permissions": [
                {
                    "contract": "*",
                    "methods": "*"
                }
            ],
            "trusts": [],
            "extra": null
        })";

        // NEP-17 Token manifest example
        nep17_manifest_json_ = R"({
            "name": "SampleNep17Token",
            "group": [],
            "supportedstandards": ["NEP-17"],
            "abi": {
                "methods": [
                    {
                        "name": "symbol",
                        "parameters": [],
                        "returntype": "String",
                        "offset": 0,
                        "safe": true
                    },
                    {
                        "name": "decimals",
                        "parameters": [],
                        "returntype": "Integer",
                        "offset": 10,
                        "safe": true
                    },
                    {
                        "name": "totalSupply",
                        "parameters": [],
                        "returntype": "Integer",
                        "offset": 20,
                        "safe": true
                    },
                    {
                        "name": "balanceOf",
                        "parameters": [
                            {
                                "name": "account",
                                "type": "Hash160"
                            }
                        ],
                        "returntype": "Integer",
                        "offset": 30,
                        "safe": true
                    },
                    {
                        "name": "transfer",
                        "parameters": [
                            {
                                "name": "from",
                                "type": "Hash160"
                            },
                            {
                                "name": "to",
                                "type": "Hash160"
                            },
                            {
                                "name": "amount",
                                "type": "Integer"
                            },
                            {
                                "name": "data",
                                "type": "Any"
                            }
                        ],
                        "returntype": "Boolean",
                        "offset": 40,
                        "safe": false
                    }
                ],
                "events": [
                    {
                        "name": "Transfer",
                        "parameters": [
                            {
                                "name": "from",
                                "type": "Hash160"
                            },
                            {
                                "name": "to",
                                "type": "Hash160"
                            },
                            {
                                "name": "amount",
                                "type": "Integer"
                            }
                        ]
                    }
                ]
            },
            "permissions": [
                {
                    "contract": "*",
                    "methods": "*"
                }
            ],
            "trusts": [],
            "extra": null
        })";

        // Test ECPoint (from C# tests)
        std::vector<uint8_t> pubkey_bytes = {
            0x02, 0x18, 0x21, 0x80, 0x7f, 0x92, 0x3a, 0x3d, 
            0xa0, 0x04, 0xfb, 0x73, 0x87, 0x15, 0x09, 0xd7, 
            0x63, 0x5b, 0xcc, 0x05, 0xf4, 0x1e, 0xde, 0xf2, 
            0xa3, 0xca, 0x5c, 0x94, 0x1d, 0x8b, 0xbc, 0x12, 0x31
        };
        test_public_key_ = ECPoint::Parse(pubkey_bytes);
    }

    std::string default_manifest_json_;
    std::string nep17_manifest_json_;
    ECPoint test_public_key_;
};

TEST_F(ContractManifestTest, ParseFromJson_Default) {
    auto manifest = ContractManifest::FromJson(default_manifest_json_);
    
    ASSERT_NE(manifest, nullptr);
    EXPECT_EQ(manifest->GetName(), "TestContract");
    EXPECT_TRUE(manifest->GetGroups().empty());
    EXPECT_TRUE(manifest->GetSupportedStandards().empty());
    EXPECT_EQ(manifest->GetAbi().GetMethods().size(), 1);
    EXPECT_EQ(manifest->GetAbi().GetEvents().size(), 0);
    EXPECT_EQ(manifest->GetPermissions().size(), 1);
    EXPECT_TRUE(manifest->GetTrusts().IsWildcard());
    EXPECT_TRUE(manifest->GetExtra().empty());
}

TEST_F(ContractManifestTest, ParseFromJson_NEP17Token) {
    auto manifest = ContractManifest::FromJson(nep17_manifest_json_);
    
    ASSERT_NE(manifest, nullptr);
    EXPECT_EQ(manifest->GetName(), "SampleNep17Token");
    EXPECT_TRUE(manifest->GetGroups().empty());
    EXPECT_EQ(manifest->GetSupportedStandards().size(), 1);
    EXPECT_EQ(manifest->GetSupportedStandards()[0], "NEP-17");
    
    // Test ABI methods
    const auto& methods = manifest->GetAbi().GetMethods();
    EXPECT_EQ(methods.size(), 5);
    
    // Test symbol method
    auto symbol_method = std::find_if(methods.begin(), methods.end(),
        [](const auto& m) { return m.GetName() == "symbol"; });
    ASSERT_NE(symbol_method, methods.end());
    EXPECT_TRUE(symbol_method->IsSafe());
    EXPECT_EQ(symbol_method->GetReturnType(), ContractParameterType::String);
    EXPECT_TRUE(symbol_method->GetParameters().empty());
    
    // Test transfer method
    auto transfer_method = std::find_if(methods.begin(), methods.end(),
        [](const auto& m) { return m.GetName() == "transfer"; });
    ASSERT_NE(transfer_method, methods.end());
    EXPECT_FALSE(transfer_method->IsSafe());
    EXPECT_EQ(transfer_method->GetReturnType(), ContractParameterType::Boolean);
    EXPECT_EQ(transfer_method->GetParameters().size(), 4);
    
    // Test Transfer event
    const auto& events = manifest->GetAbi().GetEvents();
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].GetName(), "Transfer");
    EXPECT_EQ(events[0].GetParameters().size(), 3);
}

TEST_F(ContractManifestTest, ParseFromJson_Permissions) {
    std::string permission_manifest = R"({
        "name": "PermissionTest",
        "group": [],
        "supportedstandards": [],
        "abi": {
            "methods": [],
            "events": []
        },
        "permissions": [
            {
                "contract": "*",
                "methods": "*"
            },
            {
                "contract": "0x1234567890123456789012345678901234567890",
                "methods": ["specificMethod"]
            }
        ],
        "trusts": [],
        "extra": null
    })";
    
    auto manifest = ContractManifest::FromJson(permission_manifest);
    ASSERT_NE(manifest, nullptr);
    
    const auto& permissions = manifest->GetPermissions();
    EXPECT_EQ(permissions.size(), 2);
    
    // Test wildcard permission
    EXPECT_TRUE(permissions[0].GetContract().IsWildcard());
    EXPECT_TRUE(permissions[0].GetMethods().IsWildcard());
    
    // Test specific permission
    EXPECT_FALSE(permissions[1].GetContract().IsWildcard());
    EXPECT_FALSE(permissions[1].GetMethods().IsWildcard());
    EXPECT_EQ(permissions[1].GetMethods().GetCount(), 1);
}

TEST_F(ContractManifestTest, ParseFromJson_SafeMethods) {
    std::string safe_methods_manifest = R"({
        "name": "SafeMethodsTest",
        "group": [],
        "supportedstandards": [],
        "abi": {
            "methods": [
                {
                    "name": "safeMethod",
                    "parameters": [],
                    "returntype": "String",
                    "offset": 0,
                    "safe": true
                },
                {
                    "name": "unsafeMethod",
                    "parameters": [],
                    "returntype": "String",
                    "offset": 10,
                    "safe": false
                }
            ],
            "events": []
        },
        "permissions": [{"contract": "*", "methods": "*"}],
        "trusts": [],
        "extra": null
    })";
    
    auto manifest = ContractManifest::FromJson(safe_methods_manifest);
    ASSERT_NE(manifest, nullptr);
    
    const auto& methods = manifest->GetAbi().GetMethods();
    EXPECT_EQ(methods.size(), 2);
    
    auto safe_method = std::find_if(methods.begin(), methods.end(),
        [](const auto& m) { return m.GetName() == "safeMethod"; });
    auto unsafe_method = std::find_if(methods.begin(), methods.end(),
        [](const auto& m) { return m.GetName() == "unsafeMethod"; });
    
    ASSERT_NE(safe_method, methods.end());
    ASSERT_NE(unsafe_method, methods.end());
    
    EXPECT_TRUE(safe_method->IsSafe());
    EXPECT_FALSE(unsafe_method->IsSafe());
}

TEST_F(ContractManifestTest, ParseFromJson_Trust) {
    std::string trust_manifest = R"({
        "name": "TrustTest",
        "group": [],
        "supportedstandards": [],
        "abi": {
            "methods": [],
            "events": []
        },
        "permissions": [{"contract": "*", "methods": "*"}],
        "trusts": [
            "0x1234567890123456789012345678901234567890",
            "0x0987654321098765432109876543210987654321"
        ],
        "extra": null
    })";
    
    auto manifest = ContractManifest::FromJson(trust_manifest);
    ASSERT_NE(manifest, nullptr);
    
    const auto& trusts = manifest->GetTrusts();
    EXPECT_FALSE(trusts.IsWildcard());
    EXPECT_EQ(trusts.GetCount(), 2);
    
    // Verify specific trust entries
    auto hash1 = UInt160::Parse("0x1234567890123456789012345678901234567890");
    auto hash2 = UInt160::Parse("0x0987654321098765432109876543210987654321");
    
    EXPECT_TRUE(trusts.Contains(hash1));
    EXPECT_TRUE(trusts.Contains(hash2));
}

TEST_F(ContractManifestTest, ParseFromJson_Groups) {
    // Create a manifest with contract groups
    std::string groups_manifest = R"({
        "name": "GroupsTest",
        "group": [
            {
                "pubkey": "021821807f923a3da004fb73871509d7635bcc05f41edef2a3ca5c941d8bbc1231",
                "signature": "VGVzdCBzaWduYXR1cmUgZGF0YSBmb3IgZGVtb25zdHJhdGlvbiBwdXJwb3Nlcw=="
            }
        ],
        "supportedstandards": [],
        "abi": {
            "methods": [],
            "events": []
        },
        "permissions": [{"contract": "*", "methods": "*"}],
        "trusts": [],
        "extra": null
    })";
    
    auto manifest = ContractManifest::FromJson(groups_manifest);
    ASSERT_NE(manifest, nullptr);
    
    const auto& groups = manifest->GetGroups();
    EXPECT_EQ(groups.size(), 1);
    
    const auto& group = groups[0];
    EXPECT_EQ(group.GetPublicKey(), test_public_key_);
    EXPECT_FALSE(group.GetSignature().empty());
}

TEST_F(ContractManifestTest, ParseFromJson_Extra) {
    std::string extra_manifest = R"({
        "name": "ExtraTest",
        "group": [],
        "supportedstandards": [],
        "abi": {
            "methods": [],
            "events": []
        },
        "permissions": [{"contract": "*", "methods": "*"}],
        "trusts": [],
        "extra": {
            "author": "Test Author",
            "version": "1.0.0",
            "description": "Test contract with extra metadata"
        }
    })";
    
    auto manifest = ContractManifest::FromJson(extra_manifest);
    ASSERT_NE(manifest, nullptr);
    
    const auto& extra = manifest->GetExtra();
    EXPECT_FALSE(extra.empty());
    
    auto extra_json = json::parse(extra);
    EXPECT_EQ(extra_json["author"], "Test Author");
    EXPECT_EQ(extra_json["version"], "1.0.0");
    EXPECT_EQ(extra_json["description"], "Test contract with extra metadata");
}

TEST_F(ContractManifestTest, TestDeserializeAndSerialize) {
    auto original_manifest = ContractManifest::FromJson(nep17_manifest_json_);
    ASSERT_NE(original_manifest, nullptr);
    
    // Serialize to binary
    MemoryStream stream;
    BinaryWriter writer(stream);
    original_manifest->Serialize(writer);
    
    // Deserialize from binary
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    auto deserialized_manifest = std::make_unique<ContractManifest>();
    deserialized_manifest->Deserialize(reader);
    
    // Verify they are equal
    EXPECT_EQ(original_manifest->GetName(), deserialized_manifest->GetName());
    EXPECT_EQ(original_manifest->GetSupportedStandards(), deserialized_manifest->GetSupportedStandards());
    EXPECT_EQ(original_manifest->GetAbi().GetMethods().size(), deserialized_manifest->GetAbi().GetMethods().size());
    EXPECT_EQ(original_manifest->GetPermissions().size(), deserialized_manifest->GetPermissions().size());
}

TEST_F(ContractManifestTest, EqualTests) {
    auto manifest1 = ContractManifest::FromJson(default_manifest_json_);
    auto manifest2 = ContractManifest::FromJson(default_manifest_json_);
    auto manifest3 = ContractManifest::FromJson(nep17_manifest_json_);
    
    ASSERT_NE(manifest1, nullptr);
    ASSERT_NE(manifest2, nullptr);
    ASSERT_NE(manifest3, nullptr);
    
    EXPECT_EQ(*manifest1, *manifest2);
    EXPECT_NE(*manifest1, *manifest3);
}

TEST_F(ContractManifestTest, TestSerializeTrusts) {
    std::string wildcard_trust_manifest = R"({
        "name": "WildcardTrust",
        "group": [],
        "supportedstandards": [],
        "abi": {"methods": [], "events": []},
        "permissions": [{"contract": "*", "methods": "*"}],
        "trusts": "*",
        "extra": null
    })";
    
    auto manifest = ContractManifest::FromJson(wildcard_trust_manifest);
    ASSERT_NE(manifest, nullptr);
    
    // Convert to stack item
    auto stack_item = manifest->ToStackItem();
    ASSERT_NE(stack_item, nullptr);
    
    // Create new manifest from stack item
    auto reconstructed_manifest = ContractManifest::FromStackItem(stack_item);
    ASSERT_NE(reconstructed_manifest, nullptr);
    
    EXPECT_EQ(manifest->GetName(), reconstructed_manifest->GetName());
    EXPECT_EQ(manifest->GetTrusts().IsWildcard(), reconstructed_manifest->GetTrusts().IsWildcard());
}

TEST_F(ContractManifestTest, TestGenerator) {
    // Test basic manifest construction
    ContractManifestBuilder builder;
    auto manifest = builder
        .SetName("GeneratedContract")
        .AddMethod("testMethod", {}, ContractParameterType::Boolean, false)
        .AddEvent("TestEvent", {{"param1", ContractParameterType::String}})
        .AddPermission("*", "*")
        .Build();
    
    ASSERT_NE(manifest, nullptr);
    EXPECT_EQ(manifest->GetName(), "GeneratedContract");
    EXPECT_EQ(manifest->GetAbi().GetMethods().size(), 1);
    EXPECT_EQ(manifest->GetAbi().GetEvents().size(), 1);
    EXPECT_EQ(manifest->GetPermissions().size(), 1);
}

TEST_F(ContractManifestTest, LargeManifestTest) {
    // Test with a large number of methods (performance test)
    ContractManifestBuilder builder;
    builder.SetName("LargeContract");
    
    // Add 100 methods
    for (int i = 0; i < 100; ++i) {
        builder.AddMethod("method" + std::to_string(i), {}, ContractParameterType::Any, i % 2 == 0);
    }
    
    // Add 50 events
    for (int i = 0; i < 50; ++i) {
        builder.AddEvent("event" + std::to_string(i), {});
    }
    
    auto manifest = builder.Build();
    ASSERT_NE(manifest, nullptr);
    
    EXPECT_EQ(manifest->GetAbi().GetMethods().size(), 100);
    EXPECT_EQ(manifest->GetAbi().GetEvents().size(), 50);
    
    // Test serialization performance
    auto start = std::chrono::high_resolution_clock::now();
    
    MemoryStream stream;
    BinaryWriter writer(stream);
    manifest->Serialize(writer);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 100); // Should serialize in less than 100ms
}

TEST_F(ContractManifestTest, InvalidJsonHandling) {
    // Test various invalid JSON scenarios
    std::vector<std::string> invalid_manifests = {
        "invalid json",
        "{}",  // Missing required fields
        R"({"name": ""})",  // Empty name
        R"({"name": "Test", "abi": "invalid"})",  // Invalid ABI
        R"({"name": "Test", "permissions": "invalid"})"  // Invalid permissions
    };
    
    for (const auto& invalid_json : invalid_manifests) {
        EXPECT_THROW(ContractManifest::FromJson(invalid_json), std::exception);
    }
}

TEST_F(ContractManifestTest, MethodParameterValidation) {
    std::string method_params_manifest = R"({
        "name": "MethodParamsTest",
        "group": [],
        "supportedstandards": [],
        "abi": {
            "methods": [
                {
                    "name": "complexMethod",
                    "parameters": [
                        {"name": "hash", "type": "Hash160"},
                        {"name": "amount", "type": "Integer"},
                        {"name": "data", "type": "ByteArray"},
                        {"name": "signature", "type": "Signature"},
                        {"name": "publicKey", "type": "PublicKey"}
                    ],
                    "returntype": "Array",
                    "offset": 0,
                    "safe": false
                }
            ],
            "events": []
        },
        "permissions": [{"contract": "*", "methods": "*"}],
        "trusts": [],
        "extra": null
    })";
    
    auto manifest = ContractManifest::FromJson(method_params_manifest);
    ASSERT_NE(manifest, nullptr);
    
    const auto& methods = manifest->GetAbi().GetMethods();
    EXPECT_EQ(methods.size(), 1);
    
    const auto& method = methods[0];
    EXPECT_EQ(method.GetName(), "complexMethod");
    EXPECT_EQ(method.GetParameters().size(), 5);
    EXPECT_EQ(method.GetReturnType(), ContractParameterType::Array);
    
    // Verify parameter types
    const auto& params = method.GetParameters();
    EXPECT_EQ(params[0].GetType(), ContractParameterType::Hash160);
    EXPECT_EQ(params[1].GetType(), ContractParameterType::Integer);
    EXPECT_EQ(params[2].GetType(), ContractParameterType::ByteArray);
    EXPECT_EQ(params[3].GetType(), ContractParameterType::Signature);
    EXPECT_EQ(params[4].GetType(), ContractParameterType::PublicKey);
}

TEST_F(ContractManifestTest, ToInteroperable_Trust) {
    // Test complex trust scenarios (Oracle contract example from C# tests)
    std::string oracle_trust_manifest = R"({
        "name": "OracleTest",
        "group": [],
        "supportedstandards": [],
        "abi": {
            "methods": [
                {
                    "name": "getResponse",
                    "parameters": [{"name": "requestId", "type": "Integer"}],
                    "returntype": "Array",
                    "offset": 0,
                    "safe": true
                }
            ],
            "events": []
        },
        "permissions": [
            {
                "contract": "0x1234567890123456789012345678901234567890",
                "methods": ["oracleRequest"]
            }
        ],
        "trusts": [
            "0x1234567890123456789012345678901234567890"
        ],
        "extra": null
    })";
    
    auto manifest = ContractManifest::FromJson(oracle_trust_manifest);
    ASSERT_NE(manifest, nullptr);
    
    // Test conversion to stack item and back
    auto stack_item = manifest->ToStackItem();
    ASSERT_NE(stack_item, nullptr);
    
    auto reconstructed = ContractManifest::FromStackItem(stack_item);
    ASSERT_NE(reconstructed, nullptr);
    
    EXPECT_EQ(manifest->GetName(), reconstructed->GetName());
    EXPECT_EQ(manifest->GetTrusts().GetCount(), reconstructed->GetTrusts().GetCount());
    EXPECT_EQ(manifest->GetPermissions().size(), reconstructed->GetPermissions().size());
}