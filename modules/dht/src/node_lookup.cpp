#include "bitscrape/dht/node_lookup.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include "bitscrape/dht/dht_session.hpp"

namespace bitscrape::dht {

NodeLookup::NodeLookup(const types::NodeID& local_id,
                       const types::NodeID& target_id,
                       const RoutingTable& routing_table,
                       network::UDPSocket& socket,
                       DHTMessageFactory& message_factory,
                       DHTSession& session,
                       QueryType query_type,
                       std::shared_ptr<lock::LockManager> lock_manager)
    : local_id_(local_id),
      target_id_(target_id),
      routing_table_(routing_table),
      socket_(socket),
      message_factory_(message_factory),
      session_(session),
      query_type_(query_type),
      active_queries_(0),
      complete_(false),
      lock_manager_(lock_manager),
      resource_id_(lock_manager->register_resource(get_resource_name(), lock::LockManager::LockPriority::NORMAL)) {
}

void NodeLookup::set_peer_callback(std::function<void(const types::InfoHash&, const types::Endpoint&)> callback) {
    on_peer_discovered_ = std::move(callback);
}

std::string NodeLookup::get_resource_name() const {
    std::stringstream ss;
    ss << "dht.node_lookup." << target_id_.to_hex().substr(0, 8);
    return ss.str();
}

NodeLookup::~NodeLookup() {
    // Ensure any waiting threads are notified
    std::lock_guard<std::mutex> lock(cv_mutex_);
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

    // Wait for the lookup to complete (5 second timeout)
    wait_for_completion(5000);

    // Return the k closest nodes
    return get_closest_nodes();
}

std::future<std::vector<types::DHTNode>> NodeLookup::start_async() {
    return std::async(std::launch::async, [this]() {
        return this->start();
    });
}

void NodeLookup::process_response(const std::shared_ptr<DHTMessage>& response, const types::Endpoint& sender_endpoint) {
    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

    // Find the node that sent the response
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [&sender_endpoint](const NodeEntry& entry) {
        return entry.node.endpoint() == sender_endpoint;
    });

    if (it != nodes_.end()) {
        // Calculate RTT
        auto now = std::chrono::steady_clock::now();
        (void)std::chrono::duration_cast<std::chrono::milliseconds>(now - it->sent_time).count();

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
        } else {
            // Also check for get_peers response
            auto get_peers_response = std::dynamic_pointer_cast<DHTGetPeersMessage>(response);
            if (get_peers_response) {
                // Get the nodes from the response
                new_nodes = get_peers_response->nodes();
                
                // Extract peers
                auto peers = get_peers_response->values();
                if (!peers.empty() && on_peer_discovered_) {
                    // target_id_ is the infohash for get_peers
                    types::InfoHash info_hash(std::vector<uint8_t>(target_id_.bytes().begin(), target_id_.bytes().end()));
                    for (const auto& peer : peers) {
                        on_peer_discovered_(info_hash, peer);
                    }
                }
            }
        }

        // Add the new nodes to the lookup process
        add_nodes(new_nodes);

        // Check if we need to send more queries
        if (!complete_.load() && !has_converged()) {
            send_queries_internal();
        } else if (active_queries_ == 0) {
            // No more active queries and we've converged, so we're done
            std::lock_guard<std::mutex> lock(cv_mutex_);
            complete_.store(true);
            cv_.notify_all();
        }
    }
}

bool NodeLookup::is_complete() const {
    return complete_.load();
}

bool NodeLookup::wait_for_completion(int timeout_ms) {
    // Lock the condition variable mutex
    std::unique_lock<std::mutex> lock(cv_mutex_);

    // Note: We intentionally do NOT hold the resource lock while waiting
    // to allow process_response to run and update completion status.

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
    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);
    send_queries_internal();
}

void NodeLookup::send_queries_internal() {
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
        std::lock_guard<std::mutex> lock(cv_mutex_);
        complete_.store(true);
        cv_.notify_all();
    }
}

bool NodeLookup::send_query(const types::DHTNode& node) {
    // Find the node in our list
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [&node](const NodeEntry& entry) {
        return entry.node.id() == node.id();
    });

    if (it != nodes_.end() && it->state == NodeState::UNKNOWN) {
        // Create a transaction ID
        auto transaction_id = DHTMessageFactory::generate_transaction_id();
        
        // Create the message based on the query type
        std::shared_ptr<DHTMessage> message;
        if (query_type_ == QueryType::GET_PEERS) {
            types::InfoHash info_hash(std::vector<uint8_t>(target_id_.bytes().begin(), target_id_.bytes().end()));
            message = message_factory_.create_get_peers(transaction_id, local_id_, info_hash);
        } else {
            message = message_factory_.create_find_node(transaction_id, local_id_, target_id_);
        }

        // Encode the message
        auto data = message->encode();

        // Send the message
        network::Address address(node.endpoint().address(), node.endpoint().port());
        socket_.send_to(data.data(), data.size(), address);

        // Register the transaction with the session
        session_.register_transaction(transaction_id, shared_from_this());

        // Update the node state
        it->state = NodeState::QUERIED;
        it->sent_time = std::chrono::steady_clock::now();

        // Schedule a timeout
        std::thread([this, node, transaction_id]() {
            // Sleep for the timeout duration
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));

            auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

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
                // Check if we need to send more queries
                if (!complete_.load() && !has_converged()) {
                    send_queries_internal();
                }

                if (active_queries_ == 0) {
                    // No more active queries and we've converged, so we're done
                    std::lock_guard<std::mutex> lock(cv_mutex_);
                    complete_.store(true);
                    cv_.notify_all();
                }
            }
            
            // Clean up the transaction
            session_.unregister_transaction(transaction_id);
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

    // Validate the endpoint - reject invalid/empty endpoints
    const auto& endpoint = node.endpoint();
    if (!endpoint.is_valid() || endpoint.address().empty() || 
        endpoint.address() == "0.0.0.0" || endpoint.port() == 0) {
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
    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

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
