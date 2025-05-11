#include <gtest/gtest.h>
#include <bitscrape/storage/data_models.hpp>
#include <bitscrape/storage/database.hpp>
#include <bitscrape/types/node_id.hpp>
#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/endpoint.hpp>
#include <bitscrape/types/metadata_info.hpp>
#include <bitscrape/types/torrent_info.hpp>

#include <filesystem>
#include <string>
#include <chrono>
#include <vector>

using namespace bitscrape::storage;
using namespace bitscrape::types;

class DataModelsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary database file
        test_db_path_ = "test_data_models.db";

        // Remove the file if it exists
        std::filesystem::remove(test_db_path_);

        // Create and initialize the database
        db_ = std::make_shared<Database>(test_db_path_);
        ASSERT_TRUE(db_->initialize());

        // Create test tables
        ASSERT_TRUE(db_->execute_update(
            "CREATE TABLE nodes ("
            "node_id BLOB PRIMARY KEY, "
            "ip TEXT NOT NULL, "
            "port INTEGER NOT NULL, "
            "first_seen INTEGER NOT NULL, "
            "last_seen INTEGER NOT NULL, "
            "ping_count INTEGER NOT NULL DEFAULT 0, "
            "query_count INTEGER NOT NULL DEFAULT 0, "
            "response_count INTEGER NOT NULL DEFAULT 0, "
            "is_responsive INTEGER NOT NULL DEFAULT 0"
            ")"
        ));

        ASSERT_TRUE(db_->execute_update(
            "CREATE TABLE infohashes ("
            "info_hash BLOB PRIMARY KEY, "
            "first_seen INTEGER NOT NULL, "
            "last_seen INTEGER NOT NULL, "
            "announce_count INTEGER NOT NULL DEFAULT 0, "
            "peer_count INTEGER NOT NULL DEFAULT 0, "
            "has_metadata INTEGER NOT NULL DEFAULT 0"
            ")"
        ));

        ASSERT_TRUE(db_->execute_update(
            "CREATE TABLE metadata ("
            "info_hash BLOB PRIMARY KEY, "
            "metadata BLOB NOT NULL, "
            "download_time INTEGER NOT NULL, "
            "name TEXT NOT NULL, "
            "total_size INTEGER NOT NULL, "
            "piece_count INTEGER NOT NULL, "
            "file_count INTEGER NOT NULL, "
            "comment TEXT, "
            "created_by TEXT, "
            "creation_date INTEGER"
            ")"
        ));

        ASSERT_TRUE(db_->execute_update(
            "CREATE TABLE files ("
            "info_hash BLOB NOT NULL, "
            "path TEXT NOT NULL, "
            "size INTEGER NOT NULL, "
            "PRIMARY KEY (info_hash, path)"
            ")"
        ));

        ASSERT_TRUE(db_->execute_update(
            "CREATE TABLE trackers ("
            "info_hash BLOB NOT NULL, "
            "url TEXT NOT NULL, "
            "first_seen INTEGER NOT NULL, "
            "last_seen INTEGER NOT NULL, "
            "announce_count INTEGER NOT NULL DEFAULT 0, "
            "scrape_count INTEGER NOT NULL DEFAULT 0, "
            "PRIMARY KEY (info_hash, url)"
            ")"
        ));

        ASSERT_TRUE(db_->execute_update(
            "CREATE TABLE peers ("
            "info_hash BLOB NOT NULL, "
            "ip TEXT NOT NULL, "
            "port INTEGER NOT NULL, "
            "peer_id BLOB, "
            "first_seen INTEGER NOT NULL, "
            "last_seen INTEGER NOT NULL, "
            "supports_dht INTEGER NOT NULL DEFAULT 0, "
            "supports_extension_protocol INTEGER NOT NULL DEFAULT 0, "
            "supports_fast_protocol INTEGER NOT NULL DEFAULT 0, "
            "PRIMARY KEY (info_hash, ip, port)"
            ")"
        ));
    }

    void TearDown() override {
        // Close the database
        db_->close();

        // Remove the test database file
        std::filesystem::remove(test_db_path_);
    }

    std::string test_db_path_;
    std::shared_ptr<Database> db_;
};

