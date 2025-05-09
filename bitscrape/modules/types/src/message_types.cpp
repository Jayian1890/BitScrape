#include "bitscrape/types/message_types.hpp"

#include <sstream>
#include <stdexcept>

namespace bitscrape::types {

// Initialize static members
std::unordered_map<Message::Type, MessageFactory::MessageCreator> MessageFactory::creators_;
std::unordered_map<uint32_t, MessageFactory::MessageCreator> MessageFactory::custom_creators_;

Message::Message(Type type)
    : type_(type), custom_type_id_(0) {
}

Message::Message(Type type, uint32_t custom_type_id)
    : type_(type), custom_type_id_(custom_type_id) {
}

std::future<std::vector<uint8_t>> Message::serialize_async() const {
    return std::async(std::launch::async, [this]() {
        return this->serialize();
    });
}

std::string Message::to_string() const {
    std::ostringstream oss;
    
    oss << "Message[";
    
    switch (type_) {
        case Type::DHT_PING:
            oss << "DHT_PING";
            break;
        case Type::DHT_PING_RESPONSE:
            oss << "DHT_PING_RESPONSE";
            break;
        case Type::DHT_FIND_NODE:
            oss << "DHT_FIND_NODE";
            break;
        case Type::DHT_FIND_NODE_RESPONSE:
            oss << "DHT_FIND_NODE_RESPONSE";
            break;
        case Type::DHT_GET_PEERS:
            oss << "DHT_GET_PEERS";
            break;
        case Type::DHT_GET_PEERS_RESPONSE:
            oss << "DHT_GET_PEERS_RESPONSE";
            break;
        case Type::DHT_ANNOUNCE_PEER:
            oss << "DHT_ANNOUNCE_PEER";
            break;
        case Type::DHT_ANNOUNCE_PEER_RESPONSE:
            oss << "DHT_ANNOUNCE_PEER_RESPONSE";
            break;
        case Type::BT_HANDSHAKE:
            oss << "BT_HANDSHAKE";
            break;
        case Type::BT_KEEPALIVE:
            oss << "BT_KEEPALIVE";
            break;
        case Type::BT_CHOKE:
            oss << "BT_CHOKE";
            break;
        case Type::BT_UNCHOKE:
            oss << "BT_UNCHOKE";
            break;
        case Type::BT_INTERESTED:
            oss << "BT_INTERESTED";
            break;
        case Type::BT_NOT_INTERESTED:
            oss << "BT_NOT_INTERESTED";
            break;
        case Type::BT_HAVE:
            oss << "BT_HAVE";
            break;
        case Type::BT_BITFIELD:
            oss << "BT_BITFIELD";
            break;
        case Type::BT_REQUEST:
            oss << "BT_REQUEST";
            break;
        case Type::BT_PIECE:
            oss << "BT_PIECE";
            break;
        case Type::BT_CANCEL:
            oss << "BT_CANCEL";
            break;
        case Type::BT_PORT:
            oss << "BT_PORT";
            break;
        case Type::BT_EXTENDED:
            oss << "BT_EXTENDED";
            break;
        case Type::TRACKER_ANNOUNCE:
            oss << "TRACKER_ANNOUNCE";
            break;
        case Type::TRACKER_ANNOUNCE_RESPONSE:
            oss << "TRACKER_ANNOUNCE_RESPONSE";
            break;
        case Type::TRACKER_SCRAPE:
            oss << "TRACKER_SCRAPE";
            break;
        case Type::TRACKER_SCRAPE_RESPONSE:
            oss << "TRACKER_SCRAPE_RESPONSE";
            break;
        case Type::TRACKER_ERROR:
            oss << "TRACKER_ERROR";
            break;
        case Type::USER_DEFINED:
            oss << "USER_DEFINED(" << custom_type_id_ << ")";
            break;
        default:
            oss << "UNKNOWN(" << static_cast<int>(type_) << ")";
            break;
    }
    
    oss << "]";
    
    return oss.str();
}

std::unique_ptr<Message> MessageFactory::create(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        throw std::runtime_error("MessageFactory: Empty data");
    }
    
    // In a real implementation, we would parse the message type from the data
    // and use the appropriate creator function
    
    // For now, we'll just throw an exception
    throw std::runtime_error("MessageFactory: Not implemented");
}

std::future<std::unique_ptr<Message>> MessageFactory::create_async(const std::vector<uint8_t>& data) {
    return std::async(std::launch::async, [data]() {
        return create(data);
    });
}

} // namespace bitscrape::types
