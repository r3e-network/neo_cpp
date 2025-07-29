#include <chrono>
#include <gtest/gtest.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/node_capability.h>
#include <neo/network/p2p/peer.h>
#include <neo/network/p2p/peer_list.h>
#include <sstream>
#include <vector>

using namespace neo::network::p2p;
using namespace neo::network;
using namespace neo::io;

TEST(PeerTest, Constructor)
{
    // Default constructor
    Peer peer1;
    EXPECT_EQ(peer1.GetVersion(), 0);
    EXPECT_EQ(peer1.GetLastConnectionTime(), 0);
    EXPECT_EQ(peer1.GetLastSeenTime(), 0);
    EXPECT_EQ(peer1.GetConnectionAttempts(), 0);
    EXPECT_FALSE(peer1.IsConnected());
    EXPECT_FALSE(peer1.IsBad());

    // Endpoint constructor
    IPEndPoint endpoint(IPAddress::Parse("127.0.0.1"), 10333);
    Peer peer2(endpoint);
    EXPECT_EQ(peer2.GetEndPoint(), endpoint);
    EXPECT_EQ(peer2.GetVersion(), 0);
    EXPECT_EQ(peer2.GetLastConnectionTime(), 0);
    EXPECT_EQ(peer2.GetLastSeenTime(), 0);
    EXPECT_EQ(peer2.GetConnectionAttempts(), 0);
    EXPECT_FALSE(peer2.IsConnected());
    EXPECT_FALSE(peer2.IsBad());

    // Full constructor
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(ServerCapability(NodeCapabilityType::TcpServer, 10333));
    capabilities.push_back(FullNodeCapability(12345));
    Peer peer3(endpoint, 0, capabilities);
    EXPECT_EQ(peer3.GetEndPoint(), endpoint);
    EXPECT_EQ(peer3.GetVersion(), 0);
    EXPECT_EQ(peer3.GetCapabilities().size(), 2);
    EXPECT_EQ(peer3.GetLastConnectionTime(), 0);
    EXPECT_EQ(peer3.GetLastSeenTime(), 0);
    EXPECT_EQ(peer3.GetConnectionAttempts(), 0);
    EXPECT_FALSE(peer3.IsConnected());
    EXPECT_FALSE(peer3.IsBad());
}

TEST(PeerTest, Setters)
{
    Peer peer;

    // Set endpoint
    IPEndPoint endpoint(IPAddress::Parse("127.0.0.1"), 10333);
    peer.SetEndPoint(endpoint);
    EXPECT_EQ(peer.GetEndPoint(), endpoint);

    // Set version
    peer.SetVersion(0);
    EXPECT_EQ(peer.GetVersion(), 0);

    // Set capabilities
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(ServerCapability(NodeCapabilityType::TcpServer, 10333));
    capabilities.push_back(FullNodeCapability(12345));
    peer.SetCapabilities(capabilities);
    EXPECT_EQ(peer.GetCapabilities().size(), 2);

    // Set last connection time
    peer.SetLastConnectionTime(123456789);
    EXPECT_EQ(peer.GetLastConnectionTime(), 123456789);

    // Set last seen time
    peer.SetLastSeenTime(987654321);
    EXPECT_EQ(peer.GetLastSeenTime(), 987654321);

    // Set connection attempts
    peer.SetConnectionAttempts(5);
    EXPECT_EQ(peer.GetConnectionAttempts(), 5);

    // Increment connection attempts
    peer.IncrementConnectionAttempts();
    EXPECT_EQ(peer.GetConnectionAttempts(), 6);

    // Set connected
    peer.SetConnected(true);
    EXPECT_TRUE(peer.IsConnected());
    EXPECT_EQ(peer.GetConnectionAttempts(), 0);

    // Set bad
    peer.SetBad(true);
    EXPECT_TRUE(peer.IsBad());
}

TEST(PeerTest, Serialize_Deserialize)
{
    // Create a peer
    IPEndPoint endpoint(IPAddress::Parse("127.0.0.1"), 10333);
    std::vector<NodeCapability> capabilities;
    capabilities.push_back(ServerCapability(NodeCapabilityType::TcpServer, 10333));
    capabilities.push_back(FullNodeCapability(12345));
    Peer peer(endpoint, 0, capabilities);
    peer.SetLastConnectionTime(123456789);
    peer.SetLastSeenTime(987654321);
    peer.SetConnectionAttempts(5);
    peer.SetBad(true);

    // Serialize the peer
    std::ostringstream stream;
    BinaryWriter writer(stream);
    peer.Serialize(writer);
    std::string data = stream.str();

    // Deserialize the peer
    std::istringstream inputStream(data);
    BinaryReader reader(inputStream);
    Peer deserializedPeer;
    deserializedPeer.Deserialize(reader);

    // Check the deserialized peer
    EXPECT_EQ(deserializedPeer.GetEndPoint(), endpoint);
    EXPECT_EQ(deserializedPeer.GetVersion(), 0);
    EXPECT_EQ(deserializedPeer.GetCapabilities().size(), 2);
    EXPECT_EQ(deserializedPeer.GetLastConnectionTime(), 123456789);
    EXPECT_EQ(deserializedPeer.GetLastSeenTime(), 987654321);
    EXPECT_EQ(deserializedPeer.GetConnectionAttempts(), 5);
    EXPECT_TRUE(deserializedPeer.IsBad());
    EXPECT_FALSE(deserializedPeer.IsConnected());
}

