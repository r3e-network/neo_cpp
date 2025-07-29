#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/block_header.h>
#include <neo/network/ip_address.h>
#include <neo/network/p2p/payloads/addr_payload.h>
#include <neo/network/p2p/payloads/get_block_by_index_payload.h>
#include <neo/network/p2p/payloads/get_blocks_payload.h>
#include <neo/network/p2p/payloads/get_data_payload.h>
#include <neo/network/p2p/payloads/headers_payload.h>
#include <neo/network/p2p/payloads/inv_payload.h>
#include <neo/network/p2p/payloads/ping_payload.h>
#include <neo/network/p2p/payloads/version_payload.h>
#include <sstream>
#include <vector>

using namespace neo::network::p2p;
using namespace neo::network::p2p::payloads;
using namespace neo::io;
using namespace neo::network;

TEST(P2PPayloadsTest, VersionPayload_Serialize_Deserialize)
{
    // Create a version payload
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(ServerCapability(NodeCapabilityType::TcpServer, 10333));
    capabilities.push_back(FullNodeCapability(12345));

    VersionPayload payload = VersionPayload::Create(7630401, 123456, "Neo C++ Node", capabilities);
    payload.SetVersion(0);

    // Verify initial values (matching C# implementation)
    EXPECT_EQ(payload.GetNetwork(), 7630401);  // Magic number
    EXPECT_EQ(payload.GetVersion(), 0);
    EXPECT_EQ(payload.GetNonce(), 123456);
    EXPECT_EQ(payload.GetUserAgent(), "Neo C++ Node");
    EXPECT_TRUE(payload.GetAllowCompression());  // Should be true without DisableCompressionCapability
    EXPECT_EQ(payload.GetCapabilities().size(), 2);

    // Verify size calculation
    EXPECT_GT(payload.GetSize(), 0);  // Size should be greater than 0

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    VersionPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetNetwork(), 7630401);
    EXPECT_EQ(deserializedPayload.GetVersion(), 0);
    EXPECT_EQ(deserializedPayload.GetNonce(), 123456);
    EXPECT_EQ(deserializedPayload.GetUserAgent(), "Neo C++ Node");
    EXPECT_TRUE(deserializedPayload.GetAllowCompression());
    EXPECT_EQ(deserializedPayload.GetCapabilities().size(), 2);
    EXPECT_EQ(deserializedPayload.GetCapabilities()[0].GetType(), NodeCapabilityType::TcpServer);
    EXPECT_EQ(static_cast<const ServerCapability&>(deserializedPayload.GetCapabilities()[0]).GetPort(), 10333);
    EXPECT_EQ(deserializedPayload.GetCapabilities()[1].GetType(), NodeCapabilityType::FullNode);
    EXPECT_EQ(static_cast<const FullNodeCapability&>(deserializedPayload.GetCapabilities()[1]).GetStartHeight(), 12345);
}

TEST(P2PPayloadsTest, VersionPayload_DisableCompression)
{
    // Create capabilities including DisableCompression
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(ServerCapability(NodeCapabilityType::TcpServer, 10333));
    capabilities.push_back(FullNodeCapability(12345));
    capabilities.push_back(NodeCapability(NodeCapabilityType::DisableCompression));

    // Create the payload
    VersionPayload payload = VersionPayload::Create(7630401, 123456, "Neo C++ Node", capabilities);

    // Verify that AllowCompression is false with DisableCompressionCapability
    EXPECT_FALSE(payload.GetAllowCompression());

    // Serialize and deserialize
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    VersionPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check that AllowCompression is still false
    EXPECT_FALSE(deserializedPayload.GetAllowCompression());
}

