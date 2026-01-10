#pragma once

#include <cstdint>
#include <vector>
#include <future>
#include <optional>

#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/dht_node.hpp"
#include "bitscrape/dht/k_bucket.hpp"
#include "bitscrape/lock/lock_manager.hpp"
#include "bitscrape/lock/lock_guard.hpp"

namespace bitscrape::dht {

/**
 * @brief Represents a routing table in the DHT network
 *
 * RoutingTable manages a collection of k-buckets for storing nodes in the DHT network.
 * It organizes nodes based on their distance from the local node ID.
 */
class RoutingTable {
public:
    /**
     * @brief Create a routing table with the specified local node ID
     *
     * @param local_id Local node ID
     * @param lock_manager Pointer to the lock manager
     */
    RoutingTable(const types::NodeID& local_id, std::shared_ptr<lock::LockManager> lock_manager);

    /**
     * @brief Add a node to the routing table
     *
     * @param node Node to add
     * @return true if the node was added, false if the node could not be added
     */
    bool add_node(const types::DHTNode& node);

    /**
     * @brief Add a node to the routing table asynchronously
     *
     * @param node Node to add
     * @return Future containing true if the node was added, false if the node could not be added
     */
    std::future<bool> add_node_async(const types::DHTNode& node);

    /**
     * @brief Remove a node from the routing table
     *
     * @param node_id ID of the node to remove
     * @return true if the node was removed, false if the node was not in the routing table
     */
    bool remove_node(const types::NodeID& node_id);

    /**
     * @brief Remove a node from the routing table asynchronously
     *
     * @param node_id ID of the node to remove
     * @return Future containing true if the node was removed, false if the node was not in the routing table
     */
    std::future<bool> remove_node_async(const types::NodeID& node_id);

    /**
     * @brief Update a node in the routing table
     *
     * @param node Updated node
     * @return true if the node was updated, false if the node was not in the routing table
     */
    bool update_node(const types::DHTNode& node);

    /**
     * @brief Update a node in the routing table asynchronously
     *
     * @param node Updated node
     * @return Future containing true if the node was updated, false if the node was not in the routing table
     */
    std::future<bool> update_node_async(const types::DHTNode& node);

    /**
     * @brief Get a node from the routing table
     *
     * @param node_id ID of the node to get
     * @return Optional containing the node if found, empty optional otherwise
     */
    std::optional<types::DHTNode> get_node(const types::NodeID& node_id) const;

    /**
     * @brief Get a node from the routing table asynchronously
     *
     * @param node_id ID of the node to get
     * @return Future containing an optional with the node if found, empty optional otherwise
     */
    std::future<std::optional<types::DHTNode>> get_node_async(const types::NodeID& node_id) const;

    /**
     * @brief Check if the routing table contains a node
     *
     * @param node_id ID of the node to check
     * @return true if the routing table contains the node, false otherwise
     */
    bool contains_node(const types::NodeID& node_id) const;

    /**
     * @brief Check if the routing table contains a node asynchronously
     *
     * @param node_id ID of the node to check
     * @return Future containing true if the routing table contains the node, false otherwise
     */
    std::future<bool> contains_node_async(const types::NodeID& node_id) const;

    /**
     * @brief Get the k closest nodes to a target ID
     *
     * @param target_id Target ID
     * @param k Maximum number of nodes to return
     * @return Vector of the k closest nodes to the target ID
     */
    std::vector<types::DHTNode> get_closest_nodes(const types::NodeID& target_id, size_t k) const;

    /**
     * @brief Get the k closest nodes to a target ID asynchronously
     *
     * @param target_id Target ID
     * @param k Maximum number of nodes to return
     * @return Future containing a vector of the k closest nodes to the target ID
     */
    std::future<std::vector<types::DHTNode>> get_closest_nodes_async(const types::NodeID& target_id, size_t k) const;

    /**
     * @brief Get all nodes in the routing table
     *
     * @return Vector of all nodes in the routing table
     */
    std::vector<types::DHTNode> get_all_nodes() const;

    /**
     * @brief Get all nodes in the routing table asynchronously
     *
     * @return Future containing a vector of all nodes in the routing table
     */
    std::future<std::vector<types::DHTNode>> get_all_nodes_async() const;

    /**
     * @brief Get the number of nodes in the routing table
     *
     * @return Number of nodes in the routing table
     */
    size_t size() const;

    /**
     * @brief Get the local node ID
     *
     * @return Local node ID
     */
    const types::NodeID& local_id() const;

private:
    /**
     * @brief Get the bucket index for a node ID
     *
     * @param node_id Node ID
     * @return Bucket index
     */
    size_t get_bucket_index(const types::NodeID& node_id) const;

    /**
     * @brief Get or create a bucket for a node ID
     *
     * @param node_id Node ID
     * @return Reference to the bucket
     */
    KBucket& get_or_create_bucket(const types::NodeID& node_id);

    /**
     * @brief Internal method to get all nodes without acquiring the lock
     * Caller must hold the mutex_
     *
     * @return Vector of all nodes in the routing table
     */
    std::vector<types::DHTNode> get_all_nodes_internal() const;

    /**
     * @brief Get a resource name for this routing table
     *
     * @return Resource name string
     */
    std::string get_resource_name() const;

    types::NodeID local_id_;                 ///< Local node ID
    std::vector<KBucket> buckets_;           ///< Buckets for storing nodes
    std::shared_ptr<lock::LockManager> lock_manager_; ///< Pointer to the lock manager
    uint64_t resource_id_;                   ///< Resource ID for the lock manager
};

} // namespace bitscrape::dht
