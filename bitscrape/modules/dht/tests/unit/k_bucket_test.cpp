#include <gtest/gtest.h>

#include "bitscrape/dht/k_bucket.hpp"

using namespace bitscrape::dht;
using namespace bitscrape::types;

TEST(KBucketTest, Constructor) {
  KBucket bucket(0);
  
  EXPECT_EQ(bucket.prefix_length(), 0);
  EXPECT_EQ(bucket.size(), 0);
  EXPECT_TRUE(bucket.is_empty());
  EXPECT_FALSE(bucket.is_full());
}

TEST(KBucketTest, AddNode) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);
  
  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);
  
  EXPECT_TRUE(bucket.add_node(node1));
  EXPECT_EQ(bucket.size(), 1);
  EXPECT_FALSE(bucket.is_empty());
  
  EXPECT_TRUE(bucket.add_node(node2));
  EXPECT_EQ(bucket.size(), 2);
}

TEST(KBucketTest, AddNodeAsync) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  
  DHTNode node1(id1, ep1);
  
  auto future = bucket.add_node_async(node1);
  EXPECT_TRUE(future.get());
  EXPECT_EQ(bucket.size(), 1);
}

TEST(KBucketTest, AddDuplicateNode) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  
  DHTNode node1(id1, ep1);
  
  EXPECT_TRUE(bucket.add_node(node1));
  EXPECT_FALSE(bucket.add_node(node1));
  EXPECT_EQ(bucket.size(), 1);
}

TEST(KBucketTest, AddNodeToFullBucket) {
  KBucket bucket(0);
  
  // Add K nodes to fill the bucket
  for (size_t i = 0; i < KBucket::K; ++i) {
    NodeID id(std::to_string(i));
    Endpoint ep(std::string("192.168.1.1"), 6881 + i);
    
    DHTNode node(id, ep);
    EXPECT_TRUE(bucket.add_node(node));
  }
  
  EXPECT_EQ(bucket.size(), KBucket::K);
  EXPECT_TRUE(bucket.is_full());
  
  // Try to add one more node
  NodeID id(std::string("extra"));
  Endpoint ep(std::string("192.168.1.1"), 7000);
  
  DHTNode node(id, ep);
  EXPECT_FALSE(bucket.add_node(node));
  EXPECT_EQ(bucket.size(), KBucket::K);
}

TEST(KBucketTest, RemoveNode) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);
  
  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);
  
  bucket.add_node(node1);
  bucket.add_node(node2);
  
  EXPECT_EQ(bucket.size(), 2);
  
  EXPECT_TRUE(bucket.remove_node(id1));
  EXPECT_EQ(bucket.size(), 1);
  
  EXPECT_FALSE(bucket.remove_node(id1));
  EXPECT_EQ(bucket.size(), 1);
  
  EXPECT_TRUE(bucket.remove_node(id2));
  EXPECT_EQ(bucket.size(), 0);
  EXPECT_TRUE(bucket.is_empty());
}

TEST(KBucketTest, RemoveNodeAsync) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  
  DHTNode node1(id1, ep1);
  
  bucket.add_node(node1);
  
  auto future = bucket.remove_node_async(id1);
  EXPECT_TRUE(future.get());
  EXPECT_EQ(bucket.size(), 0);
}

TEST(KBucketTest, UpdateNode) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  
  DHTNode node1(id1, ep1);
  bucket.add_node(node1);
  
  DHTNode node2(id1, ep1, DHTNode::Status::GOOD);
  EXPECT_TRUE(bucket.update_node(node2));
  
  auto node = bucket.get_node(id1);
  EXPECT_TRUE(node.has_value());
  EXPECT_EQ(node->status(), DHTNode::Status::GOOD);
}

TEST(KBucketTest, UpdateNodeAsync) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  
  DHTNode node1(id1, ep1);
  bucket.add_node(node1);
  
  DHTNode node2(id1, ep1, DHTNode::Status::GOOD);
  auto future = bucket.update_node_async(node2);
  EXPECT_TRUE(future.get());
  
  auto node = bucket.get_node(id1);
  EXPECT_TRUE(node.has_value());
  EXPECT_EQ(node->status(), DHTNode::Status::GOOD);
}

TEST(KBucketTest, GetNode) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  
  DHTNode node1(id1, ep1);
  bucket.add_node(node1);
  
  auto node = bucket.get_node(id1);
  EXPECT_TRUE(node.has_value());
  EXPECT_EQ(node->id(), id1);
  
  auto node2 = bucket.get_node(id2);
  EXPECT_FALSE(node2.has_value());
}

TEST(KBucketTest, GetNodeAsync) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  
  DHTNode node1(id1, ep1);
  bucket.add_node(node1);
  
  auto future = bucket.get_node_async(id1);
  auto node = future.get();
  EXPECT_TRUE(node.has_value());
  EXPECT_EQ(node->id(), id1);
}

TEST(KBucketTest, ContainsNode) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  
  DHTNode node1(id1, ep1);
  bucket.add_node(node1);
  
  EXPECT_TRUE(bucket.contains_node(id1));
  EXPECT_FALSE(bucket.contains_node(id2));
}

TEST(KBucketTest, ContainsNodeAsync) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  
  DHTNode node1(id1, ep1);
  bucket.add_node(node1);
  
  auto future1 = bucket.contains_node_async(id1);
  EXPECT_TRUE(future1.get());
  
  auto future2 = bucket.contains_node_async(id2);
  EXPECT_FALSE(future2.get());
}

TEST(KBucketTest, GetNodes) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);
  
  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);
  
  bucket.add_node(node1);
  bucket.add_node(node2);
  
  auto nodes = bucket.get_nodes();
  EXPECT_EQ(nodes.size(), 2);
  
  // Check that both nodes are in the result
  bool found1 = false;
  bool found2 = false;
  for (const auto& node : nodes) {
    if (node.id() == id1) {
      found1 = true;
    } else if (node.id() == id2) {
      found2 = true;
    }
  }
  
  EXPECT_TRUE(found1);
  EXPECT_TRUE(found2);
}

TEST(KBucketTest, GetNodesAsync) {
  KBucket bucket(0);
  
  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);
  
  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);
  
  bucket.add_node(node1);
  bucket.add_node(node2);
  
  auto future = bucket.get_nodes_async();
  auto nodes = future.get();
  EXPECT_EQ(nodes.size(), 2);
}

TEST(KBucketTest, UpdateLastUpdated) {
  KBucket bucket(0);
  
  auto before = bucket.last_updated();
  
  // Sleep for a short time to ensure the time changes
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  bucket.update_last_updated();
  
  auto after = bucket.last_updated();
  
  EXPECT_GT(after, before);
}
