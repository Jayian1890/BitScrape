#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <memory>

#include "bitscrape/types/dht_node.hpp"

namespace bitscrape::types {

/**
 * @brief Represents an entry in the DHT routing table
 * 
 * DHTRoutingTableEntry encapsulates an entry in the DHT routing table,
 * which consists of a k-bucket of nodes and a range of node IDs.
 */
class DHTRoutingTableEntry {
public:
    /// Maximum number of nodes in a k-bucket (k=8 in the BitTorrent DHT)
    static constexpr size_t K = 8;
    
    /**
     * @brief Create a routing table entry with the specified prefix length
     * 
     * @param prefix_length Number of bits in the common prefix for this bucket
     */
    explicit DHTRoutingTableEntry(uint8_t prefix_length);
    
    /**
     * @brief Copy constructor
     */
    DHTRoutingTableEntry(const DHTRoutingTableEntry& other) = default;
    
    /**
     * @brief Move constructor
     */
    DHTRoutingTableEntry(DHTRoutingTableEntry&& other) noexcept = default;
    
    /**
     * @brief Copy assignment operator
     */
    DHTRoutingTableEntry& operator=(const DHTRoutingTableEntry& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    DHTRoutingTableEntry& operator=(DHTRoutingTableEntry&& other) noexcept = default;
    
    /**
     * @brief Destructor
     */
    ~DHTRoutingTableEntry() = default;
    
    /**
     * @brief Get the prefix length
     * 
     * @return Prefix length
     */
    uint8_t prefix_length() const { return prefix_length_; }
    
    /**
     * @brief Get the nodes in this bucket
     * 
     * @return Vector of nodes
     */
    const std::vector<DHTNode>& nodes() const { return nodes_; }
    
    /**
     * @brief Get the last updated time
     * 
     * @return Last updated time
     */
    std::chrono::system_clock::time_point last_updated() const { return last_updated_; }
    
    /**
     * @brief Add a node to the bucket
     * 
     * If the bucket is full, the node is not added.
     * 
     * @param node Node to add
     * @return true if the node was added, false otherwise
     */
    bool add_node(const DHTNode& node);
    
    /**
     * @brief Remove a node from the bucket
     * 
     * @param node Node to remove
     * @return true if the node was removed, false otherwise
     */
    bool remove_node(const DHTNode& node);
    
    /**
     * @brief Update a node in the bucket
     * 
     * If the node is not in the bucket, it is not updated.
     * 
     * @param node Node to update
     * @return true if the node was updated, false otherwise
     */
    bool update_node(const DHTNode& node);
    
    /**
     * @brief Check if the bucket contains a node
     * 
     * @param node Node to check
     * @return true if the bucket contains the node, false otherwise
     */
    bool contains_node(const DHTNode& node) const;
    
    /**
     * @brief Check if the bucket contains a node with the specified ID
     * 
     * @param id Node ID to check
     * @return true if the bucket contains a node with the ID, false otherwise
     */
    bool contains_node_id(const NodeID& id) const;
    
    /**
     * @brief Get a node by ID
     * 
     * @param id Node ID
     * @return Pointer to the node if found, nullptr otherwise
     */
    const DHTNode* get_node(const NodeID& id) const;
    
    /**
     * @brief Check if the bucket is full
     * 
     * @return true if the bucket is full, false otherwise
     */
    bool is_full() const { return nodes_.size() >= K; }
    
    /**
     * @brief Check if the bucket is empty
     * 
     * @return true if the bucket is empty, false otherwise
     */
    bool is_empty() const { return nodes_.empty(); }
    
    /**
     * @brief Get the number of nodes in the bucket
     * 
     * @return Number of nodes
     */
    size_t size() const { return nodes_.size(); }
    
    /**
     * @brief Check if a node ID belongs in this bucket
     * 
     * @param id Node ID
     * @param local_id Local node ID
     * @return true if the node ID belongs in this bucket, false otherwise
     */
    bool contains_id_in_range(const NodeID& id, const NodeID& local_id) const;
    
    /**
     * @brief Convert the entry to a string representation
     * 
     * @return String representation of the entry
     */
    std::string to_string() const;

private:
    uint8_t prefix_length_;                           ///< Number of bits in the common prefix for this bucket
    std::vector<DHTNode> nodes_;                      ///< Nodes in this bucket
    std::chrono::system_clock::time_point last_updated_; ///< Last updated time
};

} // namespace bitscrape::types
