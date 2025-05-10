#pragma once

#include <cstdint>
#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <functional>
#include <memory>
#include <atomic>
#include <condition_variable>

#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/dht_node.hpp"
#include "bitscrape/dht/routing_table.hpp"
#include "bitscrape/dht/dht_message.hpp"
#include "bitscrape/dht/dht_message_factory.hpp"
#include "bitscrape/network/udp_socket.hpp"

namespace bitscrape::dht {

/**
 * @brief Performs a node lookup in the DHT network
 * 
 * NodeLookup implements the iterative node lookup algorithm for finding nodes close to a target ID.
 * It sends find_node queries to the closest known nodes and processes the responses.
 */
class NodeLookup {
public:
    /// Alpha value for parallel lookups (number of concurrent requests)
    static constexpr size_t ALPHA = 3;
    
    /// Maximum number of nodes to return
    static constexpr size_t K = 8;
    
    /// Timeout for node lookup in milliseconds
    static constexpr int TIMEOUT_MS = 1500;
    
    /// Maximum number of timeouts before considering a node unresponsive
    static constexpr int MAX_TIMEOUTS = 2;
    
    /**
     * @brief Create a node lookup with the specified parameters
     * 
     * @param local_id Local node ID
     * @param target_id Target ID to look up
     * @param routing_table Routing table to use for initial nodes
     * @param socket UDP socket for sending and receiving messages
     * @param message_factory Factory for creating DHT messages
     */
    NodeLookup(const types::NodeID& local_id,
               const types::NodeID& target_id,
               const RoutingTable& routing_table,
               network::UDPSocket& socket,
               DHTMessageFactory& message_factory);
    
    /**
     * @brief Destructor
     */
    ~NodeLookup();
    
    /**
     * @brief Start the node lookup
     * 
     * @return Vector of the k closest nodes to the target ID
     */
    std::vector<types::DHTNode> start();
    
    /**
     * @brief Start the node lookup asynchronously
     * 
     * @return Future containing a vector of the k closest nodes to the target ID
     */
    std::future<std::vector<types::DHTNode>> start_async();
    
    /**
     * @brief Process a find_node response
     * 
     * @param response Response message
     * @param sender_endpoint Endpoint of the sender
     */
    void process_response(const std::shared_ptr<DHTMessage>& response, const types::Endpoint& sender_endpoint);
    
    /**
     * @brief Check if the lookup is complete
     * 
     * @return true if the lookup is complete, false otherwise
     */
    bool is_complete() const;
    
    /**
     * @brief Wait for the lookup to complete
     * 
     * @param timeout_ms Timeout in milliseconds, or 0 for no timeout
     * @return true if the lookup completed, false if it timed out
     */
    bool wait_for_completion(int timeout_ms = 0);
    
private:
    /**
     * @brief Node state in the lookup process
     */
    enum class NodeState {
        UNKNOWN,    ///< Node has not been queried yet
        QUERIED,    ///< Query has been sent to the node
        RESPONDED,  ///< Node has responded
        FAILED      ///< Node failed to respond
    };
    
    /**
     * @brief Node entry in the lookup process
     */
    struct NodeEntry {
        types::DHTNode node;      ///< The node
        NodeState state;          ///< Current state of the node
        int timeouts;             ///< Number of timeouts for this node
        
        /**
         * @brief Create a node entry
         * 
         * @param node The node
         * @param state Initial state
         */
        NodeEntry(const types::DHTNode& node, NodeState state = NodeState::UNKNOWN)
            : node(node), state(state), timeouts(0) {}
        
        /**
         * @brief Compare node entries by distance to the target
         * 
         * @param target Target ID
         * @param a First node entry
         * @param b Second node entry
         * @return true if a is closer to the target than b
         */
        static bool compare_distance(const types::NodeID& target, const NodeEntry& a, const NodeEntry& b) {
            return a.node.id().distance(target) < b.node.id().distance(target);
        }
    };
    
    /**
     * @brief Send find_node queries to the closest nodes
     */
    void send_queries();
    
    /**
     * @brief Send a find_node query to a node
     * 
     * @param node Node to query
     * @return true if the query was sent, false otherwise
     */
    bool send_query(const types::DHTNode& node);
    
    /**
     * @brief Add a node to the lookup process
     * 
     * @param node Node to add
     */
    void add_node(const types::DHTNode& node);
    
    /**
     * @brief Add multiple nodes to the lookup process
     * 
     * @param nodes Nodes to add
     */
    void add_nodes(const std::vector<types::DHTNode>& nodes);
    
    /**
     * @brief Get the closest nodes that have not been queried yet
     * 
     * @param count Maximum number of nodes to return
     * @return Vector of nodes
     */
    std::vector<types::DHTNode> get_closest_unqueried_nodes(size_t count);
    
    /**
     * @brief Check if the lookup has converged
     * 
     * @return true if the lookup has converged, false otherwise
     */
    bool has_converged() const;
    
    /**
     * @brief Get the k closest nodes found so far
     * 
     * @return Vector of the k closest nodes
     */
    std::vector<types::DHTNode> get_closest_nodes() const;
    
    types::NodeID local_id_;                      ///< Local node ID
    types::NodeID target_id_;                     ///< Target ID to look up
    const RoutingTable& routing_table_;           ///< Routing table for initial nodes
    network::UDPSocket& socket_;                  ///< UDP socket for sending and receiving messages
    DHTMessageFactory& message_factory_;          ///< Factory for creating DHT messages
    
    std::vector<NodeEntry> nodes_;                ///< Nodes in the lookup process
    std::atomic<size_t> active_queries_;          ///< Number of active queries
    std::atomic<bool> complete_;                  ///< Whether the lookup is complete
    
    mutable std::mutex mutex_;                    ///< Mutex for thread safety
    std::condition_variable cv_;                  ///< Condition variable for waiting for completion
};

} // namespace bitscrape::dht
