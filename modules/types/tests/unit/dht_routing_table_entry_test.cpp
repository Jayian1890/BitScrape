#include <bitscrape/testing.hpp>

#include "bitscrape/types/dht_routing_table_entry.hpp"

using namespace bitscrape::types;

TEST(DHTRoutingTableEntryTest, Construction) {
  DHTRoutingTableEntry entry(0);

  EXPECT_EQ(entry.prefix_length(), 0);
  EXPECT_TRUE(entry.is_empty());
  EXPECT_FALSE(entry.is_full());
  EXPECT_EQ(entry.size(), 0UL);
}

TEST(DHTRoutingTableEntryTest, AddNode) {
  DHTRoutingTableEntry entry(0);

  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);

  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);

  EXPECT_TRUE(entry.add_node(node1));

  EXPECT_EQ(entry.size(), 1UL);
  EXPECT_FALSE(entry.is_empty());
  EXPECT_FALSE(entry.is_full());

  // Adding the same node again should fail
  EXPECT_FALSE(entry.add_node(node1));

  // Adding a different node should succeed
  EXPECT_TRUE(entry.add_node(node2));

  EXPECT_EQ(entry.size(), 2);
}

TEST(DHTRoutingTableEntryTest, RemoveNode) {
  DHTRoutingTableEntry entry(0);

  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);

  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);

  EXPECT_TRUE(entry.add_node(node1));
  EXPECT_TRUE(entry.add_node(node2));

  EXPECT_EQ(entry.size(), 2UL);

  // Removing a node that exists should succeed
  EXPECT_TRUE(entry.remove_node(node1));

  EXPECT_EQ(entry.size(), 1UL);

  // Removing a node that doesn't exist should fail
  EXPECT_FALSE(entry.remove_node(node1));

  // Removing the last node should succeed
  EXPECT_TRUE(entry.remove_node(node2));

  EXPECT_TRUE(entry.is_empty());
}

TEST(DHTRoutingTableEntryTest, UpdateNode) {
  DHTRoutingTableEntry entry(0);

  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);

  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);

  EXPECT_TRUE(entry.add_node(node1));

  // Create a new node with the same ID but different status
  DHTNode updated_node(id1, ep1, DHTNode::Status::GOOD);

  // Updating a node that exists should succeed
  EXPECT_TRUE(entry.update_node(updated_node));

  // Check that the node was updated
  const DHTNode *node = entry.get_node(id1);
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(node->status(), DHTNode::Status::GOOD);

  // Updating a node that doesn't exist should fail
  EXPECT_FALSE(entry.update_node(node2));
}

TEST(DHTRoutingTableEntryTest, ContainsNode) {
  DHTRoutingTableEntry entry(0);

  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);

  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);

  EXPECT_TRUE(entry.add_node(node1));

  EXPECT_TRUE(entry.contains_node(node1));
  EXPECT_FALSE(entry.contains_node(node2));
}

TEST(DHTRoutingTableEntryTest, ContainsNodeID) {
  DHTRoutingTableEntry entry(0);

  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);

  EXPECT_TRUE(entry.add_node(node1));

  EXPECT_TRUE(entry.contains_node_id(id1));
  EXPECT_FALSE(entry.contains_node_id(id2));
}

TEST(DHTRoutingTableEntryTest, GetNode) {
  DHTRoutingTableEntry entry(0);

  NodeID id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);

  EXPECT_TRUE(entry.add_node(node1));

  const DHTNode *node = entry.get_node(id1);
  EXPECT_NE(node, nullptr);
  EXPECT_EQ(*node, node1);

  const DHTNode *missing_node = entry.get_node(id2);
  EXPECT_EQ(missing_node, nullptr);
}

TEST(DHTRoutingTableEntryTest, IsFull) {
  DHTRoutingTableEntry entry(0);

  // Add K nodes to fill the bucket
  for (size_t i = 0; i < DHTRoutingTableEntry::K; ++i) {
    NodeID id; // Random ID
    Endpoint ep(std::string("192.168.1.1"), 6881 + i);
    DHTNode node(id, ep);

    EXPECT_TRUE(entry.add_node(node));
  }

  EXPECT_TRUE(entry.is_full());

  // Adding another node should fail
  NodeID id; // Random ID
  Endpoint ep(std::string("192.168.1.1"), 7000);
  DHTNode node(id, ep);

  EXPECT_FALSE(entry.add_node(node));
}

TEST(DHTRoutingTableEntryTest, ToString) {
  DHTRoutingTableEntry entry(0);

  std::string str = entry.to_string();

  // Check that the string contains the prefix length and node count
  EXPECT_NE(str.find("DHTRoutingTableEntry"), std::string::npos);
  EXPECT_NE(str.find("prefix_length=0"), std::string::npos);
  EXPECT_NE(str.find("nodes=0"), std::string::npos);
}