TEST(P2PPayloadsTest, VersionPayload_SerializeJson_DeserializeJson)
{
    // Create a version payload
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(ServerCapability(NodeCapabilityType::TcpServer, 10333));
    capabilities.push_back(FullNodeCapability(12345));

    VersionPayload payload = VersionPayload::Create(7630401, 123456, "Neo C++ Node", capabilities);
    payload.SetVersion(0);

    // Serialize the payload to JSON
    nlohmann::json json = payload.ToJson();

    // Check the JSON
    EXPECT_EQ(json["network"], 7630401);
    EXPECT_EQ(json["version"], 0);
    EXPECT_EQ(json["timestamp"], payload.GetTimestamp());
    EXPECT_EQ(json["nonce"], 123456);
    EXPECT_EQ(json["useragent"], "Neo C++ Node");
    EXPECT_TRUE(json["allowCompression"]);
    EXPECT_EQ(json["capabilities"].size(), 2);
    EXPECT_EQ(json["capabilities"][0]["type"], static_cast<uint8_t>(NodeCapabilityType::TcpServer));
    EXPECT_EQ(json["capabilities"][0]["port"], 10333);
    EXPECT_EQ(json["capabilities"][1]["type"], static_cast<uint8_t>(NodeCapabilityType::FullNode));
    EXPECT_EQ(json["capabilities"][1]["start_height"], 12345);

    // Deserialize the payload from JSON
    VersionPayload deserializedPayload;
    deserializedPayload.DeserializeFromJson(json);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetNetwork(), 7630401);
    EXPECT_EQ(deserializedPayload.GetVersion(), 0);
    EXPECT_EQ(deserializedPayload.GetNonce(), 123456);
    EXPECT_EQ(deserializedPayload.GetUserAgent(), "Neo C++ Node");
    EXPECT_TRUE(deserializedPayload.GetAllowCompression());
    EXPECT_EQ(deserializedPayload.GetCapabilities().size(), 2);
    EXPECT_EQ(deserializedPayload.GetCapabilities()[0].GetType(), NodeCapabilityType::TcpServer);
    EXPECT_EQ(static_cast<const ServerCapability&>(deserializedPayload.GetCapabilities()[0]).GetPort(), 10333);
    EXPECT_EQ(deserializedPayload.GetCapabilities()[1].GetType(), NodeCapabilityType::FullNode);
    EXPECT_EQ(static_cast<const FullNodeCapability&>(deserializedPayload.GetCapabilities()[1]).GetStartHeight(), 12345);
}

TEST(P2PPayloadsTest, PingPayload_Serialize_Deserialize)
{
    // Create a ping payload using Create method with one parameter
    PingPayload payload = PingPayload::Create(12345);
    payload.SetNonce(67890);
    payload.SetTimestamp(123456789);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    PingPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload - verify correct serialization order
    EXPECT_EQ(deserializedPayload.GetLastBlockIndex(), 12345);
    EXPECT_EQ(deserializedPayload.GetNonce(), 67890);
    EXPECT_EQ(deserializedPayload.GetTimestamp(), 123456789);

    // Check the size matches C# implementation
    EXPECT_EQ(payload.GetSize(), 12);  // 4 bytes each for LastBlockIndex, Timestamp, and Nonce
}

TEST(P2PPayloadsTest, PingPayload_CreateWithNonce)
{
    // Create a ping payload using Create method with two parameters
    PingPayload payload = PingPayload::Create(12345, 67890);
    payload.SetTimestamp(123456789);  // Override timestamp for testing

    // Verify fields
    EXPECT_EQ(payload.GetLastBlockIndex(), 12345);
    EXPECT_EQ(payload.GetNonce(), 67890);
    EXPECT_EQ(payload.GetTimestamp(), 123456789);

    // Test serialization
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize and verify
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    PingPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    EXPECT_EQ(deserializedPayload.GetLastBlockIndex(), 12345);
    EXPECT_EQ(deserializedPayload.GetNonce(), 67890);
    EXPECT_EQ(deserializedPayload.GetTimestamp(), 123456789);
}

