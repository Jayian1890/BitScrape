#include "bitscrape/types/dht_routing_table_entry.hpp"

#include <sstream>
#include <algorithm>

namespace bitscrape::types {

DHTRoutingTableEntry::DHTRoutingTableEntry(uint8_t prefix_length)
    : prefix_length_(prefix_length), last_updated_(std::chrono::system_clock::now()) {
}

bool DHTRoutingTableEntry::add_node(const DHTNode& node) {
    // Check if the bucket is full
    if (is_full()) {
        return false;
    }
    
    // Check if the node is already in the bucket
    if (contains_node(node)) {
        return false;
    }
    
    // Add the node to the bucket
    nodes_.push_back(node);
    last_updated_ = std::chrono::system_clock::now();
    
    return true;
}

bool DHTRoutingTableEntry::remove_node(const DHTNode& node) {
    auto it = std::find(nodes_.begin(), nodes_.end(), node);
    
    if (it == nodes_.end()) {
        return false;
    }
    
    nodes_.erase(it);
    last_updated_ = std::chrono::system_clock::now();
    
    return true;
}

bool DHTRoutingTableEntry::update_node(const DHTNode& node) {
    for (auto& n : nodes_) {
        if (n.id() == node.id()) {
            n = node;
            last_updated_ = std::chrono::system_clock::now();
            return true;
        }
    }
    
    return false;
}

bool DHTRoutingTableEntry::contains_node(const DHTNode& node) const {
    return std::find(nodes_.begin(), nodes_.end(), node) != nodes_.end();
}

bool DHTRoutingTableEntry::contains_node_id(const NodeID& id) const {
    for (const auto& node : nodes_) {
        if (node.id() == id) {
            return true;
        }
    }
    
    return false;
}

const DHTNode* DHTRoutingTableEntry::get_node(const NodeID& id) const {
    for (const auto& node : nodes_) {
        if (node.id() == id) {
            return &node;
        }
    }
    
    return nullptr;
}

bool DHTRoutingTableEntry::contains_id_in_range(const NodeID& id, const NodeID& local_id) const {
    // Check if the first prefix_length_ bits of the XOR of id and local_id are all 0
    // except for the prefix_length_-th bit, which should be 1
    
    NodeID distance = id.distance(local_id);
    const auto& bytes = distance.bytes();
    
    // Check all bits before the prefix_length_-th bit
    for (size_t i = 0; i < prefix_length_ / 8; ++i) {
        if (bytes[i] != 0) {
            return false;
        }
    }
    
    // Check the byte containing the prefix_length_-th bit
    if (prefix_length_ % 8 != 0) {
        uint8_t byte = bytes[prefix_length_ / 8];
        uint8_t mask = 0x80 >> (prefix_length_ % 8 - 1);
        
        // The prefix_length_-th bit should be 1
        if ((byte & mask) == 0) {
            return false;
        }
        
        // All bits before the prefix_length_-th bit should be 0
        uint8_t higher_bits_mask = ~(mask | (mask - 1));
        if ((byte & higher_bits_mask) != 0) {
            return false;
        }
    }
    
    return true;
}

std::string DHTRoutingTableEntry::to_string() const {
    std::ostringstream oss;
    
    oss << "DHTRoutingTableEntry[prefix_length=" << static_cast<int>(prefix_length_) << ", ";
    oss << "nodes=" << nodes_.size() << "]";
    
    return oss.str();
}

} // namespace bitscrape::types
