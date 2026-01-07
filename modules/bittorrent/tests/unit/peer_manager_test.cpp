#include <bitscrape/testing.hpp>

#include "bitscrape/bittorrent/peer_manager.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/network/address.hpp"

using namespace bitscrape::bittorrent;
using namespace bitscrape::types;
using namespace bitscrape::network;

TEST(PeerManagerTest, Construction) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer manager
    PeerManager manager(info_hash, peer_id);

    // Check initial state
    EXPECT_EQ(manager.info_hash(), info_hash);
    EXPECT_EQ(manager.peer_id(), peer_id);
    EXPECT_EQ(manager.max_connections(), 50);
    EXPECT_TRUE(manager.known_peers().empty());
    EXPECT_TRUE(manager.connected_peers().empty());
}

TEST(PeerManagerTest, InvalidPeerId) {
    // Create info hash and invalid peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(10, 0x01); // Too short

    // Expect exception
    EXPECT_THROW(PeerManager manager(info_hash, peer_id), std::invalid_argument);
}

TEST(PeerManagerTest, AddPeer) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer manager
    PeerManager manager(info_hash, peer_id);

    // Create address
    Address address("127.0.0.1", 6881);

    // Add peer
    manager.add_peer(address);

    // Check known peers
    auto known_peers = manager.known_peers();
    EXPECT_EQ(known_peers.size(), 1);
    EXPECT_EQ(known_peers[0], address);
}

TEST(PeerManagerTest, AddPeers) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer manager
    PeerManager manager(info_hash, peer_id);

    // Create addresses
    std::vector<Address> addresses = {
        Address("127.0.0.1", 6881),
        Address("127.0.0.1", 6882),
        Address("127.0.0.1", 6883)
    };

    // Add peers
    manager.add_peers(addresses);

    // Check known peers
    auto known_peers = manager.known_peers();
    EXPECT_EQ(known_peers.size(), 3);

    // Check that all addresses are in known peers
    for (const auto& address : addresses) {
        bool found = false;
        for (const auto& known_address : known_peers) {
            if (known_address.to_string() == address.to_string()) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }
}

TEST(PeerManagerTest, RemovePeer) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer manager
    PeerManager manager(info_hash, peer_id);

    // Create addresses
    std::vector<Address> addresses = {
        Address("127.0.0.1", 6881),
        Address("127.0.0.1", 6882),
        Address("127.0.0.1", 6883)
    };

    // Add peers
    manager.add_peers(addresses);

    // Remove peer
    manager.remove_peer(addresses[1]);

    // Check known peers
    auto known_peers = manager.known_peers();
    EXPECT_EQ(known_peers.size(), 2);

    // Check that the removed address is not in known peers
    for (const auto& known_address : known_peers) {
        // Check both address and port
        bool is_removed_address = (known_address.to_string() == addresses[1].to_string() &&
                                  known_address.port() == addresses[1].port());
        EXPECT_FALSE(is_removed_address);
    }
}

TEST(PeerManagerTest, SetMaxConnections) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer manager
    PeerManager manager(info_hash, peer_id);

    // Set max connections
    manager.set_max_connections(100);

    // Check max connections
    EXPECT_EQ(manager.max_connections(), 100);
}

// Note: The following tests require a real peer to connect to, so they are disabled by default

TEST(PeerManagerTest, DISABLED_Start) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer manager
    PeerManager manager(info_hash, peer_id);

    // Start
    bool success = manager.start();

    // Check result
    EXPECT_TRUE(success);

    // Stop
    manager.stop();
}

TEST(PeerManagerTest, DISABLED_StartAsync) {
    // Create info hash and peer ID
    InfoHash info_hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    std::vector<uint8_t> peer_id(20, 0x01);

    // Create peer manager
    PeerManager manager(info_hash, peer_id);

    // Start asynchronously
    auto future = manager.start_async();

    // Wait for result
    bool success = future.get();

    // Check result
    EXPECT_TRUE(success);

    // Stop
    manager.stop();
}