TEST_F(DataModelsTest, NodeModelSerialization) {
    // Create a node model
    NodeModel node;
    node.node_id = NodeID::random();
    node.endpoint = Endpoint("192.168.1.1", 6881);
    node.first_seen = std::chrono::system_clock::now();
    node.last_seen = std::chrono::system_clock::now();
    node.ping_count = 5;
    node.query_count = 10;
    node.response_count = 8;
    node.is_responsive = true;

    // Convert to SQL parameters
    auto params = node.to_sql_params();

    // Check parameter count
    EXPECT_EQ(params.size(), 8);

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO nodes (node_id, ip, port, first_seen, last_seen, ping_count, query_count, response_count, is_responsive) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {
            node.node_id.to_hex(),
            node.endpoint.address(),
            std::to_string(node.endpoint.port()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(node.first_seen.time_since_epoch()).count()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(node.last_seen.time_since_epoch()).count()),
            std::to_string(node.ping_count),
            std::to_string(node.query_count),
            std::to_string(node.response_count),
            node.is_responsive ? "1" : "0"
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM nodes WHERE node_id = ?", {node.node_id.to_hex()});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a node model from the result
    auto retrieved_node = NodeModel::from_db_result(result);

    // Check if the retrieved node matches the original
    EXPECT_EQ(retrieved_node.node_id, node.node_id);
    EXPECT_EQ(retrieved_node.endpoint.address(), node.endpoint.address());
    EXPECT_EQ(retrieved_node.endpoint.port(), node.endpoint.port());
    EXPECT_EQ(retrieved_node.ping_count, node.ping_count);
    EXPECT_EQ(retrieved_node.query_count, node.query_count);
    EXPECT_EQ(retrieved_node.response_count, node.response_count);
    EXPECT_EQ(retrieved_node.is_responsive, node.is_responsive);
}

TEST_F(DataModelsTest, InfoHashModelSerialization) {
    // Create an infohash model
    InfoHashModel infohash;
    infohash.info_hash = InfoHash::random();
    infohash.first_seen = std::chrono::system_clock::now();
    infohash.last_seen = std::chrono::system_clock::now();
    infohash.announce_count = 3;
    infohash.peer_count = 7;
    infohash.has_metadata = true;

    // Convert to SQL parameters
    auto params = infohash.to_sql_params();

    // Check parameter count
    EXPECT_EQ(params.size(), 6);

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO infohashes (info_hash, first_seen, last_seen, announce_count, peer_count, has_metadata) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        {
            infohash.info_hash.to_hex(),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(infohash.first_seen.time_since_epoch()).count()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(infohash.last_seen.time_since_epoch()).count()),
            std::to_string(infohash.announce_count),
            std::to_string(infohash.peer_count),
            infohash.has_metadata ? "1" : "0"
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM infohashes WHERE info_hash = ?", {infohash.info_hash.to_hex()});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create an infohash model from the result
    auto retrieved_infohash = InfoHashModel::from_db_result(result);

    // Check if the retrieved infohash matches the original
    EXPECT_EQ(retrieved_infohash.info_hash, infohash.info_hash);
    EXPECT_EQ(retrieved_infohash.announce_count, infohash.announce_count);
    EXPECT_EQ(retrieved_infohash.peer_count, infohash.peer_count);
    EXPECT_EQ(retrieved_infohash.has_metadata, infohash.has_metadata);
}

TEST_F(DataModelsTest, MetadataModelSerialization) {
    // Create a metadata model
    MetadataModel metadata;
    metadata.info_hash = InfoHash::random();
    metadata.metadata = MetadataInfo(std::vector<uint8_t>{1, 2, 3, 4, 5});
    metadata.download_time = std::chrono::system_clock::now();
    metadata.name = "Test Torrent";
    metadata.total_size = 1024 * 1024 * 10; // 10 MB
    metadata.piece_count = 40;
    metadata.file_count = 2;
    metadata.comment = "Test comment";
    metadata.created_by = "BitScrape Test";
    metadata.creation_date = std::chrono::system_clock::now();

    // Convert to SQL parameters
    auto params = metadata.to_sql_params();

    // Check parameter count
    EXPECT_EQ(params.size(), 10);

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO metadata (info_hash, metadata, download_time, name, total_size, piece_count, file_count, comment, created_by, creation_date) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {
            metadata.info_hash.to_hex(),
            metadata.metadata.to_hex(),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(metadata.download_time.time_since_epoch()).count()),
            metadata.name,
            std::to_string(metadata.total_size),
            std::to_string(metadata.piece_count),
            std::to_string(metadata.file_count),
            metadata.comment,
            metadata.created_by,
            metadata.creation_date ? std::to_string(std::chrono::duration_cast<std::chrono::seconds>(metadata.creation_date->time_since_epoch()).count()) : ""
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM metadata WHERE info_hash = ?", {metadata.info_hash.to_hex()});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a metadata model from the result
    auto retrieved_metadata = MetadataModel::from_db_result(result);

    // Check if the retrieved metadata matches the original
    EXPECT_EQ(retrieved_metadata.info_hash, metadata.info_hash);
    EXPECT_EQ(retrieved_metadata.metadata.to_hex(), metadata.metadata.to_hex());
    EXPECT_EQ(retrieved_metadata.name, metadata.name);
    EXPECT_EQ(retrieved_metadata.total_size, metadata.total_size);
    EXPECT_EQ(retrieved_metadata.piece_count, metadata.piece_count);
    EXPECT_EQ(retrieved_metadata.file_count, metadata.file_count);
    EXPECT_EQ(retrieved_metadata.comment, metadata.comment);
    EXPECT_EQ(retrieved_metadata.created_by, metadata.created_by);
    EXPECT_TRUE(retrieved_metadata.creation_date.has_value());
}

TEST_F(DataModelsTest, FileModelSerialization) {
    // Create a file model
    FileModel file;
    file.info_hash = InfoHash::random();
    file.path = "test/file.txt";
    file.size = 1024 * 1024; // 1 MB

    // Convert to SQL parameters
    auto params = file.to_sql_params();

    // Check parameter count
    EXPECT_EQ(params.size(), 3);

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO files (info_hash, path, size) VALUES (?, ?, ?)",
        {
            file.info_hash.to_hex(),
            file.path,
            std::to_string(file.size)
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM files WHERE info_hash = ? AND path = ?",
                              {file.info_hash.to_hex(), file.path});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a file model from the result
    auto retrieved_file = FileModel::from_db_result(result);

    // Check if the retrieved file matches the original
    EXPECT_EQ(retrieved_file.info_hash, file.info_hash);
    EXPECT_EQ(retrieved_file.path, file.path);
    EXPECT_EQ(retrieved_file.size, file.size);
}

TEST_F(DataModelsTest, TrackerModelSerialization) {
    // Create a tracker model
    TrackerModel tracker;
    tracker.info_hash = InfoHash::random();
    tracker.url = "http://tracker.example.com:6969/announce";
    tracker.first_seen = std::chrono::system_clock::now();
    tracker.last_seen = std::chrono::system_clock::now();
    tracker.announce_count = 5;
    tracker.scrape_count = 2;

    // Convert to SQL parameters
    auto params = tracker.to_sql_params();

    // Check parameter count
    EXPECT_EQ(params.size(), 6);

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO trackers (info_hash, url, first_seen, last_seen, announce_count, scrape_count) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        {
            tracker.info_hash.to_hex(),
            tracker.url,
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(tracker.first_seen.time_since_epoch()).count()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(tracker.last_seen.time_since_epoch()).count()),
            std::to_string(tracker.announce_count),
            std::to_string(tracker.scrape_count)
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM trackers WHERE info_hash = ? AND url = ?",
                              {tracker.info_hash.to_hex(), tracker.url});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a tracker model from the result
    auto retrieved_tracker = TrackerModel::from_db_result(result);

    // Check if the retrieved tracker matches the original
    EXPECT_EQ(retrieved_tracker.info_hash, tracker.info_hash);
    EXPECT_EQ(retrieved_tracker.url, tracker.url);
    EXPECT_EQ(retrieved_tracker.announce_count, tracker.announce_count);
    EXPECT_EQ(retrieved_tracker.scrape_count, tracker.scrape_count);
}

