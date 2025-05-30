#include "bitscrape/dht/dht_message.hpp"

#include <sstream>
#include <future>

#include "bitscrape/bencode/bencode_encoder.hpp"

namespace bitscrape::dht {

DHTMessage::DHTMessage(Type type, const std::string& transaction_id)
    : type_(type), transaction_id_(transaction_id) {
}

DHTMessage::Type DHTMessage::type() const {
    return type_;
}

const std::string& DHTMessage::transaction_id() const {
    return transaction_id_;
}

void DHTMessage::set_transaction_id(const std::string& transaction_id) {
    transaction_id_ = transaction_id;
}

bencode::BencodeValue DHTMessage::to_bencode() const {
    std::map<std::string, bencode::BencodeValue> dict_map;
    bencode::BencodeValue dict(dict_map);
    add_common_fields(dict);
    return dict;
}

std::future<bencode::BencodeValue> DHTMessage::to_bencode_async() const {
    return std::async(std::launch::async, [this]() {
        return this->to_bencode();
    });
}

std::vector<uint8_t> DHTMessage::encode() const {
    bencode::BencodeValue dict = to_bencode();
    auto encoder = bencode::create_bencode_encoder();
    return encoder->encode(dict);
}

std::future<std::vector<uint8_t>> DHTMessage::encode_async() const {
    return std::async(std::launch::async, [this]() {
        return this->encode();
    });
}

bool DHTMessage::is_valid() const {
    return !transaction_id_.empty();
}

std::string DHTMessage::to_string() const {
    std::ostringstream oss;

    oss << "DHTMessage[type=";

    switch (type_) {
        case Type::PING:
            oss << "PING";
            break;
        case Type::PING_RESPONSE:
            oss << "PING_RESPONSE";
            break;
        case Type::FIND_NODE:
            oss << "FIND_NODE";
            break;
        case Type::FIND_NODE_RESPONSE:
            oss << "FIND_NODE_RESPONSE";
            break;
        case Type::GET_PEERS:
            oss << "GET_PEERS";
            break;
        case Type::GET_PEERS_RESPONSE:
            oss << "GET_PEERS_RESPONSE";
            break;
        case Type::ANNOUNCE_PEER:
            oss << "ANNOUNCE_PEER";
            break;
        case Type::ANNOUNCE_PEER_RESPONSE:
            oss << "ANNOUNCE_PEER_RESPONSE";
            break;
        case Type::ERROR:
            oss << "ERROR";
            break;
        default:
            oss << "UNKNOWN";
            break;
    }

    oss << ", transaction_id=" << transaction_id_ << "]";

    return oss.str();
}

void DHTMessage::add_common_fields(bencode::BencodeValue& dict) const {
    // Add transaction ID
    dict.set("t", bencode::BencodeValue(transaction_id_));

    // Add message type (y)
    if (type_ == Type::PING || type_ == Type::FIND_NODE ||
        type_ == Type::GET_PEERS || type_ == Type::ANNOUNCE_PEER) {
        dict.set("y", bencode::BencodeValue("q"));
    } else if (type_ == Type::PING_RESPONSE || type_ == Type::FIND_NODE_RESPONSE ||
               type_ == Type::GET_PEERS_RESPONSE || type_ == Type::ANNOUNCE_PEER_RESPONSE) {
        dict.set("y", bencode::BencodeValue("r"));
    } else if (type_ == Type::ERROR) {
        dict.set("y", bencode::BencodeValue("e"));
    }

    // Add query type (q) for queries
    if (type_ == Type::PING) {
        dict.set("q", bencode::BencodeValue("ping"));
    } else if (type_ == Type::FIND_NODE) {
        dict.set("q", bencode::BencodeValue("find_node"));
    } else if (type_ == Type::GET_PEERS) {
        dict.set("q", bencode::BencodeValue("get_peers"));
    } else if (type_ == Type::ANNOUNCE_PEER) {
        dict.set("q", bencode::BencodeValue("announce_peer"));
    }

    // Add version (v)
    dict.set("v", bencode::BencodeValue("BS"));
}

// DHTPingMessage implementation

DHTPingMessage::DHTPingMessage(const std::string& transaction_id, const types::NodeID& node_id)
    : DHTMessage(Type::PING, transaction_id), node_id_(node_id) {
}

DHTPingMessage::DHTPingMessage(const std::string& transaction_id, const types::NodeID& node_id, bool is_response)
    : DHTMessage(is_response ? Type::PING_RESPONSE : Type::PING, transaction_id), node_id_(node_id) {
}

const types::NodeID& DHTPingMessage::node_id() const {
    return node_id_;
}

void DHTPingMessage::set_node_id(const types::NodeID& node_id) {
    node_id_ = node_id;
}

bencode::BencodeValue DHTPingMessage::to_bencode() const {
    bencode::BencodeValue dict = DHTMessage::to_bencode();

    if (type() == Type::PING) {
        // Add arguments dictionary (a)
        std::map<std::string, bencode::BencodeValue> args_map;
        std::vector<uint8_t> id_bytes(node_id_.bytes().begin(), node_id_.bytes().end());
        args_map["id"] = bencode::BencodeValue(id_bytes);
        dict.set("a", bencode::BencodeValue(args_map));
    } else if (type() == Type::PING_RESPONSE) {
        // Add response dictionary (r)
        std::map<std::string, bencode::BencodeValue> response_map;
        std::vector<uint8_t> id_bytes(node_id_.bytes().begin(), node_id_.bytes().end());
        response_map["id"] = bencode::BencodeValue(id_bytes);
        dict.set("r", bencode::BencodeValue(response_map));
    }

    return dict;
}

bool DHTPingMessage::is_valid() const {
    return DHTMessage::is_valid();
}

std::string DHTPingMessage::to_string() const {
    std::ostringstream oss;

    oss << "DHTPingMessage[type=";

    if (type() == Type::PING) {
        oss << "PING";
    } else {
        oss << "PING_RESPONSE";
    }

    oss << ", transaction_id=" << transaction_id()
        << ", node_id=" << node_id_.to_hex().substr(0, 8) << "...]";

    return oss.str();
}

} // namespace bitscrape::dht
