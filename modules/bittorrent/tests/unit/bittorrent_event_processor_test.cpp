#include <gtest/gtest.h>

#include "bitscrape/bittorrent/bittorrent_event_processor.hpp"
#include "bitscrape/bittorrent/peer_manager.hpp"
#include "bitscrape/bittorrent/metadata_exchange.hpp"
#include "bitscrape/event/event_bus.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/network/address.hpp"

using namespace bitscrape::bittorrent;
using namespace bitscrape::event;
using namespace bitscrape::types;
using namespace bitscrape::network;

// Mock EventBus for testing
class MockEventBus : public EventBus {
public:
    MockEventBus() : next_token_id_(1) {}
    ~MockEventBus() override = default;

    // Delete copy and move operations
    MockEventBus(const MockEventBus&) = delete;
    MockEventBus& operator=(const MockEventBus&) = delete;
    MockEventBus(MockEventBus&&) = delete;
    MockEventBus& operator=(MockEventBus&&) = delete;

    // Implement the pure virtual methods
    bool unsubscribe(const SubscriptionToken& token) override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Find the handler with the given token
        for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
            if (it->token == token) {
                // Remove the handler
                handlers_.erase(it);
                return true;
            }
        }

        return false;
    }

protected:
    void publish_event(const Event& event) override {
        std::lock_guard<std::mutex> lock(mutex_);

        // Call all handlers
        for (const auto& handler : handlers_) {
            handler.handler(event);
        }
    }

private:
    struct Handler {
        SubscriptionToken token;
        std::function<void(const Event&)> handler;

        Handler(SubscriptionToken t, std::function<void(const Event&)> h)
            : token(t), handler(h) {}
    };

    std::vector<Handler> handlers_;
    std::mutex mutex_;
    uint64_t next_token_id_;
};

TEST(BitTorrentEventProcessorTest, Construction) {
    // Create event processor
    BitTorrentEventProcessor processor;

    // Check initial state
    // Note: We can't directly check the internal state of the processor
}

// BitTorrentEvent is abstract, so we can't test it directly
// Instead, we'll test the concrete implementations

TEST(BitTorrentEventProcessorTest, PeerDiscoveredEventConstruction) {
    // Create info hash and address
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    Address address("127.0.0.1", 6881);

    // Create peer discovered event
    PeerDiscoveredEvent event(info_hash, address);

    // Check event type
    EXPECT_EQ(event.bittorrent_event_type(), BitTorrentEventType::PEER_DISCOVERED);

    // Check info hash and address
    EXPECT_EQ(event.info_hash(), info_hash);
    EXPECT_EQ(event.address(), address);

    // Check to_string
    std::string expected = "PeerDiscoveredEvent[info_hash=" + info_hash.to_hex() + ", address=" + address.to_string() + "]";
    EXPECT_EQ(event.to_string(), expected);
}

TEST(BitTorrentEventProcessorTest, MetadataReceivedEventConstruction) {
    // Create info hash and metadata
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    MetadataInfo metadata;

    // Create metadata received event
    MetadataReceivedEvent event(info_hash, metadata);

    // Check event type
    EXPECT_EQ(event.bittorrent_event_type(), BitTorrentEventType::METADATA_RECEIVED);

    // Check info hash and metadata
    EXPECT_EQ(event.info_hash(), info_hash);
    // Note: We can't directly compare metadata objects

    // Check to_string
    std::string expected = "MetadataReceivedEvent[info_hash=" + info_hash.to_hex() + "]";
    EXPECT_EQ(event.to_string(), expected);
}

TEST(BitTorrentEventProcessorTest, StartStop) {
    // Create event bus
    MockEventBus event_bus;

    // Create event processor
    BitTorrentEventProcessor processor;

    // Start
    processor.start(event_bus);

    // Check if running
    EXPECT_TRUE(processor.is_running());

    // Stop
    processor.stop();

    // Check if stopped
    EXPECT_FALSE(processor.is_running());
}

TEST(BitTorrentEventProcessorTest, AddRemovePeerManager) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer manager
    auto peer_manager = std::make_shared<PeerManager>(info_hash, peer_id);

    // Create event processor
    BitTorrentEventProcessor processor;

    // Add peer manager
    processor.add_peer_manager(info_hash, peer_manager);

    // Remove peer manager
    processor.remove_peer_manager(info_hash);

    // Note: We can't directly check if the peer manager was added or removed
}

TEST(BitTorrentEventProcessorTest, AddRemoveMetadataExchange) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    auto metadata_exchange = std::make_shared<MetadataExchange>(protocol);

    // Create event processor
    BitTorrentEventProcessor processor;

    // Add metadata exchange
    processor.add_metadata_exchange(info_hash, metadata_exchange);

    // Remove metadata exchange
    processor.remove_metadata_exchange(info_hash);

    // Note: We can't directly check if the metadata exchange was added or removed
}

