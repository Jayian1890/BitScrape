#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <functional>
#include <unordered_map>

#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/types/endpoint.hpp"

namespace bitscrape::types {

/**
 * @brief Base class for all protocol messages
 * 
 * Message is the base class for all protocol messages in the system.
 * It provides common functionality for all messages, such as type information and serialization.
 */
class Message {
public:
    /**
     * @brief Message type enumeration
     */
    enum class Type {
        // DHT messages
        DHT_PING,
        DHT_PING_RESPONSE,
        DHT_FIND_NODE,
        DHT_FIND_NODE_RESPONSE,
        DHT_GET_PEERS,
        DHT_GET_PEERS_RESPONSE,
        DHT_ANNOUNCE_PEER,
        DHT_ANNOUNCE_PEER_RESPONSE,
        
        // BitTorrent messages
        BT_HANDSHAKE,
        BT_KEEPALIVE,
        BT_CHOKE,
        BT_UNCHOKE,
        BT_INTERESTED,
        BT_NOT_INTERESTED,
        BT_HAVE,
        BT_BITFIELD,
        BT_REQUEST,
        BT_PIECE,
        BT_CANCEL,
        BT_PORT,
        BT_EXTENDED,
        
        // Tracker messages
        TRACKER_ANNOUNCE,
        TRACKER_ANNOUNCE_RESPONSE,
        TRACKER_SCRAPE,
        TRACKER_SCRAPE_RESPONSE,
        TRACKER_ERROR,
        
        // User-defined messages start at 1000
        USER_DEFINED = 1000
    };

    /**
     * @brief Create a message with the specified type
     * 
     * @param type Message type
     */
    explicit Message(Type type);
    
    /**
     * @brief Create a message with the specified type and custom type ID
     * 
     * @param type Message type
     * @param custom_type_id Custom type ID for user-defined messages
     */
    Message(Type type, uint32_t custom_type_id);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Message() = default;
    
    /**
     * @brief Get the message type
     * 
     * @return Message type
     */
    Type type() const { return type_; }
    
    /**
     * @brief Get the custom type ID
     * 
     * @return Custom type ID (only valid for user-defined messages)
     */
    uint32_t custom_type_id() const { return custom_type_id_; }
    
    /**
     * @brief Serialize the message to a byte vector
     * 
     * @return Serialized message
     */
    virtual std::vector<uint8_t> serialize() const = 0;
    
    /**
     * @brief Serialize the message to a byte vector asynchronously
     * 
     * @return Future containing the serialized message
     */
    virtual std::future<std::vector<uint8_t>> serialize_async() const;
    
    /**
     * @brief Clone the message
     * 
     * @return A new heap-allocated copy of the message
     */
    virtual std::unique_ptr<Message> clone() const = 0;
    
    /**
     * @brief Convert the message to a string representation
     * 
     * @return String representation of the message
     */
    virtual std::string to_string() const;

private:
    Type type_;           ///< Message type
    uint32_t custom_type_id_; ///< Custom type ID for user-defined messages
};

/**
 * @brief Factory for creating messages from serialized data
 */
class MessageFactory {
public:
    /**
     * @brief Create a message from serialized data
     * 
     * @param data Serialized message data
     * @return Unique pointer to the deserialized message
     * @throws std::runtime_error if the data cannot be deserialized
     */
    static std::unique_ptr<Message> create(const std::vector<uint8_t>& data);
    
    /**
     * @brief Create a message from serialized data asynchronously
     * 
     * @param data Serialized message data
     * @return Future containing a unique pointer to the deserialized message
     */
    static std::future<std::unique_ptr<Message>> create_async(const std::vector<uint8_t>& data);
    
    /**
     * @brief Register a message type with the factory
     * 
     * @tparam T Message type class
     * @param type Message type
     * @param custom_type_id Custom type ID for user-defined messages
     */
    template<typename T>
    static void register_type(Message::Type type, uint32_t custom_type_id = 0);
    
private:
    using MessageCreator = std::function<std::unique_ptr<Message>(const std::vector<uint8_t>&)>;
    static std::unordered_map<Message::Type, MessageCreator> creators_;
    static std::unordered_map<uint32_t, MessageCreator> custom_creators_;
};

} // namespace bitscrape::types