TEST_F(DataModelsTest, PeerModelSerialization) {
    // Create a peer model
    PeerModel peer;
    peer.info_hash = InfoHash::random();
    peer.endpoint = Endpoint("192.168.1.2", 51413);
    peer.peer_id = NodeID::random();
    peer.first_seen = std::chrono::system_clock::now();
    peer.last_seen = std::chrono::system_clock::now();
    peer.supports_dht = true;
    peer.supports_extension_protocol = true;
    peer.supports_fast_protocol = false;

    // Convert to SQL parameters
    auto params = peer.to_sql_params();

    // Check parameter count
    EXPECT_EQ(params.size(), 9);

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO peers (info_hash, ip, port, peer_id, first_seen, last_seen, supports_dht, supports_extension_protocol, supports_fast_protocol) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {
            peer.info_hash.to_hex(),
            peer.endpoint.address(),
            std::to_string(peer.endpoint.port()),
            peer.peer_id ? peer.peer_id->to_hex() : "",
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(peer.first_seen.time_since_epoch()).count()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(peer.last_seen.time_since_epoch()).count()),
            peer.supports_dht ? "1" : "0",
            peer.supports_extension_protocol ? "1" : "0",
            peer.supports_fast_protocol ? "1" : "0"
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM peers WHERE info_hash = ? AND ip = ? AND port = ?",
                              {peer.info_hash.to_hex(), peer.endpoint.address(), std::to_string(peer.endpoint.port())});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a peer model from the result
    auto retrieved_peer = PeerModel::from_db_result(result);

    // Check if the retrieved peer matches the original
    EXPECT_EQ(retrieved_peer.info_hash, peer.info_hash);
    EXPECT_EQ(retrieved_peer.endpoint.address(), peer.endpoint.address());
    EXPECT_EQ(retrieved_peer.endpoint.port(), peer.endpoint.port());
    EXPECT_TRUE(retrieved_peer.peer_id.has_value());
    EXPECT_EQ(retrieved_peer.peer_id.value(), peer.peer_id.value());
    EXPECT_EQ(retrieved_peer.supports_dht, peer.supports_dht);
    EXPECT_EQ(retrieved_peer.supports_extension_protocol, peer.supports_extension_protocol);
    EXPECT_EQ(retrieved_peer.supports_fast_protocol, peer.supports_fast_protocol);
}

