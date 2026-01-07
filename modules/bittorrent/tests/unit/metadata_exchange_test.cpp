#include <bitscrape/testing.hpp>

#include "bitscrape/bittorrent/metadata_exchange.hpp"
#include "bitscrape/bittorrent/peer_wire_protocol.hpp"
#include "bitscrape/bittorrent/extended_message.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/types/metadata_info.hpp"
#include "bitscrape/network/address.hpp"
#include "bitscrape/bencode/bencode_value.hpp"

#include <memory>
#include <vector>
#include <functional>

using namespace bitscrape::bittorrent;
using namespace bitscrape::types;
using namespace bitscrape::network;
using namespace bitscrape::bencode;

TEST(MetadataExchangeTest, Construction) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    MetadataExchange exchange(protocol);

    // Check initial state
    EXPECT_EQ(exchange.metadata(), nullptr);
}

TEST(MetadataExchangeTest, SetMetadataReceivedCallback) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    MetadataExchange exchange(protocol);

    // Set callback
    bool callback_called = false;
    exchange.set_metadata_received_callback([&callback_called](const MetadataInfo&) {
        callback_called = true;
    });

    // Note: We can't test the callback directly because we need real metadata,
    // which would make this an integration test
}

// Note: The following tests require a real peer to connect to, so they are disabled by default

TEST(MetadataExchangeTest, DISABLED_RequestMetadata) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    MetadataExchange exchange(protocol);

    // Initialize
    exchange.initialize();

    // Create address
    Address address("127.0.0.1", 6881);

    // Connect to peer
    bool success = protocol.connect_to_peer(address);
    ASSERT_TRUE(success);

    // Request metadata
    success = exchange.request_metadata(address);

    // Check result
    EXPECT_TRUE(success);

    // Disconnect from peer
    protocol.disconnect_from_peer(address);
}

TEST(MetadataExchangeTest, DISABLED_RequestMetadataAsync) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    MetadataExchange exchange(protocol);

    // Initialize
    exchange.initialize();

    // Create address
    Address address("127.0.0.1", 6881);

    // Connect to peer
    bool success = protocol.connect_to_peer(address);
    ASSERT_TRUE(success);

    // Request metadata asynchronously
    auto future = exchange.request_metadata_async(address);

    // Wait for result
    success = future.get();

    // Check result
    EXPECT_TRUE(success);

    // Disconnect from peer
    protocol.disconnect_from_peer(address);
}

TEST(MetadataExchangeTest, RequestMetadataFromNonConnectedPeer) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    MetadataExchange exchange(protocol);

    // Initialize
    exchange.initialize();

    // Create address
    Address address("127.0.0.1", 6881);

    // Request metadata (should fail because peer is not connected)
    bool success = exchange.request_metadata(address);

    // Check result
    EXPECT_FALSE(success);
}

TEST(MetadataExchangeTest, RequestMetadataAsyncFromNonConnectedPeer) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    MetadataExchange exchange(protocol);

    // Initialize
    exchange.initialize();

    // Create address
    Address address("127.0.0.1", 6881);

    // Request metadata asynchronously (should fail because peer is not connected)
    auto future = exchange.request_metadata_async(address);

    // Wait for result
    bool success = future.get();

    // Check result
    EXPECT_FALSE(success);
}

TEST(MetadataExchangeTest, InitializeMultipleTimes) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    MetadataExchange exchange(protocol);

    // Initialize
    exchange.initialize();

    // Initialize again (should be a no-op)
    exchange.initialize();

    // Check that metadata is still null
    EXPECT_EQ(exchange.metadata(), nullptr);
}

TEST(MetadataExchangeTest, MetadataAccessor) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    MetadataExchange exchange(protocol);

    // Check initial state
    EXPECT_EQ(exchange.metadata(), nullptr);
}

TEST(MetadataExchangeTest, MultipleCallbacks) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    MetadataExchange exchange(protocol);

    // Set first callback
    bool first_callback_called = false;
    exchange.set_metadata_received_callback([&first_callback_called](const MetadataInfo&) {
        first_callback_called = true;
    });

    // Set second callback (should replace the first one)
    bool second_callback_called = false;
    exchange.set_metadata_received_callback([&second_callback_called](const MetadataInfo&) {
        second_callback_called = true;
    });

    // Note: We can't test the callbacks directly because we need real metadata,
    // which would make this an integration test
}
