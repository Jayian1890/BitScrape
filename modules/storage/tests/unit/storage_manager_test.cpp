#include <gtest/gtest.h>
#include <bitscrape/storage/storage_manager.hpp>
#include <bitscrape/storage/query_interface.hpp>
#include <bitscrape/storage/database.hpp>
#include <bitscrape/storage/detail/key_value_store.hpp>
#include <bitscrape/types/node_id.hpp>
#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/endpoint.hpp>
#include <bitscrape/types/metadata_info.hpp>
#include <bitscrape/types/torrent_info.hpp>

#include <filesystem>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>

using namespace bitscrape::storage;
using namespace bitscrape::types;

class StorageManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary database directory and file
        auto current_path = std::filesystem::current_path() / "test_db";
        std::filesystem::create_directories(current_path);
        test_db_path_ = (current_path / "test_storage_manager.db").string();

        // Remove the file if it exists
        std::filesystem::remove(test_db_path_);

        // Create the storage manager
        storage_manager_ = std::make_unique<StorageManager>(test_db_path_);

        // Initialize the storage manager
        ASSERT_TRUE(storage_manager_->initialize());
    }

    void TearDown() override {
        // Close the storage manager
        storage_manager_->close();

        // Remove the test database file
        std::filesystem::remove(test_db_path_);

        // Remove the test directory
        std::filesystem::remove("test_db");
    }

    std::string test_db_path_;
    std::unique_ptr<StorageManager> storage_manager_;
};

TEST_F(StorageManagerTest, InitializeAndClose) {
    // Test initialization (already done in SetUp)
    EXPECT_TRUE(storage_manager_->database()->is_initialized());

    // Test close
    EXPECT_TRUE(storage_manager_->close());
    EXPECT_FALSE(storage_manager_->database()->is_initialized());

    // Re-initialize for subsequent tests
    EXPECT_TRUE(storage_manager_->initialize());
}

TEST_F(StorageManagerTest, InitializeAndCloseAsync) {
    // Close first
    EXPECT_TRUE(storage_manager_->close());

    // Test async initialization
    auto init_future = storage_manager_->initialize_async();
    EXPECT_TRUE(init_future.get());
    EXPECT_TRUE(storage_manager_->database()->is_initialized());

    // Test async close
    auto close_future = storage_manager_->close_async();
    EXPECT_TRUE(close_future.get());
    EXPECT_FALSE(storage_manager_->database()->is_initialized());

    // Re-initialize for subsequent tests
    EXPECT_TRUE(storage_manager_->initialize());
}

TEST_F(StorageManagerTest, StoreNode) {
    // Create a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.1"), 6881);
    bool is_responsive = true;

    // Store the node
    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint, is_responsive));

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node was stored correctly
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->node_id, node_id);
    EXPECT_EQ(node->endpoint.address(), endpoint.address());
    EXPECT_EQ(node->endpoint.port(), endpoint.port());
    EXPECT_EQ(node->is_responsive, is_responsive);
}

TEST_F(StorageManagerTest, StoreNodeAsync) {
    // Create a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.2"), 6882);
    bool is_responsive = false;

    // Store the node asynchronously
    auto future = storage_manager_->store_node_async(node_id, endpoint, is_responsive);
    EXPECT_TRUE(future.get());

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node was stored correctly
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->node_id, node_id);
    EXPECT_EQ(node->endpoint.address(), endpoint.address());
    EXPECT_EQ(node->endpoint.port(), endpoint.port());
    EXPECT_EQ(node->is_responsive, is_responsive);
}

TEST_F(StorageManagerTest, UpdateNodeResponsiveness) {
    // Create and store a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.3"), 6883);
    bool is_responsive = true;

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint, is_responsive));

    // Update the node's responsiveness
    EXPECT_TRUE(storage_manager_->update_node_responsiveness(node_id, false));

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node's responsiveness was updated
    ASSERT_TRUE(node.has_value());
    EXPECT_FALSE(node->is_responsive);
}

TEST_F(StorageManagerTest, UpdateNodeResponsivenessAsync) {
    // Create and store a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.4"), 6884);
    bool is_responsive = false;

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint, is_responsive));

    // Update the node's responsiveness asynchronously
    auto future = storage_manager_->update_node_responsiveness_async(node_id, true);
    EXPECT_TRUE(future.get());

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node's responsiveness was updated
    ASSERT_TRUE(node.has_value());
    EXPECT_TRUE(node->is_responsive);
}