TEST(PeerListTest, AddPeer)
{
    PeerList peerList;

    // Add a peer
    IPEndPoint endpoint1(IPAddress::Parse("127.0.0.1"), 10333);
    Peer peer1(endpoint1);
    EXPECT_TRUE(peerList.AddPeer(peer1));
    EXPECT_EQ(peerList.GetCount(), 1);

    // Add another peer
    IPEndPoint endpoint2(IPAddress::Parse("192.168.1.1"), 10333);
    Peer peer2(endpoint2);
    EXPECT_TRUE(peerList.AddPeer(peer2));
    EXPECT_EQ(peerList.GetCount(), 2);

    // Add a duplicate peer
    EXPECT_FALSE(peerList.AddPeer(peer1));
    EXPECT_EQ(peerList.GetCount(), 2);
}

TEST(PeerListTest, GetPeer)
{
    PeerList peerList;

    // Add a peer
    IPEndPoint endpoint(IPAddress::Parse("127.0.0.1"), 10333);
    Peer peer(endpoint);
    peerList.AddPeer(peer);

    // Get the peer
    auto retrievedPeer = peerList.GetPeer(endpoint);
    EXPECT_NE(retrievedPeer, nullptr);
    EXPECT_EQ(retrievedPeer->GetEndPoint(), endpoint);

    // Get a non-existent peer
    IPEndPoint nonExistentEndpoint(IPAddress::Parse("192.168.1.1"), 10333);
    EXPECT_EQ(peerList.GetPeer(nonExistentEndpoint), nullptr);
}

TEST(PeerListTest, UpdatePeer)
{
    PeerList peerList;

    // Add a peer
    IPEndPoint endpoint(IPAddress::Parse("127.0.0.1"), 10333);
    Peer peer(endpoint);
    peerList.AddPeer(peer);

    // Update the peer
    peer.SetVersion(0);
    peer.SetLastConnectionTime(123456789);
    EXPECT_TRUE(peerList.UpdatePeer(peer));

    // Check the updated peer
    auto retrievedPeer = peerList.GetPeer(endpoint);
    EXPECT_NE(retrievedPeer, nullptr);
    EXPECT_EQ(retrievedPeer->GetVersion(), 0);
    EXPECT_EQ(retrievedPeer->GetLastConnectionTime(), 123456789);

    // Update a non-existent peer
    IPEndPoint nonExistentEndpoint(IPAddress::Parse("192.168.1.1"), 10333);
    Peer nonExistentPeer(nonExistentEndpoint);
    EXPECT_FALSE(peerList.UpdatePeer(nonExistentPeer));
}

TEST(PeerListTest, RemovePeer)
{
    PeerList peerList;

    // Add a peer
    IPEndPoint endpoint(IPAddress::Parse("127.0.0.1"), 10333);
    Peer peer(endpoint);
    peerList.AddPeer(peer);

    // Remove the peer
    EXPECT_TRUE(peerList.RemovePeer(endpoint));
    EXPECT_EQ(peerList.GetCount(), 0);

    // Remove a non-existent peer
    EXPECT_FALSE(peerList.RemovePeer(endpoint));
}

TEST(PeerListTest, GetPeers)
{
    PeerList peerList;

    // Add peers
    IPEndPoint endpoint1(IPAddress::Parse("127.0.0.1"), 10333);
    Peer peer1(endpoint1);
    peer1.SetConnected(true);
    peerList.AddPeer(peer1);

    IPEndPoint endpoint2(IPAddress::Parse("192.168.1.1"), 10333);
    Peer peer2(endpoint2);
    peer2.SetBad(true);
    peerList.AddPeer(peer2);

    IPEndPoint endpoint3(IPAddress::Parse("10.0.0.1"), 10333);
    Peer peer3(endpoint3);
    peerList.AddPeer(peer3);

    // Get all peers
    auto allPeers = peerList.GetPeers();
    EXPECT_EQ(allPeers.size(), 3);

    // Get connected peers
    auto connectedPeers = peerList.GetConnectedPeers();
    EXPECT_EQ(connectedPeers.size(), 1);
    EXPECT_EQ(connectedPeers[0].GetEndPoint(), endpoint1);

    // Get unconnected peers
    auto unconnectedPeers = peerList.GetUnconnectedPeers();
    EXPECT_EQ(unconnectedPeers.size(), 1);
    EXPECT_EQ(unconnectedPeers[0].GetEndPoint(), endpoint3);

    // Get good peers
    auto goodPeers = peerList.GetGoodPeers();
    EXPECT_EQ(goodPeers.size(), 2);

    // Get bad peers
    auto badPeers = peerList.GetBadPeers();
    EXPECT_EQ(badPeers.size(), 1);
    EXPECT_EQ(badPeers[0].GetEndPoint(), endpoint2);
}
