#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "bitscrape/dht/node_lookup.hpp"

using namespace bitscrape::dht;
using namespace bitscrape::types;
using namespace bitscrape::network;
using namespace testing;

// Mock UDP socket for testing
class MockUDPSocket : public UDPSocket {
public:
    MockUDPSocket() : UDPSocket() {}
    
    MOCK_METHOD(void, send_to, (const std::vector<uint8_t>& data, const Endpoint& endpoint), (override));
    MOCK_METHOD(std::pair<std::vector<uint8_t>, Endpoint>, receive_from, (), (override));
    MOCK_METHOD(std::future<std::pair<std::vector<uint8_t>, Endpoint>>, receive_from_async, (), (override));
};

// Mock routing table for testing
class MockRoutingTable : public RoutingTable {
public:
    MockRoutingTable() : RoutingTable(NodeID()) {}
    
    MOCK_METHOD(std::vector<DHTNode>, get_closest_nodes, (const NodeID& target_id, size_t k), (const, override));
};

// Mock message factory for testing
class MockMessageFactory : public DHTMessageFactory {
public:
    MOCK_METHOD(std::shared_ptr<DHTMessage>, create_find_node, (const std::string& transaction_id, const NodeID& node_id, const NodeID& target_id), ());
};

TEST(NodeLookupTest, Constructor) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    NodeID target_id(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    NodeLookup lookup(local_id, target_id, routing_table, socket, message_factory);
    
    EXPECT_FALSE(lookup.is_complete());
}

TEST(NodeLookupTest, StartWithNoNodes) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    NodeID target_id(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Routing table returns no nodes
    EXPECT_CALL(routing_table, get_closest_nodes(target_id, NodeLookup::K))
        .WillOnce(Return(std::vector<DHTNode>()));
    
    NodeLookup lookup(local_id, target_id, routing_table, socket, message_factory);
    
    auto nodes = lookup.start();
    
    EXPECT_TRUE(lookup.is_complete());
    EXPECT_TRUE(nodes.empty());
}

TEST(NodeLookupTest, StartWithNodes) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    NodeID target_id(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Create some test nodes
    std::vector<DHTNode> test_nodes;
    for (int i = 0; i < 3; ++i) {
        NodeID id(std::to_string(i));
        Endpoint ep(std::string("192.168.1.1"), 6881 + i);
        test_nodes.emplace_back(id, ep);
    }
    
    // Routing table returns the test nodes
    EXPECT_CALL(routing_table, get_closest_nodes(target_id, NodeLookup::K))
        .WillOnce(Return(test_nodes));
    
    // Message factory creates find_node messages
    auto find_node_message = std::make_shared<DHTMessage>(DHTMessage::Type::FIND_NODE, "aa");
    EXPECT_CALL(message_factory, create_find_node(_, local_id, target_id))
        .Times(3)
        .WillRepeatedly(Return(find_node_message));
    
    // Socket sends messages
    EXPECT_CALL(socket, send_to(_, _))
        .Times(3);
    
    NodeLookup lookup(local_id, target_id, routing_table, socket, message_factory);
    
    // Start the lookup asynchronously
    auto future = lookup.start_async();
    
    // Process responses for all nodes
    for (const auto& node : test_nodes) {
        auto response = std::make_shared<DHTMessage>(DHTMessage::Type::FIND_NODE_RESPONSE, "aa");
        lookup.process_response(response, node.endpoint());
    }
    
    // Wait for the lookup to complete
    auto nodes = future.get();
    
    EXPECT_TRUE(lookup.is_complete());
    EXPECT_EQ(nodes.size(), 3);
}

TEST(NodeLookupTest, WaitForCompletion) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    NodeID target_id(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Routing table returns no nodes
    EXPECT_CALL(routing_table, get_closest_nodes(target_id, NodeLookup::K))
        .WillOnce(Return(std::vector<DHTNode>()));
    
    NodeLookup lookup(local_id, target_id, routing_table, socket, message_factory);
    
    // Start the lookup in a separate thread
    std::thread t([&lookup]() {
        lookup.start();
    });
    
    // Wait for the lookup to complete
    EXPECT_TRUE(lookup.wait_for_completion(100));
    
    t.join();
}

TEST(NodeLookupTest, ProcessResponse) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    NodeID target_id(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Create a test node
    NodeID id(std::string("0"));
    Endpoint ep(std::string("192.168.1.1"), 6881);
    DHTNode node(id, ep);
    
    // Routing table returns the test node
    EXPECT_CALL(routing_table, get_closest_nodes(target_id, NodeLookup::K))
        .WillOnce(Return(std::vector<DHTNode>{node}));
    
    // Message factory creates a find_node message
    auto find_node_message = std::make_shared<DHTMessage>(DHTMessage::Type::FIND_NODE, "aa");
    EXPECT_CALL(message_factory, create_find_node(_, local_id, target_id))
        .WillOnce(Return(find_node_message));
    
    // Socket sends a message
    EXPECT_CALL(socket, send_to(_, _))
        .Times(1);
    
    NodeLookup lookup(local_id, target_id, routing_table, socket, message_factory);
    
    // Start the lookup in a separate thread
    std::thread t([&lookup]() {
        lookup.start();
    });
    
    // Process a response
    auto response = std::make_shared<DHTMessage>(DHTMessage::Type::FIND_NODE_RESPONSE, "aa");
    lookup.process_response(response, ep);
    
    // Wait for the lookup to complete
    EXPECT_TRUE(lookup.wait_for_completion(100));
    
    t.join();
}

TEST(NodeLookupTest, GetClosestNodes) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    NodeID target_id(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Create some test nodes with different distances to the target
    std::vector<DHTNode> test_nodes;
    for (int i = 0; i < 10; ++i) {
        NodeID id = target_id;
        // Flip some bits to create a node at a specific distance
        for (int j = 0; j < i; ++j) {
            id.flip_bit(j);
        }
        
        Endpoint ep(std::string("192.168.1.1"), 6881 + i);
        test_nodes.emplace_back(id, ep);
    }
    
    // Routing table returns the test nodes
    EXPECT_CALL(routing_table, get_closest_nodes(target_id, NodeLookup::K))
        .WillOnce(Return(test_nodes));
    
    // Message factory creates find_node messages
    auto find_node_message = std::make_shared<DHTMessage>(DHTMessage::Type::FIND_NODE, "aa");
    EXPECT_CALL(message_factory, create_find_node(_, local_id, target_id))
        .Times(3)  // ALPHA = 3
        .WillRepeatedly(Return(find_node_message));
    
    // Socket sends messages
    EXPECT_CALL(socket, send_to(_, _))
        .Times(3);
    
    NodeLookup lookup(local_id, target_id, routing_table, socket, message_factory);
    
    // Start the lookup
    auto nodes = lookup.start();
    
    // Check that the nodes are sorted by distance
    for (size_t i = 1; i < nodes.size(); ++i) {
        NodeID dist1 = nodes[i-1].id().distance(target_id);
        NodeID dist2 = nodes[i].id().distance(target_id);
        
        EXPECT_LE(dist1, dist2);
    }
}