TEST_F(DataModelsTest, PeerModelWithoutPeerIdSerialization) {
    // Create a peer model without peer_id
    PeerModel peer;
    peer.info_hash = InfoHash::random();
    peer.endpoint = Endpoint("192.168.1.3", 51414);
    peer.peer_id = std::nullopt; // No peer ID
    peer.first_seen = std::chrono::system_clock::now();
    peer.last_seen = std::chrono::system_clock::now();
    peer.supports_dht = false;
    peer.supports_extension_protocol = true;
    peer.supports_fast_protocol = true;

    // Convert to SQL parameters
    auto params = peer.to_sql_params();

    // Check parameter count
    EXPECT_EQ(params.size(), 9);

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO peers (info_hash, ip, port, peer_id, first_seen, last_seen, supports_dht, supports_extension_protocol, supports_fast_protocol) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {
            peer.info_hash.to_hex(),
            peer.endpoint.address(),
            std::to_string(peer.endpoint.port()),
            peer.peer_id ? peer.peer_id->to_hex() : "",
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(peer.first_seen.time_since_epoch()).count()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(peer.last_seen.time_since_epoch()).count()),
            peer.supports_dht ? "1" : "0",
            peer.supports_extension_protocol ? "1" : "0",
            peer.supports_fast_protocol ? "1" : "0"
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM peers WHERE info_hash = ? AND ip = ? AND port = ?",
                              {peer.info_hash.to_hex(), peer.endpoint.address(), std::to_string(peer.endpoint.port())});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a peer model from the result
    auto retrieved_peer = PeerModel::from_db_result(result);

    // Check if the retrieved peer matches the original
    EXPECT_EQ(retrieved_peer.info_hash, peer.info_hash);
    EXPECT_EQ(retrieved_peer.endpoint.address(), peer.endpoint.address());
    EXPECT_EQ(retrieved_peer.endpoint.port(), peer.endpoint.port());
    EXPECT_FALSE(retrieved_peer.peer_id.has_value());
    EXPECT_EQ(retrieved_peer.supports_dht, peer.supports_dht);
    EXPECT_EQ(retrieved_peer.supports_extension_protocol, peer.supports_extension_protocol);
    EXPECT_EQ(retrieved_peer.supports_fast_protocol, peer.supports_fast_protocol);
}

