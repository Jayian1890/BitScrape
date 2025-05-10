#include <gtest/gtest.h>

#include "bitscrape/bittorrent/peer_connection.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/network/address.hpp"

using namespace bitscrape::bittorrent;
using namespace bitscrape::types;
using namespace bitscrape::network;

TEST(PeerConnectionTest, Construction) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Check initial state
    EXPECT_EQ(connection.state(), PeerConnection::State::DISCONNECTED);
    EXPECT_EQ(connection.address(), address);
    EXPECT_EQ(connection.info_hash(), info_hash);
    EXPECT_EQ(connection.peer_id(), peer_id);
    EXPECT_TRUE(connection.remote_peer_id().empty());
    EXPECT_TRUE(connection.is_choked());
    EXPECT_FALSE(connection.is_interested());
    EXPECT_TRUE(connection.am_choked());
    EXPECT_FALSE(connection.am_interested());
}

TEST(PeerConnectionTest, InvalidPeerId) {
    // Create address, info hash, and invalid peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(10, 0x01); // Too short

    // Expect exception
    EXPECT_THROW(PeerConnection connection(address, info_hash, peer_id), std::invalid_argument);
}

// Note: The following tests require a real peer to connect to, so they are disabled by default

TEST(PeerConnectionTest, DISABLED_Connect) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Connect
    bool success = connection.connect();

    // Check result
    EXPECT_TRUE(success);
    EXPECT_EQ(connection.state(), PeerConnection::State::CONNECTED);

    // Disconnect
    connection.disconnect();

    // Check state
    EXPECT_EQ(connection.state(), PeerConnection::State::DISCONNECTED);
}

TEST(PeerConnectionTest, DISABLED_ConnectAsync) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Connect asynchronously
    auto future = connection.connect_async();

    // Wait for result
    bool success = future.get();

    // Check result
    EXPECT_TRUE(success);
    EXPECT_EQ(connection.state(), PeerConnection::State::CONNECTED);

    // Disconnect
    connection.disconnect();

    // Check state
    EXPECT_EQ(connection.state(), PeerConnection::State::DISCONNECTED);
}

TEST(PeerConnectionTest, DISABLED_SendMessage) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Connect
    bool success = connection.connect();
    ASSERT_TRUE(success);

    // Create handshake message
    // Convert InfoHash bytes to vector
    std::vector<uint8_t> info_hash_bytes(info_hash.bytes().begin(), info_hash.bytes().end());
    HandshakeMessage message(info_hash_bytes, peer_id);

    // Send message
    success = connection.send_message(message);

    // Check result
    EXPECT_TRUE(success);

    // Disconnect
    connection.disconnect();
}

TEST(PeerConnectionTest, DISABLED_ReceiveMessage) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Connect
    bool success = connection.connect();
    ASSERT_TRUE(success);

    // Receive message
    auto message = connection.receive_message();

    // Check result
    EXPECT_NE(message, nullptr);

    // Disconnect
    connection.disconnect();
}
