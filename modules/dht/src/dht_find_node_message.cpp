#include "bitscrape/dht/dht_find_node_message.hpp"

#include <sstream>

namespace bitscrape::dht {

DHTFindNodeMessage::DHTFindNodeMessage(const std::string& transaction_id, const types::NodeID& node_id, const types::NodeID& target_id)
    : DHTMessage(Type::FIND_NODE, transaction_id), node_id_(node_id), target_id_(target_id), is_response_(false) {
}

DHTFindNodeMessage::DHTFindNodeMessage(const std::string& transaction_id, 
                                     const types::NodeID& node_id, 
                                     const std::vector<types::DHTNode>& nodes)
    : DHTMessage(Type::FIND_NODE_RESPONSE, transaction_id), 
      node_id_(node_id), 
      nodes_(nodes),
      is_response_(true) {
}

const types::NodeID& DHTFindNodeMessage::node_id() const {
    return node_id_;
}

void DHTFindNodeMessage::set_node_id(const types::NodeID& node_id) {
    node_id_ = node_id;
}

const types::NodeID& DHTFindNodeMessage::target_id() const {
    return target_id_;
}

void DHTFindNodeMessage::set_target_id(const types::NodeID& target_id) {
    target_id_ = target_id;
}

const std::vector<types::DHTNode>& DHTFindNodeMessage::nodes() const {
    return nodes_;
}

void DHTFindNodeMessage::set_nodes(const std::vector<types::DHTNode>& nodes) {
    nodes_ = nodes;
}

bencode::BencodeValue DHTFindNodeMessage::to_bencode() const {
    bencode::BencodeValue dict = DHTMessage::to_bencode();
    
    if (type() == Type::FIND_NODE) {
        // Add arguments dictionary (a)
        std::map<std::string, bencode::BencodeValue> args_map;
        
        // Add node ID
        std::vector<uint8_t> id_bytes(node_id_.bytes().begin(), node_id_.bytes().end());
        args_map["id"] = bencode::BencodeValue(id_bytes);
        
        // Add target ID
        std::vector<uint8_t> target_bytes(target_id_.bytes().begin(), target_id_.bytes().end());
        args_map["target"] = bencode::BencodeValue(target_bytes);
        
        dict.set("a", bencode::BencodeValue(args_map));
    } else if (type() == Type::FIND_NODE_RESPONSE) {
        // Add response dictionary (r)
        std::map<std::string, bencode::BencodeValue> response_map;
        
        // Add node ID
        std::vector<uint8_t> id_bytes(node_id_.bytes().begin(), node_id_.bytes().end());
        response_map["id"] = bencode::BencodeValue(id_bytes);
        
        // Add nodes (compact format: 26 bytes per node - 20 bytes ID + 6 bytes endpoint)
        std::string nodes_str;
        for (const auto& node : nodes_) {
            // Add node ID (20 bytes)
            const auto& id_bytes = node.id().bytes();
            nodes_str.append(std::string(id_bytes.begin(), id_bytes.end()));
            
            // Add endpoint (6 bytes: 4 bytes IP + 2 bytes port)
            // This is a simplified implementation - in a real implementation,
            // we would need to convert the IP address to a 4-byte representation
            // For now, just add 6 null bytes as a placeholder
            nodes_str.append(std::string(6, '\0'));
        }
        response_map["nodes"] = bencode::BencodeValue(nodes_str);
        
        dict.set("r", bencode::BencodeValue(response_map));
    }
    
    return dict;
}

bool DHTFindNodeMessage::is_valid() const {
    if (!DHTMessage::is_valid()) {
        return false;
    }
    
    if (type() == Type::FIND_NODE) {
        // For a query, we need a valid node ID and target ID
        return !node_id_.to_hex().empty() && !target_id_.to_hex().empty();
    } else if (type() == Type::FIND_NODE_RESPONSE) {
        // For a response, we need a valid node ID and at least one node
        return !node_id_.to_hex().empty() && !nodes_.empty();
    }
    
    return false;
}

std::string DHTFindNodeMessage::to_string() const {
    std::ostringstream oss;

    oss << "DHTFindNodeMessage[type=";

    if (type() == Type::FIND_NODE) {
        oss << "FIND_NODE";
    } else {
        oss << "FIND_NODE_RESPONSE";
    }

    oss << ", transaction_id=" << transaction_id()
        << ", node_id=" << node_id_.to_hex().substr(0, 8) << "...";
    
    if (type() == Type::FIND_NODE) {
        oss << ", target_id=" << target_id_.to_hex().substr(0, 8) << "...";
    } else {
        oss << ", nodes=" << nodes_.size();
    }
    
    oss << "]";

    return oss.str();
}

} // namespace bitscrape::dht
