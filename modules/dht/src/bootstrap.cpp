#include "bitscrape/dht/bootstrap.hpp"

#include <random>
#include <chrono>
#include <future>

namespace bitscrape::dht {

Bootstrap::Bootstrap(const types::NodeID& local_id,
                     RoutingTable& routing_table,
                     network::UDPSocket& socket,
                     DHTMessageFactory& message_factory)
    : local_id_(local_id),
      routing_table_(routing_table),
      socket_(socket),
      message_factory_(message_factory),
      active_lookups_(0),
      complete_(false) {
}

bool Bootstrap::start(const std::vector<types::Endpoint>& bootstrap_nodes) {
    // Contact each bootstrap node
    for (const auto& endpoint : bootstrap_nodes) {
        contact_bootstrap_node(endpoint);
    }

    // Perform random lookups to populate the routing table
    for (size_t i = 0; i < RANDOM_LOOKUPS; ++i) {
        perform_random_lookup();
    }

    // Wait for all lookups to complete
    wait_for_completion();

    // Return true if we have at least one node in the routing table
    return routing_table_.size() > 0;
}

std::future<bool> Bootstrap::start_async(const std::vector<types::Endpoint>& bootstrap_nodes) {
    return std::async(std::launch::async, [this, bootstrap_nodes]() {
        return this->start(bootstrap_nodes);
    });
}

void Bootstrap::process_message(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint) {
    // Process the message based on its type
    if (message->type() == DHTMessage::Type::PING_RESPONSE) {
        // Add the node to the routing table
        types::NodeID node_id;

        // Extract the node ID from the message
        // TODO: Implement this when ping response is implemented

        // Create a node and add it to the routing table
        types::DHTNode node(node_id, sender_endpoint);
        routing_table_.add_node(node);
    }
    // Other message types are handled by the node lookup
}

bool Bootstrap::is_complete() const {
    return complete_.load();
}

bool Bootstrap::wait_for_completion(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (timeout_ms > 0) {
        // Wait with timeout
        return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() {
            return complete_.load();
        });
    } else {
        // Wait indefinitely
        cv_.wait(lock, [this]() {
            return complete_.load();
        });
        return true;
    }
}

bool Bootstrap::contact_bootstrap_node(const types::Endpoint& endpoint) {
    // Create a ping message
    auto transaction_id = DHTMessageFactory::generate_transaction_id();
    auto message = message_factory_.create_ping(transaction_id, local_id_);

    // Encode the message
    auto data = message->encode();

    // Send the message
    network::Address address(endpoint.address(), endpoint.port());
    socket_.send_to(data.data(), data.size(), address);

    // Increment the active lookups counter
    ++active_lookups_;

    // Schedule a timeout
    std::thread([this, endpoint]() {
        // Sleep for a timeout duration
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        std::lock_guard<std::mutex> lock(mutex_);

        // Decrement the active lookups counter
        --active_lookups_;

        // Check if we're done
        if (active_lookups_ == 0) {
            complete_.store(true);
            cv_.notify_all();
        }
    }).detach();

    return true;
}

bool Bootstrap::perform_random_lookup() {
    // Generate a random node ID
    types::NodeID target_id = generate_random_node_id();

    // Create a node lookup
    NodeLookup lookup(local_id_, target_id, routing_table_, socket_, message_factory_);

    // Increment the active lookups counter
    ++active_lookups_;

    // Start the lookup in a separate thread
    std::thread([this, &lookup]() {
        // Start the lookup
        auto nodes = lookup.start();

        // Add the found nodes to the routing table
        for (const auto& node : nodes) {
            routing_table_.add_node(node);
        }

        std::lock_guard<std::mutex> lock(mutex_);

        // Decrement the active lookups counter
        --active_lookups_;

        // Check if we're done
        if (active_lookups_ == 0) {
            complete_.store(true);
            cv_.notify_all();
        }
    }).detach();

    return true;
}

types::NodeID Bootstrap::generate_random_node_id() const {
    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);

    // Generate random bytes for the node ID
    std::vector<uint8_t> bytes;
    bytes.reserve(types::NodeID::SIZE);

    for (size_t i = 0; i < types::NodeID::SIZE; ++i) {
        bytes.push_back(dis(gen));
    }

    // Create a node ID from the random bytes
    return types::NodeID(bytes);
}

} // namespace bitscrape::dht
