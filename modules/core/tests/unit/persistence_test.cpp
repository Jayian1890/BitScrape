#include <gtest/gtest.h>
#include <bitscrape/core/persistence.hpp>
#include <bitscrape/types/node_id.hpp>
#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/endpoint.hpp>

#include <filesystem>
#include <thread>
#include <chrono>

namespace bitscrape::core::test {

class PersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary database file
        db_path_ = "persistence_test.db";
        
        // Remove any existing test file
        std::filesystem::remove(db_path_);
    }
    
    void TearDown() override {
        // Clean up test file
        std::filesystem::remove(db_path_);
    }
    
    std::string db_path_;
};

TEST_F(PersistenceTest, ConstructorTest) {
    // Test default constructor
    Persistence persistence;
    
    // Test constructor with db path
    Persistence persistence_with_path(db_path_);
}

TEST_F(PersistenceTest, InitializeCloseTest) {
    Persistence persistence(db_path_);
    
    // Test initialization
    EXPECT_TRUE(persistence.initialize());
    EXPECT_TRUE(std::filesystem::exists(db_path_));
    
    // Test close
    EXPECT_TRUE(persistence.close());
}

TEST_F(PersistenceTest, InitializeCloseAsyncTest) {
    Persistence persistence(db_path_);
    
    // Test asynchronous initialization
    auto init_future = persistence.initialize_async();
    EXPECT_TRUE(init_future.get());
    EXPECT_TRUE(std::filesystem::exists(db_path_));
    
    // Test asynchronous close
    auto close_future = persistence.close_async();
    EXPECT_TRUE(close_future.get());
}

TEST_F(PersistenceTest, StoreGetNodeTest) {
    Persistence persistence(db_path_);
    EXPECT_TRUE(persistence.initialize());
    
    // Create a node
    types::NodeID node_id = types::NodeID::random();
    types::Endpoint endpoint("192.168.1.1", 6881);
    
    // Test store_node
    EXPECT_TRUE(persistence.store_node(node_id, endpoint));
    
    // Test get_node
    auto result = persistence.get_node(node_id);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(endpoint.get_address(), result->get_address());
    EXPECT_EQ(endpoint.get_port(), result->get_port());
    
    // Test get_node with non-existent node
    types::NodeID non_existent_node = types::NodeID::random();
    auto non_existent_result = persistence.get_node(non_existent_node);
    EXPECT_FALSE(non_existent_result.has_value());
    
    // Clean up
    EXPECT_TRUE(persistence.close());
}

TEST_F(PersistenceTest, StoreGetNodeAsyncTest) {
    Persistence persistence(db_path_);
    EXPECT_TRUE(persistence.initialize());
    
    // Create a node
    types::NodeID node_id = types::NodeID::random();
    types::Endpoint endpoint("192.168.1.1", 6881);
    
    // Test store_node_async
    auto store_future = persistence.store_node_async(node_id, endpoint);
    EXPECT_TRUE(store_future.get());
    
    // Test get_node_async
    auto get_future = persistence.get_node_async(node_id);
    auto result = get_future.get();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(endpoint.get_address(), result->get_address());
    EXPECT_EQ(endpoint.get_port(), result->get_port());
    
    // Clean up
    EXPECT_TRUE(persistence.close());
}

TEST_F(PersistenceTest, GetNodesTest) {
    Persistence persistence(db_path_);
    EXPECT_TRUE(persistence.initialize());
    
    // Create some nodes
    std::vector<std::pair<types::NodeID, types::Endpoint>> nodes;
    for (int i = 0; i < 10; ++i) {
        types::NodeID node_id = types::NodeID::random();
        types::Endpoint endpoint("192.168.1." + std::to_string(i + 1), 6881 + i);
        nodes.emplace_back(node_id, endpoint);
        EXPECT_TRUE(persistence.store_node(node_id, endpoint));
    }
    
    // Test get_nodes with default parameters
    auto result = persistence.get_nodes();
    EXPECT_EQ(10UL, result.size());
    
    // Test get_nodes with limit
    auto limited_result = persistence.get_nodes(5);
    EXPECT_EQ(5UL, limited_result.size());
    
    // Test get_nodes with limit and offset
    auto offset_result = persistence.get_nodes(5, 5);
    EXPECT_EQ(5UL, offset_result.size());
    
    // Clean up
    EXPECT_TRUE(persistence.close());
}

