#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "bitscrape/dht/dht_session.hpp"
#include "bitscrape/lock/lock_manager_singleton.hpp"

using namespace bitscrape::dht;
using namespace bitscrape::types;
using namespace bitscrape::network;
using namespace bitscrape::event;
using namespace bitscrape::lock;
using namespace testing;

// Mock UDP socket for testing
class MockUDPSocket : public UDPSocket {
public:
    MockUDPSocket() : UDPSocket() {}

    MOCK_METHOD(bool, bind, (uint16_t port), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(void, send_to, (const std::vector<uint8_t>& data, const Endpoint& endpoint), (override));
    MOCK_METHOD(std::pair<std::vector<uint8_t>, Endpoint>, receive_from, (), (override));
    MOCK_METHOD(std::future<std::pair<std::vector<uint8_t>, Endpoint>>, receive_from_async, (), (override));
};

// Mock event bus for testing
class MockEventBus : public EventBus {
public:
    MOCK_METHOD(void, publish, (const Event& event), (override));
    MOCK_METHOD(std::future<void>, publish_async, (const Event& event), (override));
    MOCK_METHOD(void, subscribe, (const std::string& event_type, const EventCallback& callback), (override));
    MOCK_METHOD(void, unsubscribe, (const std::string& event_type, const EventCallback& callback), (override));
};

TEST(DHTSessionTest, Constructor) {
  auto lock_manager = LockManagerSingleton::instance();
  DHTSession session(lock_manager);

  EXPECT_FALSE(session.is_running());
}

TEST(DHTSessionTest, ConstructorWithNodeId) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();

  DHTSession session(node_id, lock_manager);

  EXPECT_EQ(session.node_id(), node_id);
  EXPECT_FALSE(session.is_running());
}

TEST(DHTSessionTest, ConstructorWithNodeIdAndPort) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  MockEventBus event_bus;
  auto lock_manager = LockManagerSingleton::instance();

  DHTSession session(node_id, 6881, event_bus, lock_manager);

  EXPECT_EQ(session.node_id(), node_id);
  EXPECT_FALSE(session.is_running());
}

TEST(DHTSessionTest, StartAndStop) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));

  // Create a session with a mock socket
  auto socket = std::make_unique<MockUDPSocket>();
  auto socket_ptr = socket.get();

  // Expect the socket to be bound and closed
  EXPECT_CALL(*socket_ptr, bind(6881))
      .WillOnce(Return(true));
  EXPECT_CALL(*socket_ptr, close())
      .Times(1);

  // Expect the socket to receive messages
  EXPECT_CALL(*socket_ptr, receive_from())
      .WillRepeatedly(Throw(std::runtime_error("Socket closed")));

  // Create the session
  auto lock_manager = LockManagerSingleton::instance();
  DHTSession session(node_id, lock_manager);

  // Start the session
  std::vector<Endpoint> bootstrap_nodes;
  EXPECT_TRUE(session.start(bootstrap_nodes));
  EXPECT_TRUE(session.is_running());

  // Stop the session
  session.stop();
  EXPECT_FALSE(session.is_running());
}

TEST(DHTSessionTest, StartAsync) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));

  // Create a session with a mock socket
  auto socket = std::make_unique<MockUDPSocket>();
  auto socket_ptr = socket.get();

  // Expect the socket to be bound and closed
  EXPECT_CALL(*socket_ptr, bind(6881))
      .WillOnce(Return(true));
  EXPECT_CALL(*socket_ptr, close())
      .Times(1);

  // Expect the socket to receive messages
  EXPECT_CALL(*socket_ptr, receive_from())
      .WillRepeatedly(Throw(std::runtime_error("Socket closed")));

  // Create the session
  auto lock_manager = LockManagerSingleton::instance();
  DHTSession session(node_id, lock_manager);

  // Start the session asynchronously
  std::vector<Endpoint> bootstrap_nodes;
  auto future = session.start_async(bootstrap_nodes);

  // Wait for the result
  EXPECT_TRUE(future.get());
  EXPECT_TRUE(session.is_running());

  // Stop the session
  session.stop();
  EXPECT_FALSE(session.is_running());
}

