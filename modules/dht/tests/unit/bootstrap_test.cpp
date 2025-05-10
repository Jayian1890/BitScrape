#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "bitscrape/dht/bootstrap.hpp"

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
    
    MOCK_METHOD(bool, add_node, (const DHTNode& node), (override));
    MOCK_METHOD(std::vector<DHTNode>, get_closest_nodes, (const NodeID& target_id, size_t k), (const, override));
    MOCK_METHOD(size_t, size, (), (const, override));
};

// Mock message factory for testing
class MockMessageFactory : public DHTMessageFactory {
public:
    MOCK_METHOD(std::shared_ptr<DHTMessage>, create_ping, (const std::string& transaction_id, const NodeID& node_id), ());
    MOCK_METHOD(std::shared_ptr<DHTMessage>, create_find_node, (const std::string& transaction_id, const NodeID& node_id, const NodeID& target_id), ());
};

// Mock node lookup for testing
class MockNodeLookup : public NodeLookup {
public:
    MockNodeLookup(const NodeID& local_id,
                  const NodeID& target_id,
                  const RoutingTable& routing_table,
                  UDPSocket& socket,
                  DHTMessageFactory& message_factory)
        : NodeLookup(local_id, target_id, routing_table, socket, message_factory) {}
    
    MOCK_METHOD(std::vector<DHTNode>, start, (), (override));
    MOCK_METHOD(std::future<std::vector<DHTNode>>, start_async, (), (override));
    MOCK_METHOD(bool, is_complete, (), (const, override));
    MOCK_METHOD(bool, wait_for_completion, (int), (override));
};

TEST(BootstrapTest, Constructor) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    Bootstrap bootstrap(local_id, routing_table, socket, message_factory);
    
    EXPECT_FALSE(bootstrap.is_complete());
}

TEST(BootstrapTest, StartWithNoNodes) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Routing table is empty
    EXPECT_CALL(routing_table, size())
        .WillOnce(Return(0));
    
    Bootstrap bootstrap(local_id, routing_table, socket, message_factory);
    
    std::vector<Endpoint> bootstrap_nodes;
    
    bool result = bootstrap.start(bootstrap_nodes);
    
    EXPECT_TRUE(bootstrap.is_complete());
    EXPECT_FALSE(result);
}

TEST(BootstrapTest, StartWithBootstrapNodes) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Create some test bootstrap nodes
    std::vector<Endpoint> bootstrap_nodes;
    for (int i = 0; i < 3; ++i) {
        Endpoint ep(std::string("192.168.1.1"), 6881 + i);
        bootstrap_nodes.push_back(ep);
    }
    
    // Message factory creates ping messages
    auto ping_message = std::make_shared<DHTMessage>(DHTMessage::Type::PING, "aa");
    EXPECT_CALL(message_factory, create_ping(_, local_id))
        .Times(3)
        .WillRepeatedly(Return(ping_message));
    
    // Socket sends messages
    EXPECT_CALL(socket, send_to(_, _))
        .Times(3);
    
    // Routing table has nodes after bootstrap
    EXPECT_CALL(routing_table, size())
        .WillOnce(Return(3));
    
    Bootstrap bootstrap(local_id, routing_table, socket, message_factory);
    
    // Start the bootstrap process
    bool result = bootstrap.start(bootstrap_nodes);
    
    EXPECT_TRUE(bootstrap.is_complete());
    EXPECT_TRUE(result);
}

TEST(BootstrapTest, StartAsync) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Create some test bootstrap nodes
    std::vector<Endpoint> bootstrap_nodes;
    for (int i = 0; i < 3; ++i) {
        Endpoint ep(std::string("192.168.1.1"), 6881 + i);
        bootstrap_nodes.push_back(ep);
    }
    
    // Message factory creates ping messages
    auto ping_message = std::make_shared<DHTMessage>(DHTMessage::Type::PING, "aa");
    EXPECT_CALL(message_factory, create_ping(_, local_id))
        .Times(3)
        .WillRepeatedly(Return(ping_message));
    
    // Socket sends messages
    EXPECT_CALL(socket, send_to(_, _))
        .Times(3);
    
    // Routing table has nodes after bootstrap
    EXPECT_CALL(routing_table, size())
        .WillOnce(Return(3));
    
    Bootstrap bootstrap(local_id, routing_table, socket, message_factory);
    
    // Start the bootstrap process asynchronously
    auto future = bootstrap.start_async(bootstrap_nodes);
    
    // Wait for the result
    bool result = future.get();
    
    EXPECT_TRUE(bootstrap.is_complete());
    EXPECT_TRUE(result);
}

TEST(BootstrapTest, ProcessMessage) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Create a test bootstrap node
    Endpoint ep(std::string("192.168.1.1"), 6881);
    
    // Message factory creates a ping message
    auto ping_message = std::make_shared<DHTMessage>(DHTMessage::Type::PING, "aa");
    EXPECT_CALL(message_factory, create_ping(_, local_id))
        .WillOnce(Return(ping_message));
    
    // Socket sends a message
    EXPECT_CALL(socket, send_to(_, _))
        .Times(1);
    
    Bootstrap bootstrap(local_id, routing_table, socket, message_factory);
    
    // Start the bootstrap process in a separate thread
    std::thread t([&bootstrap, &ep]() {
        std::vector<Endpoint> bootstrap_nodes = {ep};
        bootstrap.start(bootstrap_nodes);
    });
    
    // Process a ping response
    auto response = std::make_shared<DHTMessage>(DHTMessage::Type::PING_RESPONSE, "aa");
    bootstrap.process_message(response, ep);
    
    // Wait for the bootstrap process to complete
    EXPECT_TRUE(bootstrap.wait_for_completion(100));
    
    t.join();
}

TEST(BootstrapTest, WaitForCompletion) {
    NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
    MockRoutingTable routing_table;
    MockUDPSocket socket;
    MockMessageFactory message_factory;
    
    // Routing table is empty
    EXPECT_CALL(routing_table, size())
        .WillOnce(Return(0));
    
    Bootstrap bootstrap(local_id, routing_table, socket, message_factory);
    
    // Start the bootstrap process in a separate thread
    std::thread t([&bootstrap]() {
        std::vector<Endpoint> bootstrap_nodes;
        bootstrap.start(bootstrap_nodes);
    });
    
    // Wait for the bootstrap process to complete
    EXPECT_TRUE(bootstrap.wait_for_completion(100));
    
    t.join();
}
