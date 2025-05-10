#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <optional>

#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/types/endpoint.hpp"
#include "bitscrape/types/dht_node.hpp"
#include "bitscrape/types/dht_token.hpp"
#include "bitscrape/bencode/bencode_value.hpp"

namespace bitscrape::dht {

/**
 * @brief Base class for all DHT protocol messages
 * 
 * DHTMessage is the base class for all DHT protocol messages.
 * It provides common functionality for all messages, such as type information and serialization.
 */
class DHTMessage {
public:
    /**
     * @brief DHT message type enumeration
     */
    enum class Type {
        PING,
        PING_RESPONSE,
        FIND_NODE,
        FIND_NODE_RESPONSE,
        GET_PEERS,
        GET_PEERS_RESPONSE,
        ANNOUNCE_PEER,
        ANNOUNCE_PEER_RESPONSE,
        ERROR
    };

    /**
     * @brief Create a message with the specified type and transaction ID
     * 
     * @param type Message type
     * @param transaction_id Transaction ID
     */
    DHTMessage(Type type, const std::string& transaction_id);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~DHTMessage() = default;
    
    /**
     * @brief Get the message type
     * 
     * @return Message type
     */
    Type type() const;
    
    /**
     * @brief Get the transaction ID
     * 
     * @return Transaction ID
     */
    const std::string& transaction_id() const;
    
    /**
     * @brief Set the transaction ID
     * 
     * @param transaction_id Transaction ID
     */
    void set_transaction_id(const std::string& transaction_id);
    
    /**
     * @brief Convert the message to a bencode dictionary
     * 
     * @return Bencode dictionary representation of the message
     */
    virtual bencode::BencodeValue to_bencode() const;
    
    /**
     * @brief Convert the message to a bencode dictionary asynchronously
     * 
     * @return Future containing the bencode dictionary representation of the message
     */
    virtual std::future<bencode::BencodeValue> to_bencode_async() const;
    
    /**
     * @brief Encode the message to a byte vector
     * 
     * @return Byte vector containing the encoded message
     */
    std::vector<uint8_t> encode() const;
    
    /**
     * @brief Encode the message to a byte vector asynchronously
     * 
     * @return Future containing the byte vector with the encoded message
     */
    std::future<std::vector<uint8_t>> encode_async() const;
    
    /**
     * @brief Check if the message is valid
     * 
     * @return true if the message is valid, false otherwise
     */
    virtual bool is_valid() const;
    
    /**
     * @brief Get a string representation of the message
     * 
     * @return String representation of the message
     */
    virtual std::string to_string() const;
    
protected:
    /**
     * @brief Add common fields to a bencode dictionary
     * 
     * @param dict Dictionary to add fields to
     */
    void add_common_fields(bencode::BencodeValue& dict) const;
    
private:
    Type type_;                 ///< Message type
    std::string transaction_id_; ///< Transaction ID
};

/**
 * @brief DHT ping message
 * 
 * DHTPingMessage represents a ping query or response in the DHT protocol.
 */
class DHTPingMessage : public DHTMessage {
public:
    /**
     * @brief Create a ping query message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Sender's node ID
     */
    DHTPingMessage(const std::string& transaction_id, const types::NodeID& node_id);
    
    /**
     * @brief Create a ping response message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Responder's node ID
     * @param is_response Whether this is a response message
     */
    DHTPingMessage(const std::string& transaction_id, const types::NodeID& node_id, bool is_response);
    
    /**
     * @brief Get the node ID
     * 
     * @return Node ID
     */
    const types::NodeID& node_id() const;
    
    /**
     * @brief Set the node ID
     * 
     * @param node_id Node ID
     */
    void set_node_id(const types::NodeID& node_id);
    
    /**
     * @brief Convert the message to a bencode dictionary
     * 
     * @return Bencode dictionary representation of the message
     */
    bencode::BencodeValue to_bencode() const override;
    
    /**
     * @brief Check if the message is valid
     * 
     * @return true if the message is valid, false otherwise
     */
    bool is_valid() const override;
    
    /**
     * @brief Get a string representation of the message
     * 
     * @return String representation of the message
     */
    std::string to_string() const override;
    
private:
    types::NodeID node_id_; ///< Node ID
};

} // namespace bitscrape::dht
