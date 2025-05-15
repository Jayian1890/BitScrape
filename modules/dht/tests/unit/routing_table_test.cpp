#include <gtest/gtest.h>

#include "bitscrape/dht/routing_table.hpp"
#include "bitscrape/lock/lock_manager_singleton.hpp"

using namespace bitscrape::dht;
using namespace bitscrape::types;
using namespace bitscrape::lock;

TEST(RoutingTableTest, Constructor) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, lock_manager);

  EXPECT_EQ(table.local_id(), local_id);
  EXPECT_EQ(table.size(), 0);
}

TEST(RoutingTableTest, AddNode) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  NodeID id2(std::string("2122232425262728292a2b2c2d2e2f3031323334"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);

  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);

  EXPECT_TRUE(table.add_node(node1));
  EXPECT_EQ(table.size(), 1);

  EXPECT_TRUE(table.add_node(node2));
  EXPECT_EQ(table.size(), 2);
}

TEST(RoutingTableTest, AddNodeAsync) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);

  auto future = table.add_node_async(node1);
  EXPECT_TRUE(future.get());
  EXPECT_EQ(table.size(), 1);
}

TEST(RoutingTableTest, AddLocalNode) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, lock_manager);

  Endpoint ep(std::string("192.168.1.1"), 6881);
  DHTNode local_node(local_id, ep);

  // Should not be able to add the local node
  EXPECT_FALSE(table.add_node(local_node));
  EXPECT_EQ(table.size(), 0);
}

TEST(RoutingTableTest, RemoveNode) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  NodeID id2(std::string("2122232425262728292a2b2c2d2e2f3031323334"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);

  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);

  table.add_node(node1);
  table.add_node(node2);

  EXPECT_EQ(table.size(), 2);

  EXPECT_TRUE(table.remove_node(id1));
  EXPECT_EQ(table.size(), 1);

  EXPECT_FALSE(table.remove_node(id1));
  EXPECT_EQ(table.size(), 1);

  EXPECT_TRUE(table.remove_node(id2));
  EXPECT_EQ(table.size(), 0);
}

TEST(RoutingTableTest, RemoveNodeAsync) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);

  table.add_node(node1);

  auto future = table.remove_node_async(id1);
  EXPECT_TRUE(future.get());
  EXPECT_EQ(table.size(), 0);
}

TEST(RoutingTableTest, UpdateNode) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);
  table.add_node(node1);

  DHTNode node2(id1, ep1, DHTNode::Status::GOOD);
  EXPECT_TRUE(table.update_node(node2));

  auto node = table.get_node(id1);
  EXPECT_TRUE(node.has_value());
  EXPECT_EQ(node->status(), DHTNode::Status::GOOD);
}

TEST(RoutingTableTest, UpdateNodeAsync) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);
  table.add_node(node1);

  DHTNode node2(id1, ep1, DHTNode::Status::GOOD);
  auto future = table.update_node_async(node2);
  EXPECT_TRUE(future.get());

  auto node = table.get_node(id1);
  EXPECT_TRUE(node.has_value());
  EXPECT_EQ(node->status(), DHTNode::Status::GOOD);
}

TEST(RoutingTableTest, GetNode) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  NodeID id2(std::string("2122232425262728292a2b2c2d2e2f3031323334"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);
  table.add_node(node1);

  auto node = table.get_node(id1);
  EXPECT_TRUE(node.has_value());
  EXPECT_EQ(node->id(), id1);

  auto node2 = table.get_node(id2);
  EXPECT_FALSE(node2.has_value());
}

TEST(RoutingTableTest, GetNodeAsync) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);
  table.add_node(node1);

  auto future = table.get_node_async(id1);
  auto node = future.get();
  EXPECT_TRUE(node.has_value());
  EXPECT_EQ(node->id(), id1);
}

TEST(RoutingTableTest, ContainsNode) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  NodeID id2(std::string("2122232425262728292a2b2c2d2e2f3031323334"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);
  table.add_node(node1);

  EXPECT_TRUE(table.contains_node(id1));
  EXPECT_FALSE(table.contains_node(id2));
}

TEST(RoutingTableTest, ContainsNodeAsync) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  NodeID id2(std::string("2122232425262728292a2b2c2d2e2f3031323334"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);

  DHTNode node1(id1, ep1);
  table.add_node(node1);

  auto future1 = table.contains_node_async(id1);
  EXPECT_TRUE(future1.get());

  auto future2 = table.contains_node_async(id2);
  EXPECT_FALSE(future2.get());
}

TEST(RoutingTableTest, GetAllNodes) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  NodeID id2(std::string("2122232425262728292a2b2c2d2e2f3031323334"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);

  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);

  table.add_node(node1);
  table.add_node(node2);

  auto nodes = table.get_all_nodes();
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

TEST(RoutingTableTest, GetAllNodesAsync) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  NodeID id1(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  NodeID id2(std::string("2122232425262728292a2b2c2d2e2f3031323334"));
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6882);

  DHTNode node1(id1, ep1);
  DHTNode node2(id2, ep2);

  table.add_node(node1);
  table.add_node(node2);

  auto future = table.get_all_nodes_async();
  auto nodes = future.get();
  EXPECT_EQ(nodes.size(), 2);
}

TEST(RoutingTableTest, GetClosestNodes) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  // Add some nodes with different distances to a target
  NodeID target(std::string("2122232425262728292a2b2c2d2e2f3031323334"));

  // Create nodes with increasing distance from the target
  for (int i = 0; i < 10; ++i) {
    NodeID id = target;
    // Flip some bits to create a node at a specific distance
    for (int j = 0; j < i; ++j) {
      id.flip_bit(j);
    }

    Endpoint ep(std::string("192.168.1.1"), 6881 + i);
    DHTNode node(id, ep);

    table.add_node(node);
  }

  // Get the 5 closest nodes
  auto closest = table.get_closest_nodes(target, 5);
  EXPECT_EQ(closest.size(), 5);

  // Check that they are sorted by distance
  for (size_t i = 1; i < closest.size(); ++i) {
    NodeID dist1 = closest[i-1].id().distance(target);
    NodeID dist2 = closest[i].id().distance(target);

    EXPECT_LE(dist1, dist2);
  }
}

TEST(RoutingTableTest, GetClosestNodesAsync) {
  NodeID local_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  auto lock_manager = LockManagerSingleton::instance();
  RoutingTable table(local_id, *lock_manager);

  // Add some nodes with different distances to a target
  NodeID target(std::string("2122232425262728292a2b2c2d2e2f3031323334"));

  // Create nodes with increasing distance from the target
  for (int i = 0; i < 10; ++i) {
    NodeID id = target;
    // Flip some bits to create a node at a specific distance
    for (int j = 0; j < i; ++j) {
      id.flip_bit(j);
    }

    Endpoint ep(std::string("192.168.1.1"), 6881 + i);
    DHTNode node(id, ep);

    table.add_node(node);
  }

  // Get the 5 closest nodes asynchronously
  auto future = table.get_closest_nodes_async(target, 5);
  auto closest = future.get();
  EXPECT_EQ(closest.size(), 5);
}
