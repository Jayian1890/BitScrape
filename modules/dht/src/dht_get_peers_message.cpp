#include "bitscrape/dht/dht_get_peers_message.hpp"

#include <sstream>

namespace bitscrape::dht {

DHTGetPeersMessage::DHTGetPeersMessage(const std::string& transaction_id, const types::NodeID& node_id, const types::InfoHash& info_hash)
    : DHTMessage(Type::GET_PEERS, transaction_id), node_id_(node_id), info_hash_(info_hash), is_response_(false) {
}

DHTGetPeersMessage::DHTGetPeersMessage(const std::string& transaction_id, 
                                     const types::NodeID& node_id, 
                                     const types::DHTToken& token,
                                     const std::vector<types::DHTNode>& nodes,
                                     const std::vector<types::Endpoint>& values)
    : DHTMessage(Type::GET_PEERS_RESPONSE, transaction_id), 
      node_id_(node_id), 
      token_(token),
      nodes_(nodes),
      values_(values),
      is_response_(true) {
}

const types::NodeID& DHTGetPeersMessage::node_id() const {
    return node_id_;
}

void DHTGetPeersMessage::set_node_id(const types::NodeID& node_id) {
    node_id_ = node_id;
}

const types::InfoHash& DHTGetPeersMessage::info_hash() const {
    return info_hash_;
}

void DHTGetPeersMessage::set_info_hash(const types::InfoHash& info_hash) {
    info_hash_ = info_hash;
}

const types::DHTToken& DHTGetPeersMessage::token() const {
    return token_;
}

void DHTGetPeersMessage::set_token(const types::DHTToken& token) {
    token_ = token;
}

const std::vector<types::DHTNode>& DHTGetPeersMessage::nodes() const {
    return nodes_;
}

void DHTGetPeersMessage::set_nodes(const std::vector<types::DHTNode>& nodes) {
    nodes_ = nodes;
}

const std::vector<types::Endpoint>& DHTGetPeersMessage::values() const {
    return values_;
}

void DHTGetPeersMessage::set_values(const std::vector<types::Endpoint>& values) {
    values_ = values;
}

bencode::BencodeValue DHTGetPeersMessage::to_bencode() const {
    bencode::BencodeValue dict = DHTMessage::to_bencode();

    if (type() == Type::GET_PEERS) {
        // Add arguments dictionary (a)
        std::map<std::string, bencode::BencodeValue> args_map;
        
        // Add node ID
        std::vector<uint8_t> id_bytes(node_id_.bytes().begin(), node_id_.bytes().end());
        args_map["id"] = bencode::BencodeValue(id_bytes);
        
        // Add infohash
        std::vector<uint8_t> info_hash_bytes(info_hash_.bytes().begin(), info_hash_.bytes().end());
        args_map["info_hash"] = bencode::BencodeValue(info_hash_bytes);
        
        dict.set("a", bencode::BencodeValue(args_map));
    } else if (type() == Type::GET_PEERS_RESPONSE) {
        // Add response dictionary (r)
        std::map<std::string, bencode::BencodeValue> response_map;
        
        // Add node ID
        std::vector<uint8_t> id_bytes(node_id_.bytes().begin(), node_id_.bytes().end());
        response_map["id"] = bencode::BencodeValue(id_bytes);
        
        // Add token
        std::vector<uint8_t> token_bytes = token_.bytes();
        response_map["token"] = bencode::BencodeValue(token_bytes);
        
        // Add nodes if present
        if (!nodes_.empty()) {
            // Compact node info: 26 bytes per node (20 byte ID + 6 byte endpoint)
            std::vector<uint8_t> nodes_bytes;
            for (const auto& node : nodes_) {
                // Add node ID
                const auto& id_bytes = node.id().bytes();
                nodes_bytes.insert(nodes_bytes.end(), id_bytes.begin(), id_bytes.end());
                
                // Add endpoint (4 bytes IP + 2 bytes port)
                // This is a simplified implementation - in a real implementation,
                // we would need to convert the IP address to bytes
                // For now, just add 6 zero bytes as a placeholder
                nodes_bytes.insert(nodes_bytes.end(), 6, 0);
            }
            response_map["nodes"] = bencode::BencodeValue(nodes_bytes);
        }
        
        // Add values if present
        if (!values_.empty()) {
            std::vector<bencode::BencodeValue> values_list;
            for (const auto& endpoint : values_) {
                // Compact peer info: 6 bytes per peer (4 byte IP + 2 byte port)
                // This is a simplified implementation - in a real implementation,
                // we would need to convert the IP address and port to bytes
                // For now, just add the endpoint address as a string
                values_list.push_back(bencode::BencodeValue(endpoint.to_string()));
            }
            response_map["values"] = bencode::BencodeValue(values_list);
        }
        
        dict.set("r", bencode::BencodeValue(response_map));
    }

    return dict;
}

bool DHTGetPeersMessage::is_valid() const {
    if (!DHTMessage::is_valid()) {
        return false;
    }
    
    if (type() == Type::GET_PEERS) {
        // For a query, we need a valid node ID and infohash
        return !node_id_.to_hex().empty() && !info_hash_.to_hex().empty();
    } else if (type() == Type::GET_PEERS_RESPONSE) {
        // For a response, we need a valid node ID and token
        // We also need either nodes or values (or both)
        return !node_id_.to_hex().empty() && !token_.bytes().empty() && (!nodes_.empty() || !values_.empty());
    }
    
    return false;
}

std::string DHTGetPeersMessage::to_string() const {
    std::ostringstream oss;

    oss << "DHTGetPeersMessage[type=";

    if (type() == Type::GET_PEERS) {
        oss << "GET_PEERS";
    } else {
        oss << "GET_PEERS_RESPONSE";
    }

    oss << ", transaction_id=" << transaction_id()
        << ", node_id=" << node_id_.to_hex().substr(0, 8) << "...";
    
    if (type() == Type::GET_PEERS) {
        oss << ", info_hash=" << info_hash_.to_hex().substr(0, 8) << "...";
    } else {
        oss << ", token=...";
        if (!nodes_.empty()) {
            oss << ", nodes=" << nodes_.size();
        }
        if (!values_.empty()) {
            oss << ", values=" << values_.size();
        }
    }
    
    oss << "]";

    return oss.str();
}

} // namespace bitscrape::dht