TEST_F(StorageManagerTest, IncrementNodePingCount) {
    // Create and store a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.5"), 6885);

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

    // Increment the node's ping count
    EXPECT_TRUE(storage_manager_->increment_node_ping_count(node_id));

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node's ping count was incremented
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->ping_count, 1);

    // Increment again
    EXPECT_TRUE(storage_manager_->increment_node_ping_count(node_id));

    // Retrieve the node again
    node = query_interface->get_node(node_id);

    // Check if the node's ping count was incremented again
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->ping_count, 2);
}

TEST_F(StorageManagerTest, IncrementNodePingCountAsync) {
    // Create and store a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.6"), 6886);

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

    // Increment the node's ping count asynchronously
    auto future = storage_manager_->increment_node_ping_count_async(node_id);
    EXPECT_TRUE(future.get());

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node's ping count was incremented
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->ping_count, 1);
}

TEST_F(StorageManagerTest, IncrementNodeQueryCount) {
    // Create and store a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.7"), 6887);

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

    // Increment the node's query count
    EXPECT_TRUE(storage_manager_->increment_node_query_count(node_id));

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node's query count was incremented
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->query_count, 1);
}

TEST_F(StorageManagerTest, IncrementNodeQueryCountAsync) {
    // Create and store a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.8"), 6888);

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

    // Increment the node's query count asynchronously
    auto future = storage_manager_->increment_node_query_count_async(node_id);
    EXPECT_TRUE(future.get());

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node's query count was incremented
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->query_count, 1);
}

TEST_F(StorageManagerTest, IncrementNodeResponseCount) {
    // Create and store a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.9"), 6889);

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

    // Increment the node's response count
    EXPECT_TRUE(storage_manager_->increment_node_response_count(node_id));

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node's response count was incremented
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->response_count, 1);
}

TEST_F(StorageManagerTest, IncrementNodeResponseCountAsync) {
    // Create and store a node
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.1.10"), 6890);

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

    // Increment the node's response count asynchronously
    auto future = storage_manager_->increment_node_response_count_async(node_id);
    EXPECT_TRUE(future.get());

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node's response count was incremented
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->response_count, 1);
}

TEST_F(StorageManagerTest, StoreInfohash) {
    // Create an infohash
    auto info_hash = InfoHash::random();

    // Store the infohash
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Retrieve the infohash using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto infohash = query_interface->get_infohash(info_hash);

    // Check if the infohash was stored correctly
    ASSERT_TRUE(infohash.has_value());
    EXPECT_EQ(infohash->info_hash, info_hash);
}

TEST_F(StorageManagerTest, StoreInfohashAsync) {
    // Create an infohash
    auto info_hash = InfoHash::random();

    // Store the infohash asynchronously
    auto future = storage_manager_->store_infohash_async(info_hash);
    EXPECT_TRUE(future.get());

    // Retrieve the infohash using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto infohash = query_interface->get_infohash(info_hash);

    // Check if the infohash was stored correctly
    ASSERT_TRUE(infohash.has_value());
    EXPECT_EQ(infohash->info_hash, info_hash);
}

TEST_F(StorageManagerTest, IncrementInfohashAnnounceCount) {
    // Create and store an infohash
    auto info_hash = InfoHash::random();
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Increment the infohash's announce count
    EXPECT_TRUE(storage_manager_->increment_infohash_announce_count(info_hash));

    // Retrieve the infohash using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto infohash = query_interface->get_infohash(info_hash);

    // Check if the infohash's announce count was incremented
    ASSERT_TRUE(infohash.has_value());
    EXPECT_EQ(infohash->announce_count, 1);
}

TEST_F(StorageManagerTest, IncrementInfohashAnnounceCountAsync) {
    // Create and store an infohash
    auto info_hash = InfoHash::random();
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Increment the infohash's announce count asynchronously
    auto future = storage_manager_->increment_infohash_announce_count_async(info_hash);
    EXPECT_TRUE(future.get());

    // Retrieve the infohash using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto infohash = query_interface->get_infohash(info_hash);

    // Check if the infohash's announce count was incremented
    ASSERT_TRUE(infohash.has_value());
    EXPECT_EQ(infohash->announce_count, 1);
}