TEST_F(DataModelsTest, MetadataModelWithoutCreationDateSerialization) {
    // Create a metadata model without creation date
    MetadataModel metadata;
    metadata.info_hash = InfoHash::random();
    metadata.metadata = MetadataInfo(std::vector<uint8_t>{5, 4, 3, 2, 1});
    metadata.download_time = std::chrono::system_clock::now();
    metadata.name = "Test Torrent 2";
    metadata.total_size = 1024 * 1024 * 5; // 5 MB
    metadata.piece_count = 20;
    metadata.file_count = 1;
    metadata.comment = "Another test comment";
    metadata.created_by = "BitScrape Test 2";
    metadata.creation_date = std::nullopt; // No creation date

    // Convert to SQL parameters
    auto params = metadata.to_sql_params();

    // Check parameter count
    EXPECT_EQ(params.size(), 10);

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO metadata (info_hash, metadata, download_time, name, total_size, piece_count, file_count, comment, created_by, creation_date) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {
            metadata.info_hash.to_hex(),
            metadata.metadata.to_hex(),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(metadata.download_time.time_since_epoch()).count()),
            metadata.name,
            std::to_string(metadata.total_size),
            std::to_string(metadata.piece_count),
            std::to_string(metadata.file_count),
            metadata.comment,
            metadata.created_by,
            metadata.creation_date ? std::to_string(std::chrono::duration_cast<std::chrono::seconds>(metadata.creation_date->time_since_epoch()).count()) : ""
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM metadata WHERE info_hash = ?", {metadata.info_hash.to_hex()});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a metadata model from the result
    auto retrieved_metadata = MetadataModel::from_db_result(result);

    // Check if the retrieved metadata matches the original
    EXPECT_EQ(retrieved_metadata.info_hash, metadata.info_hash);
    EXPECT_EQ(retrieved_metadata.metadata.to_hex(), metadata.metadata.to_hex());
    EXPECT_EQ(retrieved_metadata.name, metadata.name);
    EXPECT_EQ(retrieved_metadata.total_size, metadata.total_size);
    EXPECT_EQ(retrieved_metadata.piece_count, metadata.piece_count);
    EXPECT_EQ(retrieved_metadata.file_count, metadata.file_count);
    EXPECT_EQ(retrieved_metadata.comment, metadata.comment);
    EXPECT_EQ(retrieved_metadata.created_by, metadata.created_by);
    EXPECT_FALSE(retrieved_metadata.creation_date.has_value());
}

