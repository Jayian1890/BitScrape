#include "bitscrape/dht/k_bucket.hpp"

#include <algorithm>
#include <future>
#include <sstream>

namespace bitscrape::dht {

KBucket::KBucket(uint8_t prefix_length, lock::LockManager& lock_manager)
    : prefix_length_(prefix_length),
      last_updated_(std::chrono::system_clock::now()),
      lock_manager_(lock_manager),
      resource_id_(lock_manager.register_resource(get_resource_name(), lock::LockManager::LockPriority::NORMAL)) {
    // Reserve space for k nodes
    nodes_.reserve(K);
}

KBucket::KBucket(uint8_t prefix_length, std::shared_ptr<lock::LockManager> lock_manager)
    : prefix_length_(prefix_length),
      last_updated_(std::chrono::system_clock::now()),
      lock_manager_(*lock_manager),
      resource_id_(lock_manager->register_resource(get_resource_name(), lock::LockManager::LockPriority::NORMAL)) {
    // Reserve space for k nodes
    nodes_.reserve(K);
}

std::string KBucket::get_resource_name() const {
    std::stringstream ss;
    ss << "dht.k_bucket." << static_cast<int>(prefix_length_);
    return ss.str();
}

bool KBucket::add_node(const types::DHTNode& node) {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

    // Check if the bucket is full
    if (is_full()) {
        return false;
    }

    // Check if the node is already in the bucket
    if (contains_node(node.id())) {
        return false;
    }

    // Add the node to the bucket
    nodes_.push_back(node);
    last_updated_ = std::chrono::system_clock::now();

    return true;
}

std::future<bool> KBucket::add_node_async(const types::DHTNode& node) {
    return std::async(std::launch::async, [this, node]() {
        return this->add_node(node);
    });
}

bool KBucket::remove_node(const types::NodeID& node_id) {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

    // Find the node
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [&node_id](const types::DHTNode& node) {
        return node.id() == node_id;
    });

    // If the node was found, remove it
    if (it != nodes_.end()) {
        nodes_.erase(it);
        last_updated_ = std::chrono::system_clock::now();
        return true;
    }

    return false;
}

std::future<bool> KBucket::remove_node_async(const types::NodeID& node_id) {
    return std::async(std::launch::async, [this, node_id]() {
        return this->remove_node(node_id);
    });
}

bool KBucket::update_node(const types::DHTNode& node) {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

    // Find the node
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [&node](const types::DHTNode& n) {
        return n.id() == node.id();
    });

    // If the node was found, update it
    if (it != nodes_.end()) {
        *it = node;
        last_updated_ = std::chrono::system_clock::now();
        return true;
    }

    return false;
}

std::future<bool> KBucket::update_node_async(const types::DHTNode& node) {
    return std::async(std::launch::async, [this, node]() {
        return this->update_node(node);
    });
}

std::optional<types::DHTNode> KBucket::get_node(const types::NodeID& node_id) const {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    // Find the node
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [&node_id](const types::DHTNode& node) {
        return node.id() == node_id;
    });

    // If the node was found, return it
    if (it != nodes_.end()) {
        return *it;
    }

    return std::nullopt;
}

std::future<std::optional<types::DHTNode>> KBucket::get_node_async(const types::NodeID& node_id) const {
    return std::async(std::launch::async, [this, node_id]() {
        return this->get_node(node_id);
    });
}

bool KBucket::contains_node(const types::NodeID& node_id) const {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    // Find the node
    auto it = std::find_if(nodes_.begin(), nodes_.end(), [&node_id](const types::DHTNode& node) {
        return node.id() == node_id;
    });

    return it != nodes_.end();
}

std::future<bool> KBucket::contains_node_async(const types::NodeID& node_id) const {
    return std::async(std::launch::async, [this, node_id]() {
        return this->contains_node(node_id);
    });
}

std::vector<types::DHTNode> KBucket::get_nodes() const {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    return nodes_;
}

std::future<std::vector<types::DHTNode>> KBucket::get_nodes_async() const {
    return std::async(std::launch::async, [this]() {
        return this->get_nodes();
    });
}

size_t KBucket::size() const {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    return nodes_.size();
}

bool KBucket::is_empty() const {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    return nodes_.empty();
}

bool KBucket::is_full() const {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    return nodes_.size() >= K;
}

uint8_t KBucket::prefix_length() const {
    return prefix_length_;
}

std::chrono::system_clock::time_point KBucket::last_updated() const {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    return last_updated_;
}

void KBucket::update_last_updated() {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

    last_updated_ = std::chrono::system_clock::now();
}

} // namespace bitscrape::dht