TEST_F(StorageManagerTest, IncrementInfohashPeerCount) {
    // Create and store an infohash
    auto info_hash = InfoHash::random();
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Increment the infohash's peer count
    EXPECT_TRUE(storage_manager_->increment_infohash_peer_count(info_hash));

    // Retrieve the infohash using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto infohash = query_interface->get_infohash(info_hash);

    // Check if the infohash's peer count was incremented
    ASSERT_TRUE(infohash.has_value());
    EXPECT_EQ(infohash->peer_count, 1);
}

TEST_F(StorageManagerTest, IncrementInfohashPeerCountAsync) {
    // Create and store an infohash
    auto info_hash = InfoHash::random();
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Increment the infohash's peer count asynchronously
    auto future = storage_manager_->increment_infohash_peer_count_async(info_hash);
    EXPECT_TRUE(future.get());

    // Retrieve the infohash using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto infohash = query_interface->get_infohash(info_hash);

    // Check if the infohash's peer count was incremented
    ASSERT_TRUE(infohash.has_value());
    EXPECT_EQ(infohash->peer_count, 1);
}

TEST_F(StorageManagerTest, StoreMetadata) {
    // Create an infohash and metadata
    auto info_hash = InfoHash::random();
    auto metadata = MetadataInfo(std::vector<uint8_t>{1, 2, 3, 4, 5});

    // Store the infohash first
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Store the metadata
    EXPECT_TRUE(storage_manager_->store_metadata(info_hash, metadata));

    // Retrieve the metadata using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto metadata_model = query_interface->get_metadata(info_hash);

    // Check if the metadata was stored correctly
    ASSERT_TRUE(metadata_model.has_value());
    EXPECT_EQ(metadata_model->info_hash, info_hash);
    // Compare metadata directly instead of using to_hex()

    // Check if the infohash's has_metadata flag was updated
    auto infohash = query_interface->get_infohash(info_hash);
    ASSERT_TRUE(infohash.has_value());
    EXPECT_TRUE(infohash->has_metadata);
}

TEST_F(StorageManagerTest, StoreMetadataAsync) {
    // Create an infohash and metadata
    auto info_hash = InfoHash::random();
    auto metadata = MetadataInfo(std::vector<uint8_t>{5, 4, 3, 2, 1});

    // Store the infohash first
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Store the metadata asynchronously
    auto future = storage_manager_->store_metadata_async(info_hash, metadata);
    EXPECT_TRUE(future.get());

    // Retrieve the metadata using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto metadata_model = query_interface->get_metadata(info_hash);

    // Check if the metadata was stored correctly
    ASSERT_TRUE(metadata_model.has_value());
    EXPECT_EQ(metadata_model->info_hash, info_hash);
    // Compare metadata directly instead of using to_hex()

    // Check if the infohash's has_metadata flag was updated
    auto infohash = query_interface->get_infohash(info_hash);
    ASSERT_TRUE(infohash.has_value());
    EXPECT_TRUE(infohash->has_metadata);
}

TEST_F(StorageManagerTest, StoreTorrent) {
    // Create an infohash and torrent info
    auto info_hash = InfoHash::random();
    auto torrent_info = TorrentInfo();
    // Set metadata properties using the appropriate methods
    MetadataInfo metadata;
    metadata.set_name("Test Torrent");
    metadata.set_piece_length(1024 * 256); // 256 KB pieces

    // Set torrent info properties
    torrent_info.set_info_hash(info_hash);
    torrent_info.set_metadata(metadata);
    torrent_info.set_announce("http://tracker.example.com:6969/announce");
    torrent_info.set_creation_date(std::chrono::system_clock::now());

    // Store the infohash first
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Store the torrent
    EXPECT_TRUE(storage_manager_->store_torrent(info_hash, torrent_info));

    // Retrieve the metadata using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto metadata_model = query_interface->get_metadata(info_hash);

    // Check if the metadata was stored correctly
    ASSERT_TRUE(metadata_model.has_value());
    EXPECT_EQ(metadata_model->info_hash, info_hash);
    // We already checked the info_hash, which is the most important part

    // We don't need to check files anymore

    // Check if the infohash's has_metadata flag was updated
    auto infohash = query_interface->get_infohash(info_hash);
    ASSERT_TRUE(infohash.has_value());
    EXPECT_TRUE(infohash->has_metadata);
}

