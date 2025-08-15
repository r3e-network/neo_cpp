#include <gtest/gtest.h>
#include <neo/network/p2p/peer.h>
#include <neo/network/p2p/message.h>

TEST(NetworkNewTest, PeerConnection) {
    neo::network::Peer peer("127.0.0.1", 20333);
    EXPECT_EQ(peer.GetAddress(), "127.0.0.1");
    EXPECT_EQ(peer.GetPort(), 20333);
}

TEST(NetworkNewTest, MessageSerialization) {
    neo::network::Message msg;
    msg.SetCommand("ping");
    msg.SetPayload({0x01, 0x02, 0x03});
    
    auto serialized = msg.Serialize();
    EXPECT_FALSE(serialized.empty());
    
    neo::network::Message deserialized;
    EXPECT_TRUE(deserialized.Deserialize(serialized));
    EXPECT_EQ(deserialized.GetCommand(), "ping");
}

TEST(NetworkNewTest, ProtocolVersion) {
    EXPECT_EQ(neo::network::PROTOCOL_VERSION, 0);
}
