#include <gtest/gtest.h>

#include "bitscrape/bittorrent/peer_message.hpp"

using namespace bitscrape::bittorrent;

TEST(PeerMessageTest, HandshakeMessageConstruction) {
    // Create info hash and peer ID
    std::vector<uint8_t> info_hash(20, 0x01);
    std::vector<uint8_t> peer_id(20, 0x02);
    
    // Create handshake message
    HandshakeMessage message(info_hash, peer_id);
    
    // Check message type
    EXPECT_EQ(message.type(), PeerMessageType::HANDSHAKE);
    
    // Check info hash
    EXPECT_EQ(message.info_hash(), info_hash);
    
    // Check peer ID
    EXPECT_EQ(message.peer_id(), peer_id);
    
    // Check reserved field
    std::vector<uint8_t> expected_reserved(8, 0);
    EXPECT_EQ(message.reserved(), expected_reserved);
}

TEST(PeerMessageTest, HandshakeMessageSerialization) {
    // Create info hash and peer ID
    std::vector<uint8_t> info_hash(20, 0x01);
    std::vector<uint8_t> peer_id(20, 0x02);
    
    // Create handshake message
    HandshakeMessage message(info_hash, peer_id);
    
    // Serialize message
    auto data = message.serialize();
    
    // Check data size
    EXPECT_EQ(data.size(), 68);
    
    // Check protocol length
    EXPECT_EQ(data[0], 19);
    
    // Check protocol string
    std::string protocol = "BitTorrent protocol";
    for (size_t i = 0; i < protocol.size(); ++i) {
        EXPECT_EQ(data[1 + i], protocol[i]);
    }
    
    // Check reserved field
    for (size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(data[20 + i], 0);
    }
    
    // Check info hash
    for (size_t i = 0; i < 20; ++i) {
        EXPECT_EQ(data[28 + i], info_hash[i]);
    }
    
    // Check peer ID
    for (size_t i = 0; i < 20; ++i) {
        EXPECT_EQ(data[48 + i], peer_id[i]);
    }
}

TEST(PeerMessageTest, HandshakeMessageInvalidInfoHash) {
    // Create invalid info hash and valid peer ID
    std::vector<uint8_t> info_hash(10, 0x01); // Too short
    std::vector<uint8_t> peer_id(20, 0x02);
    
    // Expect exception
    EXPECT_THROW(HandshakeMessage message(info_hash, peer_id), std::invalid_argument);
}

TEST(PeerMessageTest, HandshakeMessageInvalidPeerId) {
    // Create valid info hash and invalid peer ID
    std::vector<uint8_t> info_hash(20, 0x01);
    std::vector<uint8_t> peer_id(10, 0x02); // Too short
    
    // Expect exception
    EXPECT_THROW(HandshakeMessage message(info_hash, peer_id), std::invalid_argument);
}

TEST(PeerMessageTest, HandshakeMessageCustomReserved) {
    // Create info hash, peer ID, and custom reserved field
    std::vector<uint8_t> info_hash(20, 0x01);
    std::vector<uint8_t> peer_id(20, 0x02);
    std::vector<uint8_t> reserved(8, 0x03);
    
    // Create handshake message
    HandshakeMessage message(info_hash, peer_id, reserved);
    
    // Check reserved field
    EXPECT_EQ(message.reserved(), reserved);
    
    // Serialize message
    auto data = message.serialize();
    
    // Check reserved field in serialized data
    for (size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(data[20 + i], reserved[i]);
    }
}

TEST(PeerMessageTest, PeerMessageFactoryCreateHandshake) {
    // Create info hash and peer ID
    std::vector<uint8_t> info_hash(20, 0x01);
    std::vector<uint8_t> peer_id(20, 0x02);
    
    // Create handshake message
    auto message = PeerMessageFactory::create_handshake(info_hash, peer_id);
    
    // Check message type
    EXPECT_EQ(message->type(), PeerMessageType::HANDSHAKE);
    
    // Check info hash
    EXPECT_EQ(message->info_hash(), info_hash);
    
    // Check peer ID
    EXPECT_EQ(message->peer_id(), peer_id);
}
