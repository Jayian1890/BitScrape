#pragma once

#include <cstdint>
#include <string>
#include <future>
#include <chrono>

#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/endpoint.hpp"

namespace bitscrape::types {

/**
 * @brief Represents a node in the DHT network
 * 
 * DHTNode encapsulates a node in the DHT network, consisting of a NodeID and an Endpoint.
 * It also tracks the node's status, last seen time, and other properties.
 */
class DHTNode {
public:
    /**
     * @brief Node status enumeration
     */
    enum class Status {
        UNKNOWN,    ///< Node has been discovered but not contacted
        GOOD,       ///< Node is responsive and good
        QUESTIONABLE, ///< Node has been unresponsive recently
        BAD         ///< Node is unresponsive or misbehaving
    };

    /**
     * @brief Default constructor creates an invalid node
     */
    DHTNode();
    
    /**
     * @brief Create a node with the specified ID and endpoint
     * 
     * @param id Node ID
     * @param endpoint Node endpoint (IP and port)
     */
    DHTNode(const NodeID& id, const Endpoint& endpoint);
    
    /**
     * @brief Create a node with the specified ID, endpoint, and status
     * 
     * @param id Node ID
     * @param endpoint Node endpoint (IP and port)
     * @param status Node status
     */
    DHTNode(const NodeID& id, const Endpoint& endpoint, Status status);
    
    /**
     * @brief Copy constructor
     */
    DHTNode(const DHTNode& other) = default;
    
    /**
     * @brief Move constructor
     */
    DHTNode(DHTNode&& other) noexcept = default;
    
    /**
     * @brief Copy assignment operator
     */
    DHTNode& operator=(const DHTNode& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    DHTNode& operator=(DHTNode&& other) noexcept = default;
    
    /**
     * @brief Destructor
     */
    ~DHTNode() = default;
    
    /**
     * @brief Get the node ID
     * 
     * @return Node ID
     */
    const NodeID& id() const { return id_; }
    
    /**
     * @brief Get the node endpoint
     * 
     * @return Node endpoint
     */
    const Endpoint& endpoint() const { return endpoint_; }
    
    /**
     * @brief Get the node status
     * 
     * @return Node status
     */
    Status status() const { return status_; }
    
    /**
     * @brief Set the node status
     * 
     * @param status New status
     */
    void set_status(Status status);
    
    /**
     * @brief Get the last seen time
     * 
     * @return Last seen time
     */
    std::chrono::system_clock::time_point last_seen() const { return last_seen_; }
    
    /**
     * @brief Update the last seen time to now
     */
    void update_last_seen();

    /**
     * @brief Get the last RTT in milliseconds
     * 
     * @return Last RTT in milliseconds
     */
    uint32_t last_rtt_ms() const { return last_rtt_ms_; }

    /**
     * @brief Set the last RTT in milliseconds
     * 
     * @param rtt_ms Last RTT in milliseconds
     */
    void set_last_rtt_ms(uint32_t rtt_ms) { last_rtt_ms_ = rtt_ms; }
    
    /**
     * @brief Check if the node is valid
     * 
     * A node is valid if it has a valid endpoint.
     * 
     * @return true if the node is valid, false otherwise
     */
    bool is_valid() const;
    
    /**
     * @brief Calculate the distance to another node
     * 
     * @param other Other node
     * @return Distance between this node and the other node
     */
    NodeID distance(const DHTNode& other) const;
    
    /**
     * @brief Calculate the distance to another node asynchronously
     * 
     * @param other Other node
     * @return Future containing the distance
     */
    std::future<NodeID> distance_async(const DHTNode& other) const;
    
    /**
     * @brief Calculate the distance to a node ID
     * 
     * @param id Node ID
     * @return Distance between this node and the node ID
     */
    NodeID distance(const NodeID& id) const;
    
    /**
     * @brief Calculate the distance to a node ID asynchronously
     * 
     * @param id Node ID
     * @return Future containing the distance
     */
    std::future<NodeID> distance_async(const NodeID& id) const;
    
    /**
     * @brief Convert the node to a string representation
     * 
     * @return String representation of the node
     */
    std::string to_string() const;
    
    /**
     * @brief Equality operator
     */
    bool operator==(const DHTNode& other) const;
    
    /**
     * @brief Inequality operator
     */
    bool operator!=(const DHTNode& other) const;

private:
    NodeID id_;                                  ///< Node ID
    Endpoint endpoint_;                          ///< Node endpoint
    Status status_ = Status::UNKNOWN;            ///< Node status
    std::chrono::system_clock::time_point last_seen_; ///< Last seen time
    uint32_t last_rtt_ms_ = 0;                   ///< Last RTT in milliseconds
};

} // namespace bitscrape::types
