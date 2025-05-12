#include "bitscrape/dht/dht_session.hpp"

#include <random>
#include <chrono>
#include <future>

namespace bitscrape::dht {

DHTSession::DHTSession()
    : node_id_(types::NodeID()),
      port_(6881),
      event_bus_(nullptr),
      running_(false) {
    // Create the socket
    socket_ = std::make_unique<network::UDPSocket>();

    // Create the routing table
    routing_table_ = std::make_unique<RoutingTable>(node_id_);

    // Create the message factory
    message_factory_ = std::make_unique<DHTMessageFactory>();

    // Create the token manager
    token_manager_ = std::make_unique<TokenManager>();
}

DHTSession::DHTSession(const types::NodeID& node_id)
    : node_id_(node_id),
      port_(6881),
      event_bus_(nullptr),
      running_(false) {
    // Create the socket
    socket_ = std::make_unique<network::UDPSocket>();

    // Create the routing table
    routing_table_ = std::make_unique<RoutingTable>(node_id_);

    // Create the message factory
    message_factory_ = std::make_unique<DHTMessageFactory>();

    // Create the token manager
    token_manager_ = std::make_unique<TokenManager>();
}

DHTSession::DHTSession(const types::NodeID& node_id, uint16_t port, event::EventBus& event_bus)
    : node_id_(node_id),
      port_(port),
      event_bus_(&event_bus),
      running_(false) {
    // Create the socket
    socket_ = std::make_unique<network::UDPSocket>();

    // Create the routing table
    routing_table_ = std::make_unique<RoutingTable>(node_id_);

    // Create the message factory
    message_factory_ = std::make_unique<DHTMessageFactory>();

    // Create the token manager
    token_manager_ = std::make_unique<TokenManager>();
}

DHTSession::~DHTSession() {
    // Stop the session if it's running
    if (running_) {
        stop();
    }
}

bool DHTSession::start(const std::vector<types::Endpoint>& bootstrap_nodes) {
    // Check if the session is already running
    if (running_) {
        return false;
    }

    // Bind the socket to the port
    if (!socket_->bind(port_)) {
        return false;
    }

    // Start the receive loop
    running_ = true;
    start_receive_loop();

    // Bootstrap the DHT
    Bootstrap bootstrap(node_id_, *routing_table_, *socket_, *message_factory_);
    return bootstrap.start(bootstrap_nodes);
}

std::future<bool> DHTSession::start_async(const std::vector<types::Endpoint>& bootstrap_nodes) {
    return std::async(std::launch::async, [this, bootstrap_nodes]() {
        return this->start(bootstrap_nodes);
    });
}

void DHTSession::stop() {
    // Check if the session is running
    if (!running_) {
        return;
    }

    // Stop the receive loop
    running_ = false;

    // Close the socket
    socket_->close();

    // Wait for the receive thread to finish
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
}

std::vector<types::DHTNode> DHTSession::find_nodes(const types::NodeID& target_id) {
    // Create a node lookup
    NodeLookup lookup(node_id_, target_id, *routing_table_, *socket_, *message_factory_);

    // Start the lookup
    return lookup.start();
}

std::future<std::vector<types::DHTNode>> DHTSession::find_nodes_async(const types::NodeID& target_id) {
    return std::async(std::launch::async, [this, target_id]() {
        return this->find_nodes(target_id);
    });
}

std::vector<types::Endpoint> DHTSession::find_peers(const types::InfoHash& infohash) {
    // Check if we already have peers for this infohash
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        auto it = peers_.find(infohash);
        if (it != peers_.end() && !it->second.empty()) {
            return it->second;
        }
    }

    // Convert the infohash to a node ID for the lookup
    types::NodeID target_id(std::vector<uint8_t>(infohash.bytes().begin(), infohash.bytes().end()));

    // Find nodes close to the infohash
    auto nodes = find_nodes(target_id);

    // TODO: Send get_peers messages to the found nodes

    // Return the peers
    std::lock_guard<std::mutex> lock(peers_mutex_);
    return peers_[infohash];
}

std::future<std::vector<types::Endpoint>> DHTSession::find_peers_async(const types::InfoHash& infohash) {
    return std::async(std::launch::async, [this, infohash]() {
        return this->find_peers(infohash);
    });
}

