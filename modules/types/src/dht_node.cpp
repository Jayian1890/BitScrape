#include "bitscrape/types/dht_node.hpp"

#include <sstream>

namespace bitscrape::types {

DHTNode::DHTNode() : last_seen_(std::chrono::system_clock::now()) {
    // Create an invalid node
}

DHTNode::DHTNode(const NodeID& id, const Endpoint& endpoint)
    : id_(id), endpoint_(endpoint), status_(Status::UNKNOWN), last_seen_(std::chrono::system_clock::now()) {
}

DHTNode::DHTNode(const NodeID& id, const Endpoint& endpoint, Status status)
    : id_(id), endpoint_(endpoint), status_(status), last_seen_(std::chrono::system_clock::now()) {
}

void DHTNode::set_status(Status status) {
    status_ = status;
}

void DHTNode::update_last_seen() {
    last_seen_ = std::chrono::system_clock::now();
}

bool DHTNode::is_valid() const {
    return endpoint_.is_valid();
}

NodeID DHTNode::distance(const DHTNode& other) const {
    return id_.distance(other.id_);
}

std::future<NodeID> DHTNode::distance_async(const DHTNode& other) const {
    return id_.distance_async(other.id_);
}

NodeID DHTNode::distance(const NodeID& id) const {
    return id_.distance(id);
}

std::future<NodeID> DHTNode::distance_async(const NodeID& id) const {
    return id_.distance_async(id);
}

std::string DHTNode::to_string() const {
    std::ostringstream oss;
    
    oss << "DHTNode[id=" << id_.to_hex().substr(0, 8) << "..., ";
    oss << "endpoint=" << endpoint_.to_string() << ", ";
    
    switch (status_) {
        case Status::UNKNOWN:
            oss << "status=UNKNOWN";
            break;
        case Status::GOOD:
            oss << "status=GOOD";
            break;
        case Status::QUESTIONABLE:
            oss << "status=QUESTIONABLE";
            break;
        case Status::BAD:
            oss << "status=BAD";
            break;
        default:
            oss << "status=INVALID";
            break;
    }
    
    oss << "]";
    
    return oss.str();
}

bool DHTNode::operator==(const DHTNode& other) const {
    return id_ == other.id_ && endpoint_ == other.endpoint_;
}

bool DHTNode::operator!=(const DHTNode& other) const {
    return !(*this == other);
}

} // namespace bitscrape::types
