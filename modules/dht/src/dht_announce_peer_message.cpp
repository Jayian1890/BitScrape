#include "bitscrape/dht/dht_announce_peer_message.hpp"

#include <sstream>

namespace bitscrape::dht {

DHTAnnouncePeerMessage::DHTAnnouncePeerMessage(const std::string& transaction_id, 
                                             const types::NodeID& node_id, 
                                             const types::InfoHash& info_hash,
                                             uint16_t port,
                                             const types::DHTToken& token,
                                             bool implied_port)
    : DHTMessage(Type::ANNOUNCE_PEER, transaction_id), 
      node_id_(node_id), 
      info_hash_(info_hash),
      port_(port),
      token_(token),
      implied_port_(implied_port),
      is_response_(false) {
}

DHTAnnouncePeerMessage::DHTAnnouncePeerMessage(const std::string& transaction_id, const types::NodeID& node_id)
    : DHTMessage(Type::ANNOUNCE_PEER_RESPONSE, transaction_id), 
      node_id_(node_id), 
      port_(0),
      implied_port_(false),
      is_response_(true) {
}

const types::NodeID& DHTAnnouncePeerMessage::node_id() const {
    return node_id_;
}

void DHTAnnouncePeerMessage::set_node_id(const types::NodeID& node_id) {
    node_id_ = node_id;
}

const types::InfoHash& DHTAnnouncePeerMessage::info_hash() const {
    return info_hash_;
}

void DHTAnnouncePeerMessage::set_info_hash(const types::InfoHash& info_hash) {
    info_hash_ = info_hash;
}

uint16_t DHTAnnouncePeerMessage::port() const {
    return port_;
}

void DHTAnnouncePeerMessage::set_port(uint16_t port) {
    port_ = port;
}

const types::DHTToken& DHTAnnouncePeerMessage::token() const {
    return token_;
}

void DHTAnnouncePeerMessage::set_token(const types::DHTToken& token) {
    token_ = token;
}

bool DHTAnnouncePeerMessage::implied_port() const {
    return implied_port_;
}

void DHTAnnouncePeerMessage::set_implied_port(bool implied_port) {
    implied_port_ = implied_port;
}

bencode::BencodeValue DHTAnnouncePeerMessage::to_bencode() const {
    bencode::BencodeValue dict = DHTMessage::to_bencode();
    
    if (type() == Type::ANNOUNCE_PEER) {
        // Add arguments dictionary (a)
        std::map<std::string, bencode::BencodeValue> args_map;
        
        // Add node ID
        std::vector<uint8_t> id_bytes(node_id_.bytes().begin(), node_id_.bytes().end());
        args_map["id"] = bencode::BencodeValue(id_bytes);
        
        // Add infohash
        std::vector<uint8_t> info_hash_bytes(info_hash_.bytes().begin(), info_hash_.bytes().end());
        args_map["info_hash"] = bencode::BencodeValue(info_hash_bytes);
        
        // Add port
        args_map["port"] = bencode::BencodeValue(static_cast<int64_t>(port_));
        
        // Add token
        std::vector<uint8_t> token_bytes(token_.bytes().begin(), token_.bytes().end());
        args_map["token"] = bencode::BencodeValue(token_bytes);
        
        // Add implied_port if true
        if (implied_port_) {
            args_map["implied_port"] = bencode::BencodeValue(static_cast<int64_t>(1));
        }
        
        dict.set("a", bencode::BencodeValue(args_map));
    } else if (type() == Type::ANNOUNCE_PEER_RESPONSE) {
        // Add response dictionary (r)
        std::map<std::string, bencode::BencodeValue> response_map;
        
        // Add node ID
        std::vector<uint8_t> id_bytes(node_id_.bytes().begin(), node_id_.bytes().end());
        response_map["id"] = bencode::BencodeValue(id_bytes);
        
        dict.set("r", bencode::BencodeValue(response_map));
    }
    
    return dict;
}

bool DHTAnnouncePeerMessage::is_valid() const {
    if (!DHTMessage::is_valid()) {
        return false;
    }
    
    if (type() == Type::ANNOUNCE_PEER) {
        // For a query, we need a valid node ID, infohash, port, and token
        return !node_id_.to_hex().empty() && 
               !info_hash_.to_hex().empty() && 
               port_ > 0 && 
               !token_.bytes().empty();
    } else if (type() == Type::ANNOUNCE_PEER_RESPONSE) {
        // For a response, we just need a valid node ID
        return !node_id_.to_hex().empty();
    }
    
    return false;
}

std::string DHTAnnouncePeerMessage::to_string() const {
    std::ostringstream oss;

    oss << "DHTAnnouncePeerMessage[type=";

    if (type() == Type::ANNOUNCE_PEER) {
        oss << "ANNOUNCE_PEER";
    } else {
        oss << "ANNOUNCE_PEER_RESPONSE";
    }

    oss << ", transaction_id=" << transaction_id()
        << ", node_id=" << node_id_.to_hex().substr(0, 8) << "...";
    
    if (type() == Type::ANNOUNCE_PEER) {
        oss << ", info_hash=" << info_hash_.to_hex().substr(0, 8) << "..."
            << ", port=" << port_
            << ", token=...";
        if (implied_port_) {
            oss << ", implied_port=true";
        }
    }
    
    oss << "]";

    return oss.str();
}

} // namespace bitscrape::dht