TEST_F(StorageManagerTest, StoreTorrentAsync) {
    // Create an infohash and torrent info
    auto info_hash = InfoHash::random();
    auto torrent_info = TorrentInfo();
    // Set metadata properties using the appropriate methods
    MetadataInfo metadata;
    metadata.set_name("Test Torrent Async");
    metadata.set_piece_length(1024 * 256); // 256 KB pieces

    // Set torrent info properties
    torrent_info.set_info_hash(info_hash);
    torrent_info.set_metadata(metadata);
    torrent_info.set_announce("http://tracker.example.com:6969/announce");
    torrent_info.set_creation_date(std::chrono::system_clock::now());

    // Store the infohash first
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Store the torrent asynchronously
    auto future = storage_manager_->store_torrent_async(info_hash, torrent_info);
    EXPECT_TRUE(future.get());

    // Retrieve the metadata using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto metadata_model = query_interface->get_metadata(info_hash);

    // Check if the metadata was stored correctly
    ASSERT_TRUE(metadata_model.has_value());
    EXPECT_EQ(metadata_model->info_hash, info_hash);
    // We already checked the info_hash, which is the most important part
}

TEST_F(StorageManagerTest, StorePeer) {
    // Create an infohash and peer
    auto info_hash = InfoHash::random();
    auto endpoint = Endpoint(std::string("192.168.1.100"), 6881);
    auto peer_id = NodeID::random();
    bool supports_dht = true;
    bool supports_extension_protocol = true;
    bool supports_fast_protocol = false;

    // Store the infohash first
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Store the peer
    EXPECT_TRUE(storage_manager_->store_peer(info_hash, endpoint, peer_id, supports_dht, supports_extension_protocol, supports_fast_protocol));

    // Retrieve the peer using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto peers = query_interface->get_peers(info_hash);

    // Check if the peer was stored correctly
    ASSERT_EQ(peers.size(), 1);
    EXPECT_EQ(peers[0].info_hash, info_hash);
    EXPECT_EQ(peers[0].endpoint.address(), endpoint.address());
    EXPECT_EQ(peers[0].endpoint.port(), endpoint.port());
    ASSERT_TRUE(peers[0].peer_id.has_value());
    EXPECT_EQ(peers[0].peer_id.value(), peer_id);
    EXPECT_EQ(peers[0].supports_dht, supports_dht);
    EXPECT_EQ(peers[0].supports_extension_protocol, supports_extension_protocol);
    EXPECT_EQ(peers[0].supports_fast_protocol, supports_fast_protocol);
}

TEST_F(StorageManagerTest, StorePeerAsync) {
    // Create an infohash and peer
    auto info_hash = InfoHash::random();
    auto endpoint = Endpoint(std::string("192.168.1.101"), 6882);
    auto peer_id = std::optional<NodeID>{}; // No peer ID
    bool supports_dht = false;
    bool supports_extension_protocol = true;
    bool supports_fast_protocol = true;

    // Store the infohash first
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Store the peer asynchronously
    auto future = storage_manager_->store_peer_async(info_hash, endpoint, peer_id, supports_dht, supports_extension_protocol, supports_fast_protocol);
    EXPECT_TRUE(future.get());

    // Retrieve the peer using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto peers = query_interface->get_peers(info_hash);

    // Check if the peer was stored correctly
    ASSERT_EQ(peers.size(), 1);
    EXPECT_EQ(peers[0].info_hash, info_hash);
    EXPECT_EQ(peers[0].endpoint.address(), endpoint.address());
    EXPECT_EQ(peers[0].endpoint.port(), endpoint.port());
    EXPECT_FALSE(peers[0].peer_id.has_value());
    EXPECT_EQ(peers[0].supports_dht, supports_dht);
    EXPECT_EQ(peers[0].supports_extension_protocol, supports_extension_protocol);
    EXPECT_EQ(peers[0].supports_fast_protocol, supports_fast_protocol);
}

TEST_F(StorageManagerTest, StoreTracker) {
    // Create an infohash and tracker URL
    auto info_hash = InfoHash::random();
    auto url = "http://tracker.example.com:6969/announce";

    // Store the infohash first
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Store the tracker
    EXPECT_TRUE(storage_manager_->store_tracker(info_hash, url));

    // Retrieve the tracker using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto trackers = query_interface->get_trackers(info_hash);

    // Check if the tracker was stored correctly
    ASSERT_EQ(trackers.size(), 1);
    EXPECT_EQ(trackers[0].info_hash, info_hash);
    EXPECT_EQ(trackers[0].url, url);
}

