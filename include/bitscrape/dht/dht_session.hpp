#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <future>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <condition_variable>
#include <functional>

#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/types/endpoint.hpp"
#include "bitscrape/types/dht_node.hpp"
#include "bitscrape/dht/routing_table.hpp"
#include "bitscrape/dht/bootstrap.hpp"
#include "bitscrape/dht/node_lookup.hpp"
#include "bitscrape/dht/dht_message.hpp"
#include "bitscrape/dht/dht_message_factory.hpp"
#include "bitscrape/dht/dht_find_node_message.hpp"
#include "bitscrape/dht/dht_get_peers_message.hpp"
#include "bitscrape/dht/dht_announce_peer_message.hpp"
#include "bitscrape/dht/token_manager.hpp"
#include "bitscrape/network/udp_socket.hpp"
#include "bitscrape/event/event_bus.hpp"
#include "bitscrape/lock/lock_manager.hpp"
#include "bitscrape/lock/lock_guard.hpp"

namespace bitscrape::dht {

/**
 * @brief Main class for DHT functionality
 *
 * DHTSession is the main entry point for the DHT module.
 * It provides high-level methods for DHT operations such as finding nodes and peers.
 */
class DHTSession {
public:
    /**
     * @brief Create a DHT session with a random node ID
     *
     * @param lock_manager Pointer to the lock manager
     */
    explicit DHTSession(std::shared_ptr<lock::LockManager> lock_manager);

    /**
     * @brief Create a DHT session with the specified node ID
     *
     * @param node_id Node ID to use
     * @param lock_manager Pointer to the lock manager
     */
    DHTSession(const types::NodeID& node_id, std::shared_ptr<lock::LockManager> lock_manager);

    /**
     * @brief Create a DHT session with the specified parameters
     *
     * @param node_id Node ID to use
     * @param port UDP port to listen on
     * @param event_bus Event bus for event-driven communication
     * @param lock_manager Pointer to the lock manager
     */
    DHTSession(const types::NodeID& node_id, uint16_t port, event::EventBus& event_bus, std::shared_ptr<lock::LockManager> lock_manager);

    /**
     * @brief Destructor
     */
    ~DHTSession();

    /**
     * @brief Start the DHT session
     *
     * @param bootstrap_nodes List of bootstrap nodes to contact
     * @return true if the session was started successfully, false otherwise
     */
    bool start(const std::vector<types::Endpoint>& bootstrap_nodes);

    /**
     * @brief Start the DHT session asynchronously
     *
     * @param bootstrap_nodes List of bootstrap nodes to contact
     * @return Future containing true if the session was started successfully, false otherwise
     */
    std::future<bool> start_async(const std::vector<types::Endpoint>& bootstrap_nodes);

    /**
     * @brief Stop the DHT session
     */
    void stop();

    /**
     * @brief Find nodes close to a target ID
     *
     * @param target_id Target ID to look up
     * @return Vector of nodes close to the target ID
     */
    std::vector<types::DHTNode> find_nodes(const types::NodeID& target_id);

    /**
     * @brief Find nodes close to a target ID asynchronously
     *
     * @param target_id Target ID to look up
     * @return Future containing a vector of nodes close to the target ID
     */
    std::future<std::vector<types::DHTNode>> find_nodes_async(const types::NodeID& target_id);

    /**
     * @brief Find peers for an infohash
     *
     * @param infohash Infohash to look up
     * @return Vector of endpoints for peers with the infohash
     */
    std::vector<types::Endpoint> find_peers(const types::InfoHash& infohash);

    /**
     * @brief Find peers for an infohash asynchronously
     *
     * @param infohash Infohash to look up
     * @return Future containing a vector of endpoints for peers with the infohash
     */
    std::future<std::vector<types::Endpoint>> find_peers_async(const types::InfoHash& infohash);

    /**
     * @brief Announce as a peer for an infohash
     *
     * @param infohash Infohash to announce for
     * @param port Port to announce
     * @return true if the announce was successful, false otherwise
     */
    bool announce_peer(const types::InfoHash& infohash, uint16_t port);

