#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <optional>

#include "bitscrape/dht/dht_message.hpp"
#include "bitscrape/bencode/bencode_value.hpp"
#include "bitscrape/bencode/bencode_decoder.hpp"

namespace bitscrape::dht {

/**
 * @brief Factory class for creating and parsing DHT messages
 * 
 * DHTMessageFactory provides methods for creating DHT messages from raw data
 * and for creating specific types of DHT messages.
 */
class DHTMessageFactory {
public:
    /**
     * @brief Default constructor
     */
    DHTMessageFactory() = default;
    
    /**
     * @brief Create a DHT message from raw data
     * 
     * @param data Raw message data
     * @return Shared pointer to the created message, or nullptr if parsing failed
     */
    std::shared_ptr<DHTMessage> create_from_data(const std::vector<uint8_t>& data);
    
    /**
     * @brief Create a DHT message from raw data asynchronously
     * 
     * @param data Raw message data
     * @return Future containing a shared pointer to the created message, or nullptr if parsing failed
     */
    std::future<std::shared_ptr<DHTMessage>> create_from_data_async(const std::vector<uint8_t>& data);
    
    /**
     * @brief Create a DHT message from a bencode value
     * 
     * @param value Bencode value
     * @return Shared pointer to the created message, or nullptr if parsing failed
     */
    std::shared_ptr<DHTMessage> create_from_bencode(const bencode::BencodeValue& value);
    
    /**
     * @brief Create a DHT message from a bencode value asynchronously
     * 
     * @param value Bencode value
     * @return Future containing a shared pointer to the created message, or nullptr if parsing failed
     */
    std::future<std::shared_ptr<DHTMessage>> create_from_bencode_async(const bencode::BencodeValue& value);
    
    /**
     * @brief Create a ping query message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Sender's node ID
     * @return Shared pointer to the created message
     */
    std::shared_ptr<DHTPingMessage> create_ping(const std::string& transaction_id, const types::NodeID& node_id);
    
    /**
     * @brief Create a ping response message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Responder's node ID
     * @return Shared pointer to the created message
     */
    std::shared_ptr<DHTPingMessage> create_ping_response(const std::string& transaction_id, const types::NodeID& node_id);
    
    /**
     * @brief Generate a random transaction ID
     * 
     * @return Random transaction ID
     */
    static std::string generate_transaction_id();
    
private:
    /**
     * @brief Parse a ping query or response message
     * 
     * @param value Bencode value
     * @param transaction_id Transaction ID
     * @param is_response Whether this is a response message
     * @return Shared pointer to the created message, or nullptr if parsing failed
     */
    std::shared_ptr<DHTPingMessage> parse_ping(const bencode::BencodeValue& value, 
                                              const std::string& transaction_id, 
                                              bool is_response);
};

} // namespace bitscrape::dht