TEST(DHTSessionTest, FindNodes) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID target_id(std::string("1112131415161718191a1b1c1d1e1f2021222324"));

  // Create a session with a mock socket
  auto socket = std::make_unique<MockUDPSocket>();
  auto socket_ptr = socket.get();

  // Expect the socket to be bound and closed
  EXPECT_CALL(*socket_ptr, bind(6881))
      .WillOnce(Return(true));
  EXPECT_CALL(*socket_ptr, close())
      .Times(1);

  // Expect the socket to receive messages
  EXPECT_CALL(*socket_ptr, receive_from())
      .WillRepeatedly(Throw(std::runtime_error("Socket closed")));

  // Create the session
  auto lock_manager = LockManagerSingleton::instance();
  DHTSession session(node_id, lock_manager);

  // Start the session
  std::vector<Endpoint> bootstrap_nodes;
  EXPECT_TRUE(session.start(bootstrap_nodes));

  // Find nodes
  auto nodes = session.find_nodes(target_id);

  // Stop the session
  session.stop();
}

TEST(DHTSessionTest, FindNodesAsync) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID target_id(std::string("1112131415161718191a1b1c1d1e1f2021222324"));

  // Create a session with a mock socket
  auto socket = std::make_unique<MockUDPSocket>();
  auto socket_ptr = socket.get();

  // Expect the socket to be bound and closed
  EXPECT_CALL(*socket_ptr, bind(6881))
      .WillOnce(Return(true));
  EXPECT_CALL(*socket_ptr, close())
      .Times(1);

  // Expect the socket to receive messages
  EXPECT_CALL(*socket_ptr, receive_from())
      .WillRepeatedly(Throw(std::runtime_error("Socket closed")));

  // Create the session
  auto lock_manager = LockManagerSingleton::instance();
  DHTSession session(node_id, lock_manager);

  // Start the session
  std::vector<Endpoint> bootstrap_nodes;
  EXPECT_TRUE(session.start(bootstrap_nodes));

  // Find nodes asynchronously
  auto future = session.find_nodes_async(target_id);

  // Wait for the result
  auto nodes = future.get();

  // Stop the session
  session.stop();
}

TEST(DHTSessionTest, FindPeers) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  InfoHash infohash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));

  // Create a session with a mock socket
  auto socket = std::make_unique<MockUDPSocket>();
  auto socket_ptr = socket.get();

  // Expect the socket to be bound and closed
  EXPECT_CALL(*socket_ptr, bind(6881))
      .WillOnce(Return(true));
  EXPECT_CALL(*socket_ptr, close())
      .Times(1);

  // Expect the socket to receive messages
  EXPECT_CALL(*socket_ptr, receive_from())
      .WillRepeatedly(Throw(std::runtime_error("Socket closed")));

  // Create the session
  auto lock_manager = LockManagerSingleton::instance();
  DHTSession session(node_id, lock_manager);

  // Start the session
  std::vector<Endpoint> bootstrap_nodes;
  EXPECT_TRUE(session.start(bootstrap_nodes));

  // Find peers
  auto peers = session.find_peers(infohash);

  // Stop the session
  session.stop();
}

TEST(DHTSessionTest, AnnouncePeer) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  InfoHash infohash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));

  // Create a session with a mock socket
  auto socket = std::make_unique<MockUDPSocket>();
  auto socket_ptr = socket.get();

  // Expect the socket to be bound and closed
  EXPECT_CALL(*socket_ptr, bind(6881))
      .WillOnce(Return(true));
  EXPECT_CALL(*socket_ptr, close())
      .Times(1);

  // Expect the socket to receive messages
  EXPECT_CALL(*socket_ptr, receive_from())
      .WillRepeatedly(Throw(std::runtime_error("Socket closed")));

  // Create the session
  auto lock_manager = LockManagerSingleton::instance();
  DHTSession session(node_id, lock_manager);

  // Start the session
  std::vector<Endpoint> bootstrap_nodes;
  EXPECT_TRUE(session.start(bootstrap_nodes));

  // Announce peer
  EXPECT_TRUE(session.announce_peer(infohash, 6881));

  // Stop the session
  session.stop();
}
