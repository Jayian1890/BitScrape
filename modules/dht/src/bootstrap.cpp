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
    // Check if we have any bootstrap nodes
    if (bootstrap_nodes.empty()) {
        // No bootstrap nodes provided, just return true
        // In a real implementation, we would use a hardcoded list of bootstrap nodes
        return true;
    }

    // Contact each bootstrap node
    for (const auto& endpoint : bootstrap_nodes) {
        contact_bootstrap_node(endpoint);
    }

    // Perform random lookups to populate the routing table
    for (size_t i = 0; i < RANDOM_LOOKUPS; ++i) {
        perform_random_lookup();
    }

    // Wait for all lookups to complete with a 10-second timeout
    bool completed = wait_for_completion(10000);

    // Return true if we have at least one node in the routing table or if we completed successfully
    return routing_table_.size() > 0 || completed;
}

std::future<bool> Bootstrap::start_async(const std::vector<types::Endpoint>& bootstrap_nodes) {
    return std::async(std::launch::async, [this, bootstrap_nodes]() {
        return this->start(bootstrap_nodes);
    });
}

void Bootstrap::process_message(const std::shared_ptr<DHTMessage>& message, const types::Endpoint& sender_endpoint) {
    // Process the message based on its type
    if (message->type() == DHTMessage::Type::PING_RESPONSE) {
        // Check if this is a response to one of our pings
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pending_pings_.find(message->transaction_id());
        if (it == pending_pings_.end()) {
            // Not one of our pings
            return;
        }

        // Remove the pending ping
        pending_pings_.erase(it);

        // Extract the node ID from the message
        auto ping_response = std::dynamic_pointer_cast<DHTPingMessage>(message);
        if (!ping_response) {
            return;
        }

        // Get the node ID from the response
        types::NodeID node_id = ping_response->node_id();

        // Create a node and add it to the routing table
        types::DHTNode node(node_id, sender_endpoint);
        bool added = routing_table_.add_node(node);

        // Decrement the active lookups counter
        if (active_lookups_ > 0) {
            --active_lookups_;
        }

        // Check if we're done
        if (active_lookups_ == 0) {
            complete_.store(true);
            cv_.notify_all();
        }
    }
    // Other message types are handled by the node lookup
}

bool Bootstrap::is_complete() const {
    return complete_.load();
}

bool Bootstrap::wait_for_completion(int timeout_ms) {
    // Check if already complete
    if (complete_.load()) {
        return true;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    if (timeout_ms > 0) {
        // Wait with timeout
        bool completed = cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() {
            return complete_.load();
        });

        // If we timed out but have at least one node in the routing table, consider it a success
        if (!completed && routing_table_.size() > 0) {
            complete_.store(true);
            return true;
        }

        return completed;
    } else {
        // Use a default timeout of 10 seconds instead of waiting indefinitely
        bool completed = cv_.wait_for(lock, std::chrono::seconds(10), [this]() {
            return complete_.load();
        });

        // If we timed out but have at least one node in the routing table, consider it a success
        if (!completed && routing_table_.size() > 0) {
            complete_.store(true);
            return true;
        }

        return completed;
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
    if (!socket_.send_to(data.data(), data.size(), address)) {
        return false;
    }

    // Store the transaction ID and endpoint
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_pings_[transaction_id] = endpoint;

        // Increment the active lookups counter
        ++active_lookups_;
    }

    // Schedule a timeout
    std::thread([this, transaction_id]() {
        // Sleep for a timeout duration
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        std::lock_guard<std::mutex> lock(mutex_);

        // Remove the pending ping
        pending_pings_.erase(transaction_id);

        // Decrement the active lookups counter
        if (active_lookups_ > 0) {
            --active_lookups_;
        }

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

    // Increment the active lookups counter
    ++active_lookups_;

    // Create a node lookup on the heap so it persists after this function returns
    auto lookup_ptr = std::make_shared<NodeLookup>(local_id_, target_id, routing_table_, socket_, message_factory_);

    // Start the lookup in a separate thread
    std::thread([this, lookup_ptr]() {
        // Start the lookup
        auto nodes = lookup_ptr->start();

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