    /**
     * @brief Announce as a peer for an infohash asynchronously
     *
     * @param infohash Infohash to announce for
     * @param port Port to announce
     * @return Future containing true if the announce was successful, false otherwise
     */
    std::future<bool> announce_peer_async(const types::InfoHash& infohash, uint16_t port);

    /**
     * @brief Register a transaction ID with a lookup
     *
     * @param transaction_id Transaction ID
     * @param lookup Lookup to register
     */
    void register_transaction(const std::string& transaction_id, std::shared_ptr<NodeLookup> lookup);

    /**
     * @brief Unregister a transaction ID
     *
     * @param transaction_id Transaction ID
     */
    void unregister_transaction(const std::string& transaction_id);

    /**
     * @brief Get the local node ID
     *
     * @return Local node ID
     */
    const types::NodeID& node_id() const;

    /**
     * @brief Get the routing table
     *
     * @return Reference to the routing table
     */
    const RoutingTable& routing_table() const;

    /**
     * @brief Check if the session is running
     *
     * @return true if the session is running, false otherwise
     */
    bool is_running() const;

    /**
     * @brief Set callback for infohash discovery
     * 
     * This callback is invoked when other nodes query for infohashes via
     * get_peers or announce_peer messages, allowing passive collection.
     *
     * @param callback Function to call with discovered infohash
     */
    void set_infohash_callback(std::function<void(const types::InfoHash&)> callback);

private:
    /**
     * @brief Process a received message
     *
     * @param data Raw message data
     * @param sender_endpoint Endpoint of the sender
     */
    void process_message(const std::vector<uint8_t>& data, const types::Endpoint& sender_endpoint);

    /**
     * @brief Start the message receiving loop
     */
    void start_receive_loop();

    /**
     * @brief Handle a ping message
     *
     * @param message Ping message
     * @param sender_endpoint Endpoint of the sender
     */
    void handle_ping(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint);

    /**
     * @brief Handle a find_node message
     *
     * @param message Find_node message
     * @param sender_endpoint Endpoint of the sender
     */
    void handle_find_node(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint);

    /**
     * @brief Handle a get_peers message
     *
     * @param message Get_peers message
     * @param sender_endpoint Endpoint of the sender
     */
    void handle_get_peers(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint);

    /**
     * @brief Handle an announce_peer message
     *
     * @param message Announce_peer message
     * @param sender_endpoint Endpoint of the sender
     */
    void handle_announce_peer(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint);

    types::NodeID node_id_;                                  ///< Local node ID
    uint16_t port_;                                          ///< UDP port to listen on
    [[maybe_unused]] event::EventBus*
        event_bus_;                                          ///< Event bus for event-driven communication

    std::unique_ptr<network::UDPSocket> socket_;             ///< UDP socket for sending and receiving messages
    std::unique_ptr<RoutingTable> routing_table_;            ///< Routing table for storing nodes
    std::unique_ptr<DHTMessageFactory> message_factory_;     ///< Factory for creating DHT messages
    std::unique_ptr<TokenManager> token_manager_;            ///< Manager for token generation and verification

    std::atomic<bool> running_;                              ///< Whether the session is running
    std::thread receive_thread_;                             ///< Thread for receiving messages

    /**
     * @brief Get a resource name for the peers map
     *
     * @return Resource name string
     */
    std::string get_peers_resource_name() const;

    /**
     * @brief Get a resource name for the lookups map
     *
     * @return Resource name string
     */
    std::string get_lookups_resource_name() const;

    std::unordered_map<types::InfoHash, std::vector<types::Endpoint>> peers_; ///< Map of infohashes to peers
    std::unordered_map<std::string, std::shared_ptr<NodeLookup>> lookups_; ///< Active node lookups

    std::shared_ptr<lock::LockManager> lock_manager_;         ///< Pointer to the lock manager
    uint64_t peers_resource_id_;                             ///< Resource ID for the peers map
    uint64_t lookups_resource_id_;                           ///< Resource ID for the lookups map

    std::unique_ptr<Bootstrap> bootstrap_;                    ///< Bootstrap object for joining the DHT network

    std::function<void(const types::InfoHash&)> on_infohash_discovered_; ///< Callback for infohash discovery
};

} // namespace bitscrape::dht