TEST(P2PPayloadsTest, PingPayload_SerializeJson_DeserializeJson)
{
    // Create a ping payload
    PingPayload payload = PingPayload::Create(12345);
    payload.SetNonce(67890);
    payload.SetTimestamp(123456789);

    // Serialize the payload to JSON
    nlohmann::json json = payload.ToJson();

    // Check the JSON - use correct key names matching the updated implementation
    EXPECT_EQ(json["lastBlockIndex"], 12345);
    EXPECT_EQ(json["nonce"], 67890);
    EXPECT_EQ(json["timestamp"], 123456789);

    // Deserialize the payload from JSON
    PingPayload deserializedPayload;
    deserializedPayload.DeserializeFromJson(json);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetLastBlockIndex(), 12345);
    EXPECT_EQ(deserializedPayload.GetNonce(), 67890);
    EXPECT_EQ(deserializedPayload.GetTimestamp(), 123456789);
}

TEST(P2PPayloadsTest, AddrPayload_Serialize_Deserialize)
{
    // Create an addr payload
    std::vector<NetworkAddressWithTime> addresses;

    std::vector<NodeCapability> capabilities1;
    capabilities1.push_back(ServerCapability(NodeCapabilityType::TcpServer, 10333));
    addresses.push_back(NetworkAddressWithTime(123456789, IPAddress::Parse("127.0.0.1"), capabilities1));

    std::vector<NodeCapability> capabilities2;
    capabilities2.push_back(ServerCapability(NodeCapabilityType::TcpServer, 20333));
    capabilities2.push_back(FullNodeCapability(12345));
    addresses.push_back(NetworkAddressWithTime(987654321, IPAddress::Parse("192.168.1.1"), capabilities2));

    AddrPayload payload(addresses);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    AddrPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetAddressList().size(), 2);
    EXPECT_EQ(deserializedPayload.GetAddressList()[0].GetTimestamp(), 123456789);
    EXPECT_EQ(deserializedPayload.GetAddressList()[0].GetAddress().ToString(), "127.0.0.1");
    EXPECT_EQ(deserializedPayload.GetAddressList()[0].GetCapabilities().size(), 1);
    EXPECT_EQ(deserializedPayload.GetAddressList()[0].GetCapabilities()[0].GetType(), NodeCapabilityType::TcpServer);
    EXPECT_EQ(
        static_cast<const ServerCapability&>(deserializedPayload.GetAddressList()[0].GetCapabilities()[0]).GetPort(),
        10333);
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetTimestamp(), 987654321);
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetAddress().ToString(), "192.168.1.1");
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetCapabilities().size(), 2);
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetCapabilities()[0].GetType(), NodeCapabilityType::TcpServer);
    EXPECT_EQ(
        static_cast<const ServerCapability&>(deserializedPayload.GetAddressList()[1].GetCapabilities()[0]).GetPort(),
        20333);
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetCapabilities()[1].GetType(), NodeCapabilityType::FullNode);
    EXPECT_EQ(static_cast<const FullNodeCapability&>(deserializedPayload.GetAddressList()[1].GetCapabilities()[1])
                  .GetStartHeight(),
              12345);
}

