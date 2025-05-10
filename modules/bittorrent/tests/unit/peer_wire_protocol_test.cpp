#include <gtest/gtest.h>

#include "bitscrape/bittorrent/peer_wire_protocol.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/network/address.hpp"

using namespace bitscrape::bittorrent;
using namespace bitscrape::types;
using namespace bitscrape::network;

TEST(PeerWireProtocolTest, Construction) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Check initial state
    EXPECT_EQ(protocol.info_hash(), info_hash);
    EXPECT_EQ(protocol.peer_id(), peer_id);
    EXPECT_TRUE(protocol.connected_peers().empty());
}

TEST(PeerWireProtocolTest, InvalidPeerId) {
    // Create info hash and invalid peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(10, 0x01); // Too short

    // Expect exception
    EXPECT_THROW(PeerWireProtocol protocol(info_hash, peer_id), std::invalid_argument);
}

TEST(PeerWireProtocolTest, RegisterMessageHandler) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Register message handler
    bool handler_called = false;
    protocol.register_message_handler(PeerMessageType::HANDSHAKE,
                                     [&handler_called](const Address&, const PeerMessage&) {
        handler_called = true;
    });

    // Note: We can't test the handler directly because we need a real message
    // and a real peer connection, which would make this an integration test
}

// Note: The following tests require a real peer to connect to, so they are disabled by default

TEST(PeerWireProtocolTest, DISABLED_ConnectToPeer) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create address
    Address address("127.0.0.1", 6881);

    // Connect to peer
    bool success = protocol.connect_to_peer(address);

    // Check result
    EXPECT_TRUE(success);
    EXPECT_TRUE(protocol.is_peer_connected(address));
    EXPECT_EQ(protocol.connected_peers().size(), 1);

    // Disconnect from peer
    protocol.disconnect_from_peer(address);

    // Check result
    EXPECT_FALSE(protocol.is_peer_connected(address));
    EXPECT_TRUE(protocol.connected_peers().empty());
}

TEST(PeerWireProtocolTest, DISABLED_ConnectToPeerAsync) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create address
    Address address("127.0.0.1", 6881);

    // Connect to peer asynchronously
    auto future = protocol.connect_to_peer_async(address);

    // Wait for result
    bool success = future.get();

    // Check result
    EXPECT_TRUE(success);
    EXPECT_TRUE(protocol.is_peer_connected(address));
    EXPECT_EQ(protocol.connected_peers().size(), 1);

    // Disconnect from peer
    protocol.disconnect_from_peer(address);

    // Check result
    EXPECT_FALSE(protocol.is_peer_connected(address));
    EXPECT_TRUE(protocol.connected_peers().empty());
}

TEST(PeerWireProtocolTest, DISABLED_SendMessage) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create address
    Address address("127.0.0.1", 6881);

    // Connect to peer
    bool success = protocol.connect_to_peer(address);
    ASSERT_TRUE(success);

    // Create handshake message
    // Convert InfoHash bytes to vector
    std::vector<uint8_t> info_hash_bytes(info_hash.bytes().begin(), info_hash.bytes().end());
    HandshakeMessage message(info_hash_bytes, peer_id);

    // Send message
    success = protocol.send_message(address, message);

    // Check result
    EXPECT_TRUE(success);

    // Disconnect from peer
    protocol.disconnect_from_peer(address);
}