bool DHTSession::announce_peer(const types::InfoHash& infohash, uint16_t port) {
    // Convert the infohash to a node ID for the lookup
    types::NodeID target_id(std::vector<uint8_t>(infohash.bytes().begin(), infohash.bytes().end()));

    // Find nodes close to the infohash
    auto nodes = find_nodes(target_id);

    // TODO: Send announce_peer messages to the found nodes

    return true;
}

std::future<bool> DHTSession::announce_peer_async(const types::InfoHash& infohash, uint16_t port) {
    return std::async(std::launch::async, [this, infohash, port]() {
        return this->announce_peer(infohash, port);
    });
}

const types::NodeID& DHTSession::node_id() const {
    return node_id_;
}

const RoutingTable& DHTSession::routing_table() const {
    return *routing_table_;
}

bool DHTSession::is_running() const {
    return running_;
}

void DHTSession::process_message(const std::vector<uint8_t>& data, const types::Endpoint& sender_endpoint) {
    // Parse the message
    auto message = message_factory_->create_from_data(data);
    if (!message) {
        return;
    }

    // Add the sender to the routing table if it's a valid node
    if (message->type() == DHTMessage::Type::PING ||
        message->type() == DHTMessage::Type::FIND_NODE ||
        message->type() == DHTMessage::Type::GET_PEERS ||
        message->type() == DHTMessage::Type::ANNOUNCE_PEER) {
        // TODO: Extract the node ID from the message and add it to the routing table
    }

    // Check if this is a response to an active lookup
    if (message->type() == DHTMessage::Type::PING_RESPONSE ||
        message->type() == DHTMessage::Type::FIND_NODE_RESPONSE ||
        message->type() == DHTMessage::Type::GET_PEERS_RESPONSE ||
        message->type() == DHTMessage::Type::ANNOUNCE_PEER_RESPONSE) {
        std::lock_guard<std::mutex> lock(lookups_mutex_);

        // Find the lookup with the matching transaction ID
        auto it = lookups_.find(message->transaction_id());
        if (it != lookups_.end()) {
            // Process the response
            it->second->process_response(message, sender_endpoint);
            return;
        }
    }

    // Handle the message based on its type
    switch (message->type()) {
        case DHTMessage::Type::PING:
            handle_ping(message, sender_endpoint);
            break;
        case DHTMessage::Type::FIND_NODE:
            handle_find_node(message, sender_endpoint);
            break;
        case DHTMessage::Type::GET_PEERS:
            handle_get_peers(message, sender_endpoint);
            break;
        case DHTMessage::Type::ANNOUNCE_PEER:
            handle_announce_peer(message, sender_endpoint);
            break;
        default:
            // Ignore other message types
            break;
    }
}

void DHTSession::start_receive_loop() {
    // Start the receive thread
    receive_thread_ = std::thread([this]() {
        while (running_) {
            try {
                // Receive a message
                network::Buffer buffer;
                network::Address address;
                int bytes_received = socket_->receive_from(buffer, address);

                if (bytes_received <= 0) {
                    continue; // No data received or error
                }

                // Convert to vector<uint8_t> and Endpoint
                std::vector<uint8_t> data(buffer.data(), buffer.data() + buffer.size());
                types::Endpoint sender_endpoint(address.to_string(), address.port());

                // Process the message
                process_message(data, sender_endpoint);
            } catch (const std::exception& e) {
                // Ignore exceptions
            }
        }
    });
}

void DHTSession::handle_ping(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint) {
    // Create a ping response
    auto response = message_factory_->create_ping_response(message->transaction_id(), node_id_);

    // Encode the response
    auto data = response->encode();

    // Send the response
    network::Address address(sender_endpoint.address(), sender_endpoint.port());
    socket_->send_to(data.data(), data.size(), address);
}

void DHTSession::handle_find_node(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint) {
    // TODO: Implement find_node handling
}

void DHTSession::handle_get_peers(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint) {
    // TODO: Implement get_peers handling
}

void DHTSession::handle_announce_peer(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint) {
    // TODO: Implement announce_peer handling
}

} // namespace bitscrape::dht