TEST(P2PPayloadsTest, AddrPayload_SerializeJson_DeserializeJson)
{
    // Create an addr payload
    std::vector<NetworkAddressWithTime> addresses;

    std::vector<NodeCapability> capabilities1;
    capabilities1.push_back(ServerCapability(NodeCapabilityType::TcpServer, 10333));
    addresses.push_back(NetworkAddressWithTime(123456789, IPAddress::Parse("127.0.0.1"), capabilities1));

    std::vector<NodeCapability> capabilities2;
    capabilities2.push_back(ServerCapability(NodeCapabilityType::TcpServer, 20333));
    capabilities2.push_back(FullNodeCapability(12345));
    addresses.push_back(NetworkAddressWithTime(987654321, IPAddress::Parse("192.168.1.1"), capabilities2));

    AddrPayload payload(addresses);

    // Serialize the payload to JSON
    nlohmann::json json = payload.ToJson();

    // Check the JSON
    EXPECT_EQ(json["addresses"].size(), 2);
    EXPECT_EQ(json["addresses"][0]["timestamp"], 123456789);
    EXPECT_EQ(json["addresses"][0]["address"], "127.0.0.1");
    EXPECT_EQ(json["addresses"][0]["capabilities"].size(), 1);
    EXPECT_EQ(json["addresses"][0]["capabilities"][0]["type"], static_cast<uint8_t>(NodeCapabilityType::TcpServer));
    EXPECT_EQ(json["addresses"][0]["capabilities"][0]["port"], 10333);
    EXPECT_EQ(json["addresses"][1]["timestamp"], 987654321);
    EXPECT_EQ(json["addresses"][1]["address"], "192.168.1.1");
    EXPECT_EQ(json["addresses"][1]["capabilities"].size(), 2);
    EXPECT_EQ(json["addresses"][1]["capabilities"][0]["type"], static_cast<uint8_t>(NodeCapabilityType::TcpServer));
    EXPECT_EQ(json["addresses"][1]["capabilities"][0]["port"], 20333);
    EXPECT_EQ(json["addresses"][1]["capabilities"][1]["type"], static_cast<uint8_t>(NodeCapabilityType::FullNode));
    EXPECT_EQ(json["addresses"][1]["capabilities"][1]["start_height"], 12345);

    // Deserialize the payload from JSON
    AddrPayload deserializedPayload;
    deserializedPayload.DeserializeFromJson(json);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetAddressList().size(), 2);
    EXPECT_EQ(deserializedPayload.GetAddressList()[0].GetTimestamp(), 123456789);
    EXPECT_EQ(deserializedPayload.GetAddressList()[0].GetAddress().ToString(), "127.0.0.1");
    EXPECT_EQ(deserializedPayload.GetAddressList()[0].GetCapabilities().size(), 1);
    EXPECT_EQ(deserializedPayload.GetAddressList()[0].GetCapabilities()[0].GetType(), NodeCapabilityType::TcpServer);
    EXPECT_EQ(
        static_cast<const ServerCapability&>(deserializedPayload.GetAddressList()[0].GetCapabilities()[0]).GetPort(),
        10333);
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetTimestamp(), 987654321);
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetAddress().ToString(), "192.168.1.1");
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetCapabilities().size(), 2);
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetCapabilities()[0].GetType(), NodeCapabilityType::TcpServer);
    EXPECT_EQ(
        static_cast<const ServerCapability&>(deserializedPayload.GetAddressList()[1].GetCapabilities()[0]).GetPort(),
        20333);
    EXPECT_EQ(deserializedPayload.GetAddressList()[1].GetCapabilities()[1].GetType(), NodeCapabilityType::FullNode);
    EXPECT_EQ(static_cast<const FullNodeCapability&>(deserializedPayload.GetAddressList()[1].GetCapabilities()[1])
                  .GetStartHeight(),
              12345);
}

