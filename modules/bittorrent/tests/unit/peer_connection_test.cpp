#include <bitscrape/testing.hpp>

#include "bitscrape/bittorrent/peer_connection.hpp"
#include "bitscrape/bittorrent/peer_message.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/network/address.hpp"
#include "bitscrape/network/tcp_socket.hpp"

#include <memory>
#include <vector>

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

TEST(PeerConnectionTest, DisconnectWhenNotConnected) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Check initial state
    EXPECT_EQ(connection.state(), PeerConnection::State::DISCONNECTED);

    // Disconnect (should be a no-op)
    connection.disconnect();

    // Check state (should still be disconnected)
    EXPECT_EQ(connection.state(), PeerConnection::State::DISCONNECTED);
}

TEST(PeerConnectionTest, SendMessageWhenNotConnected) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Create handshake message
    std::vector<uint8_t> info_hash_bytes(info_hash.bytes().begin(), info_hash.bytes().end());
    HandshakeMessage message(info_hash_bytes, peer_id);

    // Send message (should fail because not connected)
    bool success = connection.send_message(message);

    // Check result
    EXPECT_FALSE(success);
}

TEST(PeerConnectionTest, ReceiveMessageWhenNotConnected) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Receive message (should return nullptr because not connected)
    auto message = connection.receive_message();

    // Check result
    EXPECT_EQ(message, nullptr);
}

TEST(PeerConnectionTest, SendRawDataWhenNotConnected) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Create raw data
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};

    // Send raw data (should fail because not connected)
    int bytes_sent = connection.send_raw_data(data.data(), data.size());

    // Check result
    EXPECT_EQ(bytes_sent, -1);
}

TEST(PeerConnectionTest, StateChecks) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Check initial state
    EXPECT_EQ(connection.state(), PeerConnection::State::DISCONNECTED);
    EXPECT_TRUE(connection.is_choked());
    EXPECT_FALSE(connection.is_interested());
    EXPECT_TRUE(connection.am_choked());
    EXPECT_FALSE(connection.am_interested());
    EXPECT_TRUE(connection.remote_peer_id().empty());
}

TEST(PeerConnectionTest, AddressAccessor) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Check address
    EXPECT_EQ(connection.address(), address);
    EXPECT_EQ(connection.address().to_string(), "127.0.0.1");
    EXPECT_EQ(connection.address().port(), 6881);
}

TEST(PeerConnectionTest, InfoHashAccessor) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Check info hash
    EXPECT_EQ(connection.info_hash(), info_hash);
    EXPECT_EQ(connection.info_hash().to_hex(), "0102030405060708090a0b0c0d0e0f1011121314");
}

TEST(PeerConnectionTest, PeerIdAccessor) {
    // Create address, info hash, and peer ID
    Address address("127.0.0.1", 6881);
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer connection
    PeerConnection connection(address, info_hash, peer_id);

    // Check peer ID
    EXPECT_EQ(connection.peer_id(), peer_id);
    EXPECT_EQ(connection.peer_id().size(), 20UL);
    for (size_t i = 0; i < 20; ++i) {
        EXPECT_EQ(connection.peer_id()[i], 0x01);
    }
}
