#pragma once

#include <cstdint>
#include <string>
#include <any>
#include <memory>
#include <chrono>
#include <functional>
#include <future>
#include <unordered_map>
#include <vector>

namespace bitscrape::types {

/**
 * @brief Base class for all events in the system
 * 
 * Event is the base class for all events that can be dispatched through the event system.
 * It provides common functionality for all events, such as type information and timestamps.
 */
class Event {
public:
    /**
     * @brief Event type enumeration
     */
    enum class Type {
        // System events
        SYSTEM_STARTUP,
        SYSTEM_SHUTDOWN,
        SYSTEM_ERROR,
        
        // Network events
        NETWORK_CONNECTED,
        NETWORK_DISCONNECTED,
        NETWORK_ERROR,
        
        // DHT events
        DHT_NODE_FOUND,
        DHT_NODE_TIMEOUT,
        DHT_BOOTSTRAP_COMPLETE,
        DHT_INFOHASH_FOUND,
        
        // BitTorrent events
        BT_PEER_CONNECTED,
        BT_PEER_DISCONNECTED,
        BT_METADATA_RECEIVED,
        BT_METADATA_ERROR,
        
        // Tracker events
        TRACKER_CONNECTED,
        TRACKER_DISCONNECTED,
        TRACKER_ANNOUNCE_COMPLETE,
        TRACKER_ERROR,
        
        // User-defined events start at 1000
        USER_DEFINED = 1000
    };

    /**
     * @brief Create an event with the specified type
     * 
     * @param type Event type
     */
    explicit Event(Type type);
    
    /**
     * @brief Create an event with the specified type and custom type ID
     * 
     * @param type Event type
     * @param custom_type_id Custom type ID for user-defined events
     */
    Event(Type type, uint32_t custom_type_id);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Event() = default;
    
    /**
     * @brief Get the event type
     * 
     * @return Event type
     */
    Type type() const { return type_; }
    
    /**
     * @brief Get the custom type ID
     * 
     * @return Custom type ID (only valid for user-defined events)
     */
    uint32_t custom_type_id() const { return custom_type_id_; }
    
    /**
     * @brief Get the event timestamp
     * 
     * @return Event timestamp
     */
    std::chrono::system_clock::time_point timestamp() const { return timestamp_; }
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    virtual std::unique_ptr<Event> clone() const = 0;
    
    /**
     * @brief Convert the event to a string representation
     * 
     * @return String representation of the event
     */
    virtual std::string to_string() const;

private:
    Type type_;                                   ///< Event type
    uint32_t custom_type_id_;                     ///< Custom type ID for user-defined events
    std::chrono::system_clock::time_point timestamp_; ///< Event timestamp
};

/**
 * @brief Type-safe event handler function
 * 
 * @tparam T Event type
 */
template<typename T>
using EventHandler = std::function<void(const T&)>;

/**
 * @brief Event subscription token
 * 
 * This token is returned when subscribing to events and can be used to unsubscribe.
 */
class SubscriptionToken {
public:
    /**
     * @brief Create a subscription token
     * 
     * @param id Token ID
     */
    explicit SubscriptionToken(uint64_t id) : id_(id) {}
    
    /**
     * @brief Get the token ID
     * 
     * @return Token ID
     */
    uint64_t id() const { return id_; }
    
    /**
     * @brief Equality operator
     */
    bool operator==(const SubscriptionToken& other) const { return id_ == other.id_; }
    
    /**
     * @brief Inequality operator
     */
    bool operator!=(const SubscriptionToken& other) const { return id_ != other.id_; }

private:
    uint64_t id_; ///< Token ID
};

/**
 * @brief Hash function for SubscriptionToken
 */
struct SubscriptionTokenHash {
    std::size_t operator()(const SubscriptionToken& token) const {
        return std::hash<uint64_t>{}(token.id());
    }
};

} // namespace bitscrape::types