TEST_F(DataModelsTest, MetadataModelWithEmptyOptionalFields) {
    // Create a metadata model with empty optional fields
    MetadataModel metadata;
    metadata.info_hash = InfoHash::random();
    metadata.metadata = MetadataInfo(std::vector<uint8_t>{1, 2, 3});
    metadata.download_time = std::chrono::system_clock::now();
    metadata.name = "Test Torrent Empty Fields";
    metadata.total_size = 1024 * 1024; // 1 MB
    metadata.piece_count = 10;
    metadata.file_count = 1;
    metadata.comment = ""; // Empty comment
    metadata.created_by = ""; // Empty created_by
    metadata.creation_date = std::nullopt; // No creation date

    // Convert to SQL parameters
    auto params = metadata.to_sql_params();

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO metadata (info_hash, metadata, download_time, name, total_size, piece_count, file_count, comment, created_by, creation_date) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {
            metadata.info_hash.to_hex(),
            metadata.metadata.to_hex(),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(metadata.download_time.time_since_epoch()).count()),
            metadata.name,
            std::to_string(metadata.total_size),
            std::to_string(metadata.piece_count),
            std::to_string(metadata.file_count),
            metadata.comment,
            metadata.created_by,
            metadata.creation_date ? std::to_string(std::chrono::duration_cast<std::chrono::seconds>(metadata.creation_date->time_since_epoch()).count()) : ""
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM metadata WHERE info_hash = ?", {metadata.info_hash.to_hex()});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a metadata model from the result
    auto retrieved_metadata = MetadataModel::from_db_result(result);

    // Check if the retrieved metadata matches the original
    EXPECT_EQ(retrieved_metadata.info_hash, metadata.info_hash);
    EXPECT_EQ(retrieved_metadata.metadata.to_hex(), metadata.metadata.to_hex());
    EXPECT_EQ(retrieved_metadata.name, metadata.name);
    EXPECT_EQ(retrieved_metadata.total_size, metadata.total_size);
    EXPECT_EQ(retrieved_metadata.piece_count, metadata.piece_count);
    EXPECT_EQ(retrieved_metadata.file_count, metadata.file_count);
    EXPECT_EQ(retrieved_metadata.comment, metadata.comment);
    EXPECT_EQ(retrieved_metadata.created_by, metadata.created_by);
    EXPECT_FALSE(retrieved_metadata.creation_date.has_value());
}