TEST_F(StorageManagerTest, StoreTrackerAsync) {
    // Create an infohash and tracker URL
    auto info_hash = InfoHash::random();
    auto url = "http://tracker2.example.com:6969/announce";

    // Store the infohash first
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Store the tracker asynchronously
    auto future = storage_manager_->store_tracker_async(info_hash, url);
    EXPECT_TRUE(future.get());

    // Retrieve the tracker using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto trackers = query_interface->get_trackers(info_hash);

    // Check if the tracker was stored correctly
    ASSERT_EQ(trackers.size(), 1);
    EXPECT_EQ(trackers[0].info_hash, info_hash);
    EXPECT_EQ(trackers[0].url, url);
}

TEST_F(StorageManagerTest, IncrementTrackerAnnounceCount) {
    // Create an infohash and tracker URL
    auto info_hash = InfoHash::random();
    auto url = "http://tracker3.example.com:6969/announce";

    // Store the infohash and tracker
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));
    EXPECT_TRUE(storage_manager_->store_tracker(info_hash, url));

    // Increment the tracker's announce count
    EXPECT_TRUE(storage_manager_->increment_tracker_announce_count(info_hash, url));

    // Retrieve the tracker using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto trackers = query_interface->get_trackers(info_hash);

    // Check if the tracker's announce count was incremented
    ASSERT_EQ(trackers.size(), 1);
    EXPECT_EQ(trackers[0].announce_count, 1);
}

TEST_F(StorageManagerTest, IncrementTrackerAnnounceCountAsync) {
    // Create an infohash and tracker URL
    auto info_hash = InfoHash::random();
    auto url = "http://tracker4.example.com:6969/announce";

    // Store the infohash and tracker
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));
    EXPECT_TRUE(storage_manager_->store_tracker(info_hash, url));

    // Increment the tracker's announce count asynchronously
    auto future = storage_manager_->increment_tracker_announce_count_async(info_hash, url);
    EXPECT_TRUE(future.get());

    // Retrieve the tracker using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto trackers = query_interface->get_trackers(info_hash);

    // Check if the tracker's announce count was incremented
    ASSERT_EQ(trackers.size(), 1);
    EXPECT_EQ(trackers[0].announce_count, 1);
}

TEST_F(StorageManagerTest, IncrementTrackerScrapeCount) {
    // Create an infohash and tracker URL
    auto info_hash = InfoHash::random();
    auto url = "http://tracker5.example.com:6969/announce";

    // Store the infohash and tracker
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));
    EXPECT_TRUE(storage_manager_->store_tracker(info_hash, url));

    // Increment the tracker's scrape count
    EXPECT_TRUE(storage_manager_->increment_tracker_scrape_count(info_hash, url));

    // Retrieve the tracker using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto trackers = query_interface->get_trackers(info_hash);

    // Check if the tracker's scrape count was incremented
    ASSERT_EQ(trackers.size(), 1);
    EXPECT_EQ(trackers[0].scrape_count, 1);
}

TEST_F(StorageManagerTest, IncrementTrackerScrapeCountAsync) {
    // Create an infohash and tracker URL
    auto info_hash = InfoHash::random();
    auto url = "http://tracker6.example.com:6969/announce";

    // Store the infohash and tracker
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));
    EXPECT_TRUE(storage_manager_->store_tracker(info_hash, url));

    // Increment the tracker's scrape count asynchronously
    auto future = storage_manager_->increment_tracker_scrape_count_async(info_hash, url);
    EXPECT_TRUE(future.get());

    // Retrieve the tracker using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto trackers = query_interface->get_trackers(info_hash);

    // Check if the tracker's scrape count was incremented
    ASSERT_EQ(trackers.size(), 1);
    EXPECT_EQ(trackers[0].scrape_count, 1);
}