TEST(BitTorrentEventProcessorTest, ProcessPeerDiscoveredEvent) {
    // Create event bus
    MockEventBus event_bus;

    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer manager
    auto peer_manager = std::make_shared<PeerManager>(info_hash, peer_id);

    // Create event processor
    BitTorrentEventProcessor processor;

    // Start
    processor.start(event_bus);

    // Add peer manager
    processor.add_peer_manager(info_hash, peer_manager);

    // Create address
    Address address("127.0.0.1", 6881);

    // Create peer discovered event
    PeerDiscoveredEvent event(info_hash, address);

    // Process event
    bool success = processor.process_event(event);

    // Check result
    EXPECT_TRUE(success);

    // Check that the peer was added to the peer manager
    auto known_peers = peer_manager->known_peers();
    EXPECT_EQ(known_peers.size(), 1);
    EXPECT_EQ(known_peers[0], address);

    // Stop
    processor.stop();
}

TEST(BitTorrentEventProcessorTest, ProcessPeerDiscoveredEventWithoutPeerManager) {
    // Create event bus
    MockEventBus event_bus;

    // Create info hash
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));

    // Create event processor
    BitTorrentEventProcessor processor;

    // Start
    processor.start(event_bus);

    // Create address
    Address address("127.0.0.1", 6881);

    // Create peer discovered event
    PeerDiscoveredEvent event(info_hash, address);

    // Process event (should succeed even if there's no peer manager for the info hash)
    bool success = processor.process_event(event);

    // Check result
    EXPECT_TRUE(success);

    // Stop
    processor.stop();
}

TEST(BitTorrentEventProcessorTest, ProcessMetadataReceivedEvent) {
    // Create event bus
    MockEventBus event_bus;

    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer wire protocol
    PeerWireProtocol protocol(info_hash, peer_id);

    // Create metadata exchange
    auto metadata_exchange = std::make_shared<MetadataExchange>(protocol);

    // Create event processor
    BitTorrentEventProcessor processor;

    // Start
    processor.start(event_bus);

    // Add metadata exchange
    processor.add_metadata_exchange(info_hash, metadata_exchange);

    // Create metadata
    MetadataInfo metadata;

    // Create metadata received event
    MetadataReceivedEvent event(info_hash, metadata);

    // Process event
    bool success = processor.process_event(event);

    // Check result
    EXPECT_TRUE(success);

    // Stop
    processor.stop();
}

TEST(BitTorrentEventProcessorTest, ProcessMetadataReceivedEventWithoutMetadataExchange) {
    // Create event bus
    MockEventBus event_bus;

    // Create info hash
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));

    // Create event processor
    BitTorrentEventProcessor processor;

    // Start
    processor.start(event_bus);

    // Create metadata
    MetadataInfo metadata;

    // Create metadata received event
    MetadataReceivedEvent event(info_hash, metadata);

    // Process event (should succeed even if there's no metadata exchange for the info hash)
    bool success = processor.process_event(event);

    // Check result
    EXPECT_TRUE(success);

    // Stop
    processor.stop();
}

TEST(BitTorrentEventProcessorTest, ProcessEventWhenNotRunning) {
    // Create info hash and address
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    Address address("127.0.0.1", 6881);

    // Create peer discovered event
    PeerDiscoveredEvent event(info_hash, address);

    // Create event processor
    BitTorrentEventProcessor processor;

    // Process event (should succeed even if the processor is not running)
    bool success = processor.process_event(event);

    // Check result
    EXPECT_TRUE(success);
}

TEST(BitTorrentEventProcessorTest, StartTwice) {
    // Create event bus
    MockEventBus event_bus;

    // Create event processor
    BitTorrentEventProcessor processor;

    // Start
    processor.start(event_bus);

    // Check if running
    EXPECT_TRUE(processor.is_running());

    // Start again (should be a no-op)
    processor.start(event_bus);

    // Check if still running
    EXPECT_TRUE(processor.is_running());

    // Stop
    processor.stop();
}

TEST(BitTorrentEventProcessorTest, StopTwice) {
    // Create event bus
    MockEventBus event_bus;

    // Create event processor
    BitTorrentEventProcessor processor;

    // Start
    processor.start(event_bus);

    // Stop
    processor.stop();

    // Check if stopped
    EXPECT_FALSE(processor.is_running());

    // Stop again (should be a no-op)
    processor.stop();

    // Check if still stopped
    EXPECT_FALSE(processor.is_running());
}
