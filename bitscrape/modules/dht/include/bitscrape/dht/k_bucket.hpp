#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <chrono>
#include <future>
#include <optional>

#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/dht_node.hpp"

namespace bitscrape::dht {

/**
 * @brief Represents a k-bucket in the DHT routing table
 * 
 * KBucket is a container for nodes in the DHT routing table.
 * It stores up to k nodes with a common prefix length.
 */
class KBucket {
public:
    /// Maximum number of nodes in a k-bucket (k=8 in the BitTorrent DHT)
    static constexpr size_t K = 8;
    
    /**
     * @brief Create a k-bucket with the specified prefix length
     * 
     * @param prefix_length Number of bits in the common prefix for this bucket
     */
    explicit KBucket(uint8_t prefix_length);
    
    /**
     * @brief Add a node to the bucket
     * 
     * @param node Node to add
     * @return true if the node was added, false if the bucket is full or the node is already in the bucket
     */
    bool add_node(const types::DHTNode& node);
    
    /**
     * @brief Add a node to the bucket asynchronously
     * 
     * @param node Node to add
     * @return Future containing true if the node was added, false if the bucket is full or the node is already in the bucket
     */
    std::future<bool> add_node_async(const types::DHTNode& node);
    
    /**
     * @brief Remove a node from the bucket
     * 
     * @param node_id ID of the node to remove
     * @return true if the node was removed, false if the node was not in the bucket
     */
    bool remove_node(const types::NodeID& node_id);
    
    /**
     * @brief Remove a node from the bucket asynchronously
     * 
     * @param node_id ID of the node to remove
     * @return Future containing true if the node was removed, false if the node was not in the bucket
     */
    std::future<bool> remove_node_async(const types::NodeID& node_id);
    
    /**
     * @brief Update a node in the bucket
     * 
     * @param node Updated node
     * @return true if the node was updated, false if the node was not in the bucket
     */
    bool update_node(const types::DHTNode& node);
    
    /**
     * @brief Update a node in the bucket asynchronously
     * 
     * @param node Updated node
     * @return Future containing true if the node was updated, false if the node was not in the bucket
     */
    std::future<bool> update_node_async(const types::DHTNode& node);
    
    /**
     * @brief Get a node from the bucket
     * 
     * @param node_id ID of the node to get
     * @return Optional containing the node if found, empty optional otherwise
     */
    std::optional<types::DHTNode> get_node(const types::NodeID& node_id) const;
    
    /**
     * @brief Get a node from the bucket asynchronously
     * 
     * @param node_id ID of the node to get
     * @return Future containing an optional with the node if found, empty optional otherwise
     */
    std::future<std::optional<types::DHTNode>> get_node_async(const types::NodeID& node_id) const;
    
    /**
     * @brief Check if the bucket contains a node
     * 
     * @param node_id ID of the node to check
     * @return true if the bucket contains the node, false otherwise
     */
    bool contains_node(const types::NodeID& node_id) const;
    
    /**
     * @brief Check if the bucket contains a node asynchronously
     * 
     * @param node_id ID of the node to check
     * @return Future containing true if the bucket contains the node, false otherwise
     */
    std::future<bool> contains_node_async(const types::NodeID& node_id) const;
    
    /**
     * @brief Get all nodes in the bucket
     * 
     * @return Vector of nodes in the bucket
     */
    std::vector<types::DHTNode> get_nodes() const;
    
    /**
     * @brief Get all nodes in the bucket asynchronously
     * 
     * @return Future containing a vector of nodes in the bucket
     */
    std::future<std::vector<types::DHTNode>> get_nodes_async() const;
    
    /**
     * @brief Get the number of nodes in the bucket
     * 
     * @return Number of nodes in the bucket
     */
    size_t size() const;
    
    /**
     * @brief Check if the bucket is empty
     * 
     * @return true if the bucket is empty, false otherwise
     */
    bool is_empty() const;
    
    /**
     * @brief Check if the bucket is full
     * 
     * @return true if the bucket is full, false otherwise
     */
    bool is_full() const;
    
    /**
     * @brief Get the prefix length of the bucket
     * 
     * @return Prefix length in bits
     */
    uint8_t prefix_length() const;
    
    /**
     * @brief Get the last updated time of the bucket
     * 
     * @return Last updated time
     */
    std::chrono::system_clock::time_point last_updated() const;
    
    /**
     * @brief Update the last updated time to now
     */
    void update_last_updated();
    
private:
    uint8_t prefix_length_;                                  ///< Prefix length in bits
    std::vector<types::DHTNode> nodes_;                      ///< Nodes in the bucket
    std::chrono::system_clock::time_point last_updated_;     ///< Last updated time
    mutable std::mutex mutex_;                               ///< Mutex for thread safety
};

} // namespace bitscrape::dht