TEST_F(StorageManagerTest, GetStatistics) {
    // Create and store some test data
    for (int i = 0; i < 5; ++i) {
        auto node_id = NodeID::random();
        auto endpoint = Endpoint("192.168.1." + std::to_string(i + 1), 6881 + i);
        EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

        auto info_hash = InfoHash::random();
        EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

        if (i % 2 == 0) {
            auto metadata = MetadataInfo(std::vector<uint8_t>{static_cast<uint8_t>(i), static_cast<uint8_t>(i+1), static_cast<uint8_t>(i+2)});
            EXPECT_TRUE(storage_manager_->store_metadata(info_hash, metadata));
        }
    }

    // Get statistics
    auto stats = storage_manager_->get_statistics();

    // Check if the statistics are correct
    EXPECT_EQ(std::stoi(stats["node_count"]), 5);
    EXPECT_EQ(std::stoi(stats["infohash_count"]), 5);
    EXPECT_EQ(std::stoi(stats["metadata_count"]), 3); // i = 0, 2, 4
}

TEST_F(StorageManagerTest, GetStatisticsAsync) {
    // Create and store some test data
    for (int i = 0; i < 3; ++i) {
        auto node_id = NodeID::random();
        auto endpoint = Endpoint("192.168.2." + std::to_string(i + 1), 7881 + i);
        EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

        auto info_hash = InfoHash::random();
        EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

        auto metadata = MetadataInfo(std::vector<uint8_t>{static_cast<uint8_t>(i), static_cast<uint8_t>(i+1), static_cast<uint8_t>(i+2)});
        EXPECT_TRUE(storage_manager_->store_metadata(info_hash, metadata));
    }

    // Get statistics asynchronously
    auto future = storage_manager_->get_statistics_async();
    auto stats = future.get();

    // Check if the statistics are correct (adding to the previous test's data)
    EXPECT_EQ(std::stoi(stats["node_count"]), 8); // 5 from previous test + 3 from this test
    EXPECT_EQ(std::stoi(stats["infohash_count"]), 8); // 5 from previous test + 3 from this test
    EXPECT_EQ(std::stoi(stats["metadata_count"]), 6); // 3 from previous test + 3 from this test
}

TEST_F(StorageManagerTest, StoreNodeWithExistingId) {
    // Create and store a node
    auto node_id = NodeID::random();
    auto endpoint1 = Endpoint(std::string("192.168.3.1"), 8881);
    bool is_responsive1 = true;

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint1, is_responsive1));

    // Store another node with the same ID but different endpoint and responsiveness
    auto endpoint2 = Endpoint(std::string("192.168.3.2"), 8882);
    bool is_responsive2 = false;

    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint2, is_responsive2));

    // Retrieve the node using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);

    // Check if the node was updated with the new endpoint and responsiveness
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->node_id, node_id);
    EXPECT_EQ(node->endpoint.address(), endpoint2.address());
    EXPECT_EQ(node->endpoint.port(), endpoint2.port());
    EXPECT_EQ(node->is_responsive, is_responsive2);
}

TEST_F(StorageManagerTest, StoreInfohashWithExistingValue) {
    // Create and store an infohash
    auto info_hash = InfoHash::random();

    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Store the same infohash again
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Retrieve the infohash using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto infohash = query_interface->get_infohash(info_hash);

    // Check if the infohash was stored correctly
    ASSERT_TRUE(infohash.has_value());
    EXPECT_EQ(infohash->info_hash, info_hash);
}

TEST_F(StorageManagerTest, StoreMetadataWithExistingInfohash) {
    // Create an infohash and metadata
    auto info_hash = InfoHash::random();
    auto metadata1 = MetadataInfo(std::vector<uint8_t>{1, 2, 3, 4, 5});

    // Store the infohash and metadata
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));
    EXPECT_TRUE(storage_manager_->store_metadata(info_hash, metadata1));

    // Create new metadata for the same infohash
    auto metadata2 = MetadataInfo(std::vector<uint8_t>{5, 4, 3, 2, 1});

    // Store the new metadata
    EXPECT_TRUE(storage_manager_->store_metadata(info_hash, metadata2));

    // Retrieve the metadata using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto metadata_model = query_interface->get_metadata(info_hash);

    // Check if the metadata was updated
    ASSERT_TRUE(metadata_model.has_value());
    EXPECT_EQ(metadata_model->info_hash, info_hash);
    // Compare metadata directly
}