TEST(P2PPayloadsTest, InvPayload_Serialize_Deserialize)
{
    // Create an inv payload
    std::vector<neo::io::UInt256> hashes;
    hashes.push_back(neo::io::UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"));
    hashes.push_back(neo::io::UInt256::Parse("FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210"));

    InvPayload payload(InventoryType::Block, hashes);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    InvPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetInventories().size(), 2);
    EXPECT_EQ(deserializedPayload.GetInventories()[0].GetType(), InventoryType::Block);
    EXPECT_EQ(deserializedPayload.GetInventories()[0].GetHash().ToHexString(),
              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(deserializedPayload.GetInventories()[1].GetType(), InventoryType::Block);
    EXPECT_EQ(deserializedPayload.GetInventories()[1].GetHash().ToHexString(),
              "FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210");
}

TEST(P2PPayloadsTest, GetDataPayload_Serialize_Deserialize)
{
    // Create a getdata payload
    std::vector<neo::io::UInt256> hashes;
    hashes.push_back(neo::io::UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"));
    hashes.push_back(neo::io::UInt256::Parse("FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210"));

    GetDataPayload payload(InventoryType::Block, hashes);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    GetDataPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetInventories().size(), 2);
    EXPECT_EQ(deserializedPayload.GetInventories()[0].GetType(), InventoryType::Block);
    EXPECT_EQ(deserializedPayload.GetInventories()[0].GetHash().ToHexString(),
              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(deserializedPayload.GetInventories()[1].GetType(), InventoryType::Block);
    EXPECT_EQ(deserializedPayload.GetInventories()[1].GetHash().ToHexString(),
              "FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210");
}

TEST(P2PPayloadsTest, GetBlocksPayload_Serialize_Deserialize)
{
    // Create a getblocks payload
    neo::io::UInt256 hashStart =
        neo::io::UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    uint16_t count = 500;

    GetBlocksPayload payload(hashStart);
    payload.SetCount(count);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    GetBlocksPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetHashStart().ToHexString(),
              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(deserializedPayload.GetCount(), 500);
}

TEST(P2PPayloadsTest, GetBlockByIndexPayload_Serialize_Deserialize)
{
    // Create a getblockbyindex payload
    uint32_t indexStart = 12345;
    uint16_t count = 500;

    GetBlockByIndexPayload payload(indexStart, count);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    GetBlockByIndexPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetIndexStart(), 12345);
    EXPECT_EQ(deserializedPayload.GetCount(), 500);
}

TEST(P2PPayloadsTest, HeadersPayload_Serialize_Deserialize)
{
    // Create a headers payload
    std::vector<std::shared_ptr<neo::ledger::BlockHeader>> headers;

    auto header1 = std::make_shared<neo::ledger::BlockHeader>();
    header1->SetVersion(0);
    header1->SetPrevHash(neo::io::UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"));
    header1->SetMerkleRoot(neo::io::UInt256::Parse("FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210"));
    header1->SetTimestamp(123456789);
    header1->SetIndex(1);
    headers.push_back(header1);

    auto header2 = std::make_shared<neo::ledger::BlockHeader>();
    header2->SetVersion(0);
    header2->SetPrevHash(header1->GetHash());
    header2->SetMerkleRoot(neo::io::UInt256::Parse("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"));
    header2->SetTimestamp(987654321);
    header2->SetIndex(2);
    headers.push_back(header2);

    HeadersPayload payload(headers);

    // Serialize the payload
    std::ostringstream stream;
    BinaryWriter writer(stream);
    payload.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the payload
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    HeadersPayload deserializedPayload;
    deserializedPayload.Deserialize(reader);

    // Check the deserialized payload
    EXPECT_EQ(deserializedPayload.GetHeaders().size(), 2);
    EXPECT_EQ(deserializedPayload.GetHeaders()[0]->GetVersion(), 0);
    EXPECT_EQ(deserializedPayload.GetHeaders()[0]->GetPrevHash().ToHexString(),
              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(deserializedPayload.GetHeaders()[0]->GetMerkleRoot().ToHexString(),
              "FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210FEDCBA9876543210");
    EXPECT_EQ(deserializedPayload.GetHeaders()[0]->GetTimestamp(), 123456789);
    EXPECT_EQ(deserializedPayload.GetHeaders()[0]->GetIndex(), 1);
    EXPECT_EQ(deserializedPayload.GetHeaders()[1]->GetVersion(), 0);
    EXPECT_EQ(deserializedPayload.GetHeaders()[1]->GetPrevHash(), header1->GetHash());
    EXPECT_EQ(deserializedPayload.GetHeaders()[1]->GetMerkleRoot().ToHexString(),
              "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");
    EXPECT_EQ(deserializedPayload.GetHeaders()[1]->GetTimestamp(), 987654321);
    EXPECT_EQ(deserializedPayload.GetHeaders()[1]->GetIndex(), 2);
}
