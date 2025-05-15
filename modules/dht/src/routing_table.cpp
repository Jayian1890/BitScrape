#include "bitscrape/dht/routing_table.hpp"

#include <algorithm>
#include <future>
#include <sstream>

namespace bitscrape::dht {

RoutingTable::RoutingTable(const types::NodeID& local_id, std::shared_ptr<lock::LockManager> lock_manager)
    : local_id_(local_id),
      lock_manager_(lock_manager),
      resource_id_(lock_manager->register_resource(get_resource_name(), lock::LockManager::LockPriority::HIGH)) {
    // Create the first bucket (covers the entire ID space)
    buckets_.emplace_back(0, *lock_manager);
}

std::string RoutingTable::get_resource_name() const {
    std::stringstream ss;
    ss << "dht.routing_table." << local_id_.to_hex().substr(0, 8);
    return ss.str();
}

bool RoutingTable::add_node(const types::DHTNode& node) {
    // Don't add the local node
    if (node.id() == local_id_) {
        return false;
    }

    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

    // Get the appropriate bucket for the node
    KBucket& bucket = get_or_create_bucket(node.id());

    // Add the node to the bucket
    return bucket.add_node(node);
}

std::future<bool> RoutingTable::add_node_async(const types::DHTNode& node) {
    return std::async(std::launch::async, [this, node]() {
        return this->add_node(node);
    });
}

bool RoutingTable::remove_node(const types::NodeID& node_id) {
    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

    // Get the bucket index for the node
    size_t index = get_bucket_index(node_id);

    // If the bucket exists, remove the node
    if (index < buckets_.size()) {
        return buckets_[index].remove_node(node_id);
    }

    return false;
}

std::future<bool> RoutingTable::remove_node_async(const types::NodeID& node_id) {
    return std::async(std::launch::async, [this, node_id]() {
        return this->remove_node(node_id);
    });
}

bool RoutingTable::update_node(const types::DHTNode& node) {
    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

    // Get the bucket index for the node
    size_t index = get_bucket_index(node.id());

    // If the bucket exists, update the node
    if (index < buckets_.size()) {
        return buckets_[index].update_node(node);
    }

    return false;
}

std::future<bool> RoutingTable::update_node_async(const types::DHTNode& node) {
    return std::async(std::launch::async, [this, node]() {
        return this->update_node(node);
    });
}

std::optional<types::DHTNode> RoutingTable::get_node(const types::NodeID& node_id) const {
    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    // Get the bucket index for the node
    size_t index = get_bucket_index(node_id);

    // If the bucket exists, get the node
    if (index < buckets_.size()) {
        return buckets_[index].get_node(node_id);
    }

    return std::nullopt;
}

std::future<std::optional<types::DHTNode>> RoutingTable::get_node_async(const types::NodeID& node_id) const {
    return std::async(std::launch::async, [this, node_id]() {
        return this->get_node(node_id);
    });
}

bool RoutingTable::contains_node(const types::NodeID& node_id) const {
    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    // Get the bucket index for the node
    size_t index = get_bucket_index(node_id);

    // If the bucket exists, check if it contains the node
    if (index < buckets_.size()) {
        return buckets_[index].contains_node(node_id);
    }

    return false;
}

std::future<bool> RoutingTable::contains_node_async(const types::NodeID& node_id) const {
    return std::async(std::launch::async, [this, node_id]() {
        return this->contains_node(node_id);
    });
}

std::vector<types::DHTNode> RoutingTable::get_closest_nodes(const types::NodeID& target_id, size_t k) const {
    // First get all nodes with the lock held
    std::vector<types::DHTNode> all_nodes;
    {
        auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);
        all_nodes = get_all_nodes_internal();
    }

    // Sort nodes by distance to the target ID (outside the lock)
    std::sort(all_nodes.begin(), all_nodes.end(), [&target_id](const types::DHTNode& a, const types::DHTNode& b) {
        return a.id().distance(target_id) < b.id().distance(target_id);
    });

    // Return the k closest nodes
    if (all_nodes.size() > k) {
        all_nodes.resize(k);
    }

    return all_nodes;
}

std::future<std::vector<types::DHTNode>> RoutingTable::get_closest_nodes_async(const types::NodeID& target_id, size_t k) const {
    return std::async(std::launch::async, [this, target_id, k]() {
        return this->get_closest_nodes(target_id, k);
    });
}

std::vector<types::DHTNode> RoutingTable::get_all_nodes() const {
    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);
    return get_all_nodes_internal();
}

// Internal method to get all nodes without acquiring the lock
// Caller must hold the mutex_
std::vector<types::DHTNode> RoutingTable::get_all_nodes_internal() const {
    std::vector<types::DHTNode> all_nodes;

    // Reserve space for all nodes
    size_t total_nodes = 0;
    for (const auto& bucket : buckets_) {
        total_nodes += bucket.size();
    }
    all_nodes.reserve(total_nodes);

    // Add all nodes from all buckets
    for (const auto& bucket : buckets_) {
        auto nodes = bucket.get_nodes();
        all_nodes.insert(all_nodes.end(), nodes.begin(), nodes.end());
    }

    return all_nodes;
}

std::future<std::vector<types::DHTNode>> RoutingTable::get_all_nodes_async() const {
    return std::async(std::launch::async, [this]() {
        return this->get_all_nodes();
    });
}

size_t RoutingTable::size() const {
    auto guard = lock_manager_->get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    size_t total_size = 0;
    for (const auto& bucket : buckets_) {
        total_size += bucket.size();
    }

    return total_size;
}

const types::NodeID& RoutingTable::local_id() const {
    return local_id_;
}

size_t RoutingTable::get_bucket_index(const types::NodeID& node_id) const {
    // Calculate the distance between the local ID and the node ID
    types::NodeID distance = local_id_.distance(node_id);

    // Find the index of the first set bit (from the left)
    // This gives us the length of the common prefix
    size_t prefix_length = 0;
    for (size_t i = 0; i < types::NodeID::SIZE * 8; ++i) {
        if (distance.is_bit_set(i)) {
            prefix_length = i;
            break;
        }
    }

    // The bucket index is the prefix length
    return prefix_length;
}

KBucket& RoutingTable::get_or_create_bucket(const types::NodeID& node_id) {
    // Get the bucket index for the node
    size_t index = get_bucket_index(node_id);

    // If the bucket doesn't exist, create it and all buckets before it
    while (index >= buckets_.size()) {
        buckets_.emplace_back(buckets_.size(), *lock_manager_);
    }

    return buckets_[index];
}

} // namespace bitscrape::dht
