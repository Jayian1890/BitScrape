#include "bitscrape/dht/node_lookup.hpp"

#include <algorithm>
#include <chrono>
#include <future>

namespace bitscrape::dht {

NodeLookup::NodeLookup(const types::NodeID& local_id,
                       const types::NodeID& target_id,
                       const RoutingTable& routing_table,
                       network::UDPSocket& socket,
                       DHTMessageFactory& message_factory)
    : local_id_(local_id),
      target_id_(target_id),
      routing_table_(routing_table),
      socket_(socket),
      message_factory_(message_factory),
      active_queries_(0),
      complete_(false) {
}

NodeLookup::~NodeLookup() {
    // Ensure any waiting threads are notified
    complete_.store(true);
    cv_.notify_all();
}

std::vector<types::DHTNode> NodeLookup::start() {
    // Get the initial nodes from the routing table
    auto initial_nodes = routing_table_.get_closest_nodes(target_id_, K);

    // Add the initial nodes to the lookup process
    add_nodes(initial_nodes);

    // Start sending queries
    send_queries();

    // Wait for the lookup to complete
    wait_for_completion();

    // Return the k closest nodes
    return get_closest_nodes();
}

std::future<std::vector<types::DHTNode>> NodeLookup::start_async() {
    return std::async(std::launch::async, [this]() {
        return this->start();
    });
}

void NodeLookup::process_response(const std::shared_ptr<DHTMessage>& response, const types::Endpoint& sender_endpoint) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find the node that sent the response
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [&sender_endpoint](const NodeEntry& entry) {
        return entry.node.endpoint() == sender_endpoint;
    });

    if (it != nodes_.end()) {
        // Update the node state
        it->state = NodeState::RESPONDED;

        // Decrement the active queries counter
        --active_queries_;

        // Extract the nodes from the response
        std::vector<types::DHTNode> new_nodes;

        // Check if the response is a find_node response
        auto find_node_response = std::dynamic_pointer_cast<DHTFindNodeMessage>(response);
        if (find_node_response) {
            // Get the nodes from the response
            new_nodes = find_node_response->nodes();
        }

        // Add the new nodes to the lookup process
        add_nodes(new_nodes);

        // Check if we need to send more queries
        if (!complete_ && !has_converged()) {
            send_queries();
        } else if (active_queries_ == 0) {
            // No more active queries and we've converged, so we're done
            complete_ = true;
            cv_.notify_all();
        }
    }
}

bool NodeLookup::is_complete() const {
    return complete_.load();
}

bool NodeLookup::wait_for_completion(int timeout_ms) {
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

void NodeLookup::send_queries() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Get the closest unqueried nodes
    auto nodes_to_query = get_closest_unqueried_nodes(ALPHA - active_queries_);

    // Send queries to these nodes
    for (const auto& node : nodes_to_query) {
        if (send_query(node)) {
            ++active_queries_;
        }
    }

    // If there are no active queries and no more nodes to query, we're done
    if (active_queries_ == 0) {
        complete_ = true;
        cv_.notify_all();
    }
}

bool NodeLookup::send_query(const types::DHTNode& node) {
    // Find the node in our list
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [&node](const NodeEntry& entry) {
        return entry.node.id() == node.id();
    });

    if (it != nodes_.end() && it->state == NodeState::UNKNOWN) {
        // Create a find_node message
        auto transaction_id = DHTMessageFactory::generate_transaction_id();
        auto message = message_factory_.create_find_node(transaction_id, local_id_, target_id_);

        // Encode the message
        auto data = message->encode();

        // Send the message
        network::Address address(node.endpoint().address(), node.endpoint().port());
        socket_.send_to(data.data(), data.size(), address);

        // Update the node state
        it->state = NodeState::QUERIED;

        // Schedule a timeout
        std::thread([this, node]() {
            // Sleep for the timeout duration
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));

            std::lock_guard<std::mutex> lock(mutex_);

            // Find the node again (it might have been removed)
            auto it = std::find_if(nodes_.begin(), nodes_.end(), [&node](const NodeEntry& entry) {
                return entry.node.id() == node.id();
            });

            if (it != nodes_.end() && it->state == NodeState::QUERIED) {
                // Node has not responded within the timeout
                ++it->timeouts;

                if (it->timeouts >= MAX_TIMEOUTS) {
                    // Node has failed too many times, mark it as failed
                    it->state = NodeState::FAILED;
                } else {
                    // Reset the state so we can try again
                    it->state = NodeState::UNKNOWN;
                }

                // Decrement the active queries counter
                --active_queries_;

                // Check if we need to send more queries
                if (!complete_.load() && !has_converged()) {
                    send_queries();
                } else if (active_queries_ == 0) {
                    // No more active queries and we've converged, so we're done
                    complete_.store(true);
                    cv_.notify_all();
                }
            }
        }).detach();

        return true;
    }

    return false;
}

void NodeLookup::add_node(const types::DHTNode& node) {
    // Don't add the local node
    if (node.id() == local_id_) {
        return;
    }

    // Check if the node is already in the list
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [&node](const NodeEntry& entry) {
        return entry.node.id() == node.id();
    });

    if (it == nodes_.end()) {
        // Add the node to the list
        nodes_.emplace_back(node);

        // Sort the nodes by distance to the target
        std::sort(nodes_.begin(), nodes_.end(), [this](const NodeEntry& a, const NodeEntry& b) {
            return NodeEntry::compare_distance(target_id_, a, b);
        });
    }
}

void NodeLookup::add_nodes(const std::vector<types::DHTNode>& nodes) {
    for (const auto& node : nodes) {
        add_node(node);
    }
}

std::vector<types::DHTNode> NodeLookup::get_closest_unqueried_nodes(size_t count) {
    std::vector<types::DHTNode> result;

    // Find the closest unqueried nodes
    for (const auto& entry : nodes_) {
        if (entry.state == NodeState::UNKNOWN && entry.timeouts < MAX_TIMEOUTS) {
            result.push_back(entry.node);

            if (result.size() >= count) {
                break;
            }
        }
    }

    return result;
}

bool NodeLookup::has_converged() const {
    // Get the k closest nodes
    std::vector<NodeEntry> closest;
    for (const auto& entry : nodes_) {
        if (entry.state == NodeState::RESPONDED) {
            closest.push_back(entry);

            if (closest.size() >= K) {
                break;
            }
        }
    }

    // If we don't have k nodes that have responded, we haven't converged
    if (closest.size() < K) {
        return false;
    }

    // Check if there are any unqueried nodes closer than the furthest of the k closest
    const auto& furthest = closest.back();

    for (const auto& entry : nodes_) {
        if (entry.state == NodeState::UNKNOWN &&
            NodeEntry::compare_distance(target_id_, entry, furthest)) {
            return false;
        }
    }

    return true;
}

std::vector<types::DHTNode> NodeLookup::get_closest_nodes() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<types::DHTNode> result;

    // Get the k closest nodes that have responded
    for (const auto& entry : nodes_) {
        if (entry.state == NodeState::RESPONDED) {
            result.push_back(entry.node);

            if (result.size() >= K) {
                break;
            }
        }
    }

    // If we don't have k nodes that have responded, include nodes that we haven't queried yet
    if (result.size() < K) {
        for (const auto& entry : nodes_) {
            if (entry.state == NodeState::UNKNOWN &&
                std::find_if(result.begin(), result.end(), [&entry](const types::DHTNode& node) {
                    return node.id() == entry.node.id();
                }) == result.end()) {
                result.push_back(entry.node);

                if (result.size() >= K) {
                    break;
                }
            }
        }
    }

    return result;
}

} // namespace bitscrape::dht
