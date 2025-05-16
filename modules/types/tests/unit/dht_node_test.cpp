#include <gtest/gtest.h>
#include <thread>

#include "bitscrape/types/dht_node.hpp"

using namespace bitscrape::types;

TEST(DHTNodeTest, DefaultConstructor) {
  DHTNode node;

  EXPECT_FALSE(node.is_valid());
  EXPECT_EQ(node.status(), DHTNode::Status::UNKNOWN);
}

TEST(DHTNodeTest, ConstructionWithIDAndEndpoint) {
  NodeID id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep(std::string("192.168.1.1"), 6881);

  DHTNode node(id, ep);

  EXPECT_EQ(node.id(), id);
  EXPECT_EQ(node.endpoint(), ep);
  EXPECT_EQ(node.status(), DHTNode::Status::UNKNOWN);
  EXPECT_TRUE(node.is_valid());
}

TEST(DHTNodeTest, ConstructionWithIDEndpointAndStatus) {
  NodeID id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep(std::string("192.168.1.1"), 6881);

  DHTNode node(id, ep, DHTNode::Status::GOOD);

  EXPECT_EQ(node.id(), id);
  EXPECT_EQ(node.endpoint(), ep);
  EXPECT_EQ(node.status(), DHTNode::Status::GOOD);
  EXPECT_TRUE(node.is_valid());
}

TEST(DHTNodeTest, SetStatus) {
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep(std::string("192.168.1.1"), 6881);
  DHTNode node(id1, ep);

  node.set_status(DHTNode::Status::GOOD);
  EXPECT_EQ(node.status(), DHTNode::Status::GOOD);

  node.set_status(DHTNode::Status::BAD);
  EXPECT_EQ(node.status(), DHTNode::Status::BAD);
}

TEST(DHTNodeTest, UpdateLastSeen) {
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep(std::string("192.168.1.1"), 6881);
  DHTNode node(id1, ep);

  auto before = node.last_seen();
  // Sleep for a short time to ensure the time changes
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  node.update_last_seen();
  EXPECT_GT(node.last_seen(), before);
}

TEST(DHTNodeTest, Distance) {
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep(std::string("192.168.1.1"), 6881);

  DHTNode node(id1, ep);
  DHTNode other(id2, ep);

  NodeID dist = node.distance(other);
  NodeID expected = id1.distance(id2);
  EXPECT_EQ(dist, expected);
}

TEST(DHTNodeTest, DistanceAsync) {
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep(std::string("192.168.1.1"), 6881);

  DHTNode node(id1, ep);
  DHTNode other(id2, ep);

  auto future = node.distance_async(other);
  NodeID dist = future.get();
  NodeID expected = id1.distance(id2);
  EXPECT_EQ(dist, expected);
}

TEST(DHTNodeTest, DistanceToNodeID) {
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep(std::string("192.168.1.1"), 6881);

  DHTNode node(id1, ep);

  NodeID dist = node.distance(id2);
  NodeID expected = id1.distance(id2);
  EXPECT_EQ(dist, expected);
}

TEST(DHTNodeTest, DistanceAsyncToNodeID) {
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep(std::string("192.168.1.1"), 6881);

  DHTNode node(id1, ep);

  auto future = node.distance_async(id2);
  NodeID dist = future.get();
  NodeID expected = id1.distance(id2);
  EXPECT_EQ(dist, expected);
}

TEST(DHTNodeTest, ToString) {
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep(std::string("192.168.1.1"), 6881);

  DHTNode node(id1, ep);
  std::string str = node.to_string();

  // Check that the string contains the node ID and endpoint
  EXPECT_NE(str.find("DHTNode"), std::string::npos);
  EXPECT_NE(str.find(id1.to_hex().substr(0, 8)), std::string::npos);
  EXPECT_NE(str.find(ep.to_string()), std::string::npos);
  EXPECT_NE(str.find("UNKNOWN"), std::string::npos);
}

TEST(DHTNodeTest, ComparisonOperators) {
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep(std::string("192.168.1.1"), 6881);

  DHTNode node(id1, ep);
  DHTNode same(id1, ep);
  DHTNode different_id(id2, ep);
  DHTNode different_ep(id1, Endpoint(std::string("192.168.1.2"), 6881));

  EXPECT_EQ(node, same);
  EXPECT_NE(node, different_id);
  EXPECT_NE(node, different_ep);
}
