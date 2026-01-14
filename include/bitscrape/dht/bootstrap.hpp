#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <future>
#include <atomic>
#include <condition_variable>
#include <unordered_map>

#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/endpoint.hpp"
#include "bitscrape/dht/routing_table.hpp"
#include "bitscrape/dht/node_lookup.hpp"
#include "bitscrape/dht/dht_message_factory.hpp"
#include "bitscrape/network/udp_socket.hpp"

namespace bitscrape::dht {

class DHTSession;

/**
 * @brief Manages the bootstrap process for joining the DHT network
 *
 * Bootstrap is responsible for joining the DHT network by contacting known bootstrap nodes
 * and performing initial node lookups to populate the routing table.
 */
class Bootstrap {
public:
    /// Number of random node IDs to look up during bootstrap
    static constexpr size_t RANDOM_LOOKUPS = 3;

    /**
     * @brief Create a bootstrap manager with the specified parameters
     *
     * @param local_id Local node ID
     * @param routing_table Routing table to populate
     * @param socket UDP socket for sending and receiving messages
     * @param message_factory Factory for creating DHT messages
     */
    Bootstrap(const types::NodeID& local_id,
              RoutingTable& routing_table,
              network::UDPSocket& socket,
              DHTMessageFactory& message_factory,
              DHTSession& session);

    /**
     * @brief Start the bootstrap process with the specified bootstrap nodes
     *
     * @param bootstrap_nodes List of bootstrap nodes to contact
     * @return true if the bootstrap process was successful, false otherwise
     */
    bool start(const std::vector<types::Endpoint>& bootstrap_nodes);

    /**
     * @brief Start the bootstrap process with the specified bootstrap nodes asynchronously
     *
     * @param bootstrap_nodes List of bootstrap nodes to contact
     * @return Future containing true if the bootstrap process was successful, false otherwise
     */
    std::future<bool> start_async(const std::vector<types::Endpoint>& bootstrap_nodes);

    /**
     * @brief Process a message received during the bootstrap process
     *
     * @param message Received message
     * @param sender_endpoint Endpoint of the sender
     */
    void process_message(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint);

    /**
     * @brief Check if the bootstrap process is complete
     *
     * @return true if the bootstrap process is complete, false otherwise
     */
    bool is_complete() const;

    /**
     * @brief Wait for the bootstrap process to complete
     *
     * @param timeout_ms Timeout in milliseconds, or 0 for no timeout
     * @return true if the bootstrap process completed, false if it timed out
     */
    bool wait_for_completion(int timeout_ms = 5000);

private:
    /**
     * @brief Contact a bootstrap node
     *
     * @param endpoint Endpoint of the bootstrap node
     * @return true if the node was contacted successfully, false otherwise
     */
    bool contact_bootstrap_node(const types::Endpoint& endpoint);

    /**
     * @brief Perform a lookup for a random node ID
     *
     * @return true if the lookup was successful, false otherwise
     */
    bool perform_random_lookup();

    /**
     * @brief Generate a random node ID
     *
     * @return Random node ID
     */
    types::NodeID generate_random_node_id() const;

    types::NodeID local_id_;                      ///< Local node ID
    RoutingTable& routing_table_;                 ///< Routing table to populate
    network::UDPSocket& socket_;                  ///< UDP socket for sending and receiving messages
    DHTMessageFactory& message_factory_;          ///< Factory for creating DHT messages
    DHTSession& session_;                         ///< Reference to the DHT session

    std::atomic<size_t> active_lookups_;          ///< Number of active lookups
    std::atomic<bool> complete_;                  ///< Whether the bootstrap process is complete

    mutable std::mutex mutex_;                    ///< Mutex for thread safety
    std::condition_variable cv_;                  ///< Condition variable for waiting for completion
    std::unordered_map<std::string, types::Endpoint> pending_pings_; ///< Map of transaction IDs to endpoints for pending pings
};

} // namespace bitscrape::dht
