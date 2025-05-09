#include "bitscrape/types/event_types.hpp"

#include <sstream>
#include <iomanip>

namespace bitscrape::types {

Event::Event(Type type)
    : type_(type), custom_type_id_(0), timestamp_(std::chrono::system_clock::now()) {
}

Event::Event(Type type, uint32_t custom_type_id)
    : type_(type), custom_type_id_(custom_type_id), timestamp_(std::chrono::system_clock::now()) {
}

std::string Event::to_string() const {
    std::ostringstream oss;
    
    // Format the timestamp
    auto time_t = std::chrono::system_clock::to_time_t(timestamp_);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp_.time_since_epoch() % std::chrono::seconds(1)).count();
    
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
        << '.' << std::setfill('0') << std::setw(3) << ms << ' ';
    
    // Add the event type
    oss << "Event[";
    
    switch (type_) {
        case Type::SYSTEM_STARTUP:
            oss << "SYSTEM_STARTUP";
            break;
        case Type::SYSTEM_SHUTDOWN:
            oss << "SYSTEM_SHUTDOWN";
            break;
        case Type::SYSTEM_ERROR:
            oss << "SYSTEM_ERROR";
            break;
        case Type::NETWORK_CONNECTED:
            oss << "NETWORK_CONNECTED";
            break;
        case Type::NETWORK_DISCONNECTED:
            oss << "NETWORK_DISCONNECTED";
            break;
        case Type::NETWORK_ERROR:
            oss << "NETWORK_ERROR";
            break;
        case Type::DHT_NODE_FOUND:
            oss << "DHT_NODE_FOUND";
            break;
        case Type::DHT_NODE_TIMEOUT:
            oss << "DHT_NODE_TIMEOUT";
            break;
        case Type::DHT_BOOTSTRAP_COMPLETE:
            oss << "DHT_BOOTSTRAP_COMPLETE";
            break;
        case Type::DHT_INFOHASH_FOUND:
            oss << "DHT_INFOHASH_FOUND";
            break;
        case Type::BT_PEER_CONNECTED:
            oss << "BT_PEER_CONNECTED";
            break;
        case Type::BT_PEER_DISCONNECTED:
            oss << "BT_PEER_DISCONNECTED";
            break;
        case Type::BT_METADATA_RECEIVED:
            oss << "BT_METADATA_RECEIVED";
            break;
        case Type::BT_METADATA_ERROR:
            oss << "BT_METADATA_ERROR";
            break;
        case Type::TRACKER_CONNECTED:
            oss << "TRACKER_CONNECTED";
            break;
        case Type::TRACKER_DISCONNECTED:
            oss << "TRACKER_DISCONNECTED";
            break;
        case Type::TRACKER_ANNOUNCE_COMPLETE:
            oss << "TRACKER_ANNOUNCE_COMPLETE";
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

} // namespace bitscrape::types