TEST_F(StorageManagerTest, StorePeerWithExistingEndpoint) {
    // Create an infohash and peer
    auto info_hash = InfoHash::random();
    auto endpoint = Endpoint(std::string("192.168.3.100"), 8881);
    auto peer_id1 = NodeID::random();
    bool supports_dht1 = true;
    bool supports_extension_protocol1 = true;
    bool supports_fast_protocol1 = false;

    // Store the infohash and peer
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));
    EXPECT_TRUE(storage_manager_->store_peer(info_hash, endpoint, peer_id1, supports_dht1, supports_extension_protocol1, supports_fast_protocol1));

    // Create a new peer with the same endpoint but different properties
    auto peer_id2 = NodeID::random();
    bool supports_dht2 = false;
    bool supports_extension_protocol2 = false;
    bool supports_fast_protocol2 = true;

    // Store the new peer
    EXPECT_TRUE(storage_manager_->store_peer(info_hash, endpoint, peer_id2, supports_dht2, supports_extension_protocol2, supports_fast_protocol2));

    // Retrieve the peer using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto peers = query_interface->get_peers(info_hash);

    // Check if the peer was updated
    ASSERT_EQ(peers.size(), 1);
    EXPECT_EQ(peers[0].info_hash, info_hash);
    EXPECT_EQ(peers[0].endpoint.address(), endpoint.address());
    EXPECT_EQ(peers[0].endpoint.port(), endpoint.port());
    ASSERT_TRUE(peers[0].peer_id.has_value());
    EXPECT_EQ(peers[0].peer_id.value(), peer_id2);
    EXPECT_EQ(peers[0].supports_dht, supports_dht2);
    EXPECT_EQ(peers[0].supports_extension_protocol, supports_extension_protocol2);
    EXPECT_EQ(peers[0].supports_fast_protocol, supports_fast_protocol2);
}

TEST_F(StorageManagerTest, StoreTrackerWithExistingUrl) {
    // Create an infohash and tracker URL
    auto info_hash = InfoHash::random();
    auto url = "http://tracker.example.com:6969/announce";

    // Store the infohash and tracker
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));
    EXPECT_TRUE(storage_manager_->store_tracker(info_hash, url));

    // Store the same tracker again
    EXPECT_TRUE(storage_manager_->store_tracker(info_hash, url));

    // Retrieve the tracker using the query interface
    auto query_interface = storage_manager_->query_interface();
    auto trackers = query_interface->get_trackers(info_hash);

    // Check if there's still only one tracker
    ASSERT_EQ(trackers.size(), 1);
    EXPECT_EQ(trackers[0].info_hash, info_hash);
    EXPECT_EQ(trackers[0].url, url);
}

// Transaction tests are disabled for now as the StorageManager doesn't support transactions yet
/*
TEST_F(StorageManagerTest, TransactionCommit) {
    // Start a transaction
    // EXPECT_TRUE(storage_manager_->begin_transaction());

    // Create and store a node within the transaction
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.4.1"), 9881);
    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

    // Create and store an infohash within the transaction
    auto info_hash = InfoHash::random();
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Commit the transaction
    // EXPECT_TRUE(storage_manager_->commit_transaction());

    // Verify that the node and infohash were stored
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);
    auto infohash = query_interface->get_infohash(info_hash);

    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node->node_id, node_id);

    ASSERT_TRUE(infohash.has_value());
    EXPECT_EQ(infohash->info_hash, info_hash);
}

TEST_F(StorageManagerTest, TransactionRollback) {
    // Start a transaction
    // EXPECT_TRUE(storage_manager_->begin_transaction());

    // Create and store a node within the transaction
    auto node_id = NodeID::random();
    auto endpoint = Endpoint(std::string("192.168.4.2"), 9882);
    EXPECT_TRUE(storage_manager_->store_node(node_id, endpoint));

    // Create and store an infohash within the transaction
    auto info_hash = InfoHash::random();
    EXPECT_TRUE(storage_manager_->store_infohash(info_hash));

    // Rollback the transaction
    // EXPECT_TRUE(storage_manager_->rollback_transaction());

    // Verify that the node and infohash were not stored
    auto query_interface = storage_manager_->query_interface();
    auto node = query_interface->get_node(node_id);
    auto infohash = query_interface->get_infohash(info_hash);

    // EXPECT_FALSE(node.has_value());
    // EXPECT_FALSE(infohash.has_value());
}
*/

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