TEST_F(PersistenceTest, GetNodesAsyncTest) {
    Persistence persistence(db_path_);
    EXPECT_TRUE(persistence.initialize());
    
    // Create some nodes
    std::vector<std::pair<types::NodeID, types::Endpoint>> nodes;
    for (int i = 0; i < 10; ++i) {
        types::NodeID node_id = types::NodeID::random();
        types::Endpoint endpoint("192.168.1." + std::to_string(i + 1), 6881 + i);
        nodes.emplace_back(node_id, endpoint);
        EXPECT_TRUE(persistence.store_node(node_id, endpoint));
    }
    
    // Test get_nodes_async with default parameters
    auto future = persistence.get_nodes_async();
    auto result = future.get();
    EXPECT_EQ(10UL, result.size());
    
    // Clean up
    EXPECT_TRUE(persistence.close());
}

TEST_F(PersistenceTest, StoreGetInfohashTest) {
    Persistence persistence(db_path_);
    EXPECT_TRUE(persistence.initialize());
    
    // Create an infohash
    types::InfoHash info_hash = types::InfoHash::random();
    
    // Test store_infohash
    EXPECT_TRUE(persistence.store_infohash(info_hash));
    
    // Test get_infohashes
    auto result = persistence.get_infohashes();
    EXPECT_EQ(1UL, result.size());
    EXPECT_EQ(info_hash, result[0]);
    
    // Clean up
    EXPECT_TRUE(persistence.close());
}

TEST_F(PersistenceTest, StoreGetInfohashAsyncTest) {
    Persistence persistence(db_path_);
    EXPECT_TRUE(persistence.initialize());
    
    // Create an infohash
    types::InfoHash info_hash = types::InfoHash::random();
    
    // Test store_infohash_async
    auto store_future = persistence.store_infohash_async(info_hash);
    EXPECT_TRUE(store_future.get());
    
    // Test get_infohashes_async
    auto get_future = persistence.get_infohashes_async();
    auto result = get_future.get();
    EXPECT_EQ(1UL, result.size());
    EXPECT_EQ(info_hash, result[0]);
    
    // Clean up
    EXPECT_TRUE(persistence.close());
}

TEST_F(PersistenceTest, GetInfohashesTest) {
    Persistence persistence(db_path_);
    EXPECT_TRUE(persistence.initialize());
    
    // Create some infohashes
    std::vector<types::InfoHash> infohashes;
    for (int i = 0; i < 10; ++i) {
        types::InfoHash info_hash = types::InfoHash::random();
        infohashes.push_back(info_hash);
        EXPECT_TRUE(persistence.store_infohash(info_hash));
    }
    
    // Test get_infohashes with default parameters
    auto result = persistence.get_infohashes();
    EXPECT_EQ(10UL, result.size());
    
    // Test get_infohashes with limit
    auto limited_result = persistence.get_infohashes(5);
    EXPECT_EQ(5UL, limited_result.size());
    
    // Test get_infohashes with limit and offset
    auto offset_result = persistence.get_infohashes(5, 5);
    EXPECT_EQ(5UL, offset_result.size());
    
    // Clean up
    EXPECT_TRUE(persistence.close());
}

TEST_F(PersistenceTest, GetStatisticsTest) {
    Persistence persistence(db_path_);
    EXPECT_TRUE(persistence.initialize());
    
    // Create some nodes and infohashes
    for (int i = 0; i < 5; ++i) {
        types::NodeID node_id = types::NodeID::random();
        types::Endpoint endpoint("192.168.1." + std::to_string(i + 1), 6881 + i);
        EXPECT_TRUE(persistence.store_node(node_id, endpoint));
        
        types::InfoHash info_hash = types::InfoHash::random();
        EXPECT_TRUE(persistence.store_infohash(info_hash));
    }
    
    // Test get_statistics
    auto stats = persistence.get_statistics();
    
    EXPECT_FALSE(stats.empty());
    EXPECT_EQ("true", stats["persistence.initialized"]);
    EXPECT_EQ(db_path_, stats["persistence.db_path"]);
    EXPECT_EQ("5", stats["persistence.node_count"]);
    EXPECT_EQ("5", stats["persistence.infohash_count"]);
    
    // Clean up
    EXPECT_TRUE(persistence.close());
}

} // namespace bitscrape::core::test