TEST_F(DataModelsTest, NodeModelWithMaxValues) {
    // Create a node model with maximum values
    NodeModel node;
    node.node_id = NodeID::random();
    node.endpoint = Endpoint("255.255.255.255", 65535); // Max IP and port
    node.first_seen = std::chrono::system_clock::now();
    node.last_seen = std::chrono::system_clock::now();
    node.ping_count = std::numeric_limits<uint32_t>::max();
    node.query_count = std::numeric_limits<uint32_t>::max();
    node.response_count = std::numeric_limits<uint32_t>::max();
    node.is_responsive = true;

    // Convert to SQL parameters
    auto params = node.to_sql_params();

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO nodes (node_id, ip, port, first_seen, last_seen, ping_count, query_count, response_count, is_responsive) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {
            node.node_id.to_hex(),
            node.endpoint.address(),
            std::to_string(node.endpoint.port()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(node.first_seen.time_since_epoch()).count()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(node.last_seen.time_since_epoch()).count()),
            std::to_string(node.ping_count),
            std::to_string(node.query_count),
            std::to_string(node.response_count),
            node.is_responsive ? "1" : "0"
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM nodes WHERE node_id = ?", {node.node_id.to_hex()});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a node model from the result
    auto retrieved_node = NodeModel::from_db_result(result);

    // Check if the retrieved node matches the original
    EXPECT_EQ(retrieved_node.node_id, node.node_id);
    EXPECT_EQ(retrieved_node.endpoint.address(), node.endpoint.address());
    EXPECT_EQ(retrieved_node.endpoint.port(), node.endpoint.port());
    EXPECT_EQ(retrieved_node.ping_count, node.ping_count);
    EXPECT_EQ(retrieved_node.query_count, node.query_count);
    EXPECT_EQ(retrieved_node.response_count, node.response_count);
    EXPECT_EQ(retrieved_node.is_responsive, node.is_responsive);
}

TEST_F(DataModelsTest, InfoHashModelWithMaxValues) {
    // Create an infohash model with maximum values
    InfoHashModel infohash;
    infohash.info_hash = InfoHash::random();
    infohash.first_seen = std::chrono::system_clock::now();
    infohash.last_seen = std::chrono::system_clock::now();
    infohash.announce_count = std::numeric_limits<uint32_t>::max();
    infohash.peer_count = std::numeric_limits<uint32_t>::max();
    infohash.has_metadata = true;

    // Convert to SQL parameters
    auto params = infohash.to_sql_params();

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO infohashes (info_hash, first_seen, last_seen, announce_count, peer_count, has_metadata) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        {
            infohash.info_hash.to_hex(),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(infohash.first_seen.time_since_epoch()).count()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(infohash.last_seen.time_since_epoch()).count()),
            std::to_string(infohash.announce_count),
            std::to_string(infohash.peer_count),
            infohash.has_metadata ? "1" : "0"
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM infohashes WHERE info_hash = ?", {infohash.info_hash.to_hex()});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create an infohash model from the result
    auto retrieved_infohash = InfoHashModel::from_db_result(result);

    // Check if the retrieved infohash matches the original
    EXPECT_EQ(retrieved_infohash.info_hash, infohash.info_hash);
    EXPECT_EQ(retrieved_infohash.announce_count, infohash.announce_count);
    EXPECT_EQ(retrieved_infohash.peer_count, infohash.peer_count);
    EXPECT_EQ(retrieved_infohash.has_metadata, infohash.has_metadata);
}

TEST_F(DataModelsTest, FileModelWithLongPath) {
    // Create a file model with a very long path
    FileModel file;
    file.info_hash = InfoHash::random();
    file.path = std::string(1000, 'a') + "/" + std::string(1000, 'b') + ".txt"; // Very long path
    file.size = 1024 * 1024 * 1024; // 1 GB

    // Convert to SQL parameters
    auto params = file.to_sql_params();

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO files (info_hash, path, size) VALUES (?, ?, ?)",
        {
            file.info_hash.to_hex(),
            file.path,
            std::to_string(file.size)
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM files WHERE info_hash = ?", {file.info_hash.to_hex()});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a file model from the result
    auto retrieved_file = FileModel::from_db_result(result);

    // Check if the retrieved file matches the original
    EXPECT_EQ(retrieved_file.info_hash, file.info_hash);
    EXPECT_EQ(retrieved_file.path, file.path);
    EXPECT_EQ(retrieved_file.size, file.size);
}

TEST_F(DataModelsTest, TrackerModelWithLongURL) {
    // Create a tracker model with a very long URL
    TrackerModel tracker;
    tracker.info_hash = InfoHash::random();
    tracker.url = "http://" + std::string(1000, 'a') + ".example.com:6969/" + std::string(1000, 'b') + "/announce"; // Very long URL
    tracker.first_seen = std::chrono::system_clock::now();
    tracker.last_seen = std::chrono::system_clock::now();
    tracker.announce_count = 100;
    tracker.scrape_count = 50;

    // Convert to SQL parameters
    auto params = tracker.to_sql_params();

    // Insert into database
    ASSERT_TRUE(db_->execute_update(
        "INSERT INTO trackers (info_hash, url, first_seen, last_seen, announce_count, scrape_count) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        {
            tracker.info_hash.to_hex(),
            tracker.url,
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(tracker.first_seen.time_since_epoch()).count()),
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(tracker.last_seen.time_since_epoch()).count()),
            std::to_string(tracker.announce_count),
            std::to_string(tracker.scrape_count)
        }
    ));

    // Query the database
    auto result = db_->execute("SELECT * FROM trackers WHERE info_hash = ?", {tracker.info_hash.to_hex()});

    // Check if we have a result
    ASSERT_TRUE(result.next());

    // Create a tracker model from the result
    auto retrieved_tracker = TrackerModel::from_db_result(result);

    // Check if the retrieved tracker matches the original
    EXPECT_EQ(retrieved_tracker.info_hash, tracker.info_hash);
    EXPECT_EQ(retrieved_tracker.url, tracker.url);
    EXPECT_EQ(retrieved_tracker.announce_count, tracker.announce_count);
    EXPECT_EQ(retrieved_tracker.scrape_count, tracker.scrape_count);
}
