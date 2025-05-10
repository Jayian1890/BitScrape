#include <gtest/gtest.h>

#include "bitscrape/bittorrent/peer_message.hpp"
#include "bitscrape/bittorrent/extended_message.hpp"
#include "bitscrape/bencode/bencode_value.hpp"

using namespace bitscrape::bittorrent;
using namespace bitscrape::bencode;

// HandshakeMessage Tests

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

TEST(PeerMessageTest, HandshakeMessageToString) {
    // Create info hash and peer ID
    std::vector<uint8_t> info_hash(20, 0x01);
    std::vector<uint8_t> peer_id(20, 0x02);

    // Create handshake message
    HandshakeMessage message(info_hash, peer_id);

    // Get string representation
    std::string str = message.to_string();

    // Check that it contains the expected information
    EXPECT_NE(str.find("HandshakeMessage"), std::string::npos);
    EXPECT_NE(str.find("info_hash"), std::string::npos);
    EXPECT_NE(str.find("peer_id"), std::string::npos);
}

// KeepAliveMessage Tests

TEST(PeerMessageTest, KeepAliveMessageConstruction) {
    // Create keep-alive message
    KeepAliveMessage message;

    // Check message type
    EXPECT_EQ(message.type(), PeerMessageType::KEEP_ALIVE);
}

TEST(PeerMessageTest, KeepAliveMessageSerialization) {
    // Create keep-alive message
    KeepAliveMessage message;

    // Serialize message
    auto data = message.serialize();

    // Check data size (4 bytes for length prefix with value 0)
    EXPECT_EQ(data.size(), 4UL);

    // Check that all bytes are 0
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(data[i], 0);
    }
}

TEST(PeerMessageTest, KeepAliveMessageToString) {
    // Create keep-alive message
    KeepAliveMessage message;

    // Get string representation
    std::string str = message.to_string();

    // Check that it contains the expected information
    EXPECT_NE(str.find("KeepAliveMessage"), std::string::npos);
}

// ExtendedMessage Tests

TEST(PeerMessageTest, ExtendedMessageConstruction) {
    // Create payload
    std::map<std::string, BencodeValue> m_dict;
    std::map<std::string, BencodeValue> payload_dict;
    payload_dict["m"] = BencodeValue(m_dict);
    payload_dict["v"] = BencodeValue("BitScrape 1.0");
    BencodeValue payload(payload_dict);

    // Create extended message (handshake)
    ExtendedMessage message(0, payload);

    // Check message type
    EXPECT_EQ(message.type(), PeerMessageType::EXTENDED);

    // Check extended message type
    EXPECT_EQ(message.extended_type(), 0);

    // Check payload
    EXPECT_EQ(message.payload().type(), BencodeValue::Type::DICT);
    EXPECT_NE(message.payload().get("m"), nullptr);
    EXPECT_NE(message.payload().get("v"), nullptr);
    EXPECT_EQ(message.payload().get("v")->as_string(), "BitScrape 1.0");
}

TEST(PeerMessageTest, ExtendedMessageSerialization) {
    // Create payload
    std::map<std::string, BencodeValue> m_dict;
    std::map<std::string, BencodeValue> payload_dict;
    payload_dict["m"] = BencodeValue(m_dict);
    payload_dict["v"] = BencodeValue("BitScrape 1.0");
    BencodeValue payload(payload_dict);

    // Create extended message (handshake)
    ExtendedMessage message(0, payload);

    // Serialize message
    auto data = message.serialize();

    // Check that data is not empty
    EXPECT_FALSE(data.empty());

    // First 4 bytes are the length prefix
    // Next byte is the message ID (20 for extended messages)
    // Next byte is the extended message ID (0 for handshake)
    // Rest is the bencoded payload
    EXPECT_GE(data.size(), 6UL);
    EXPECT_EQ(data[4], 20); // Message ID for EXTENDED
    EXPECT_EQ(data[5], 0);  // Extended message ID (0 for handshake)
}

TEST(PeerMessageTest, ExtendedMessageToString) {
    // Create payload
    std::map<std::string, BencodeValue> m_dict;
    std::map<std::string, BencodeValue> payload_dict;
    payload_dict["m"] = BencodeValue(m_dict);
    payload_dict["v"] = BencodeValue("BitScrape 1.0");
    BencodeValue payload(payload_dict);

    // Create extended message (handshake)
    ExtendedMessage message(0, payload);

    // Get string representation
    std::string str = message.to_string();

    // Check that it contains the expected information
    EXPECT_NE(str.find("ExtendedMessage"), std::string::npos);
    EXPECT_NE(str.find("type=0"), std::string::npos);
}

// PeerMessageFactory Tests

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

TEST(PeerMessageTest, PeerMessageFactoryCreateKeepAlive) {
    // Create keep-alive message
    auto message = PeerMessageFactory::create_keep_alive();

    // Check message type
    EXPECT_EQ(message->type(), PeerMessageType::KEEP_ALIVE);

    // Check serialization
    auto data = message->serialize();
    EXPECT_EQ(data.size(), 4UL);
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(data[i], 0);
    }
}
