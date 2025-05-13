#pragma once

#include "bitscrape/dht/dht_message.hpp"
#include "bitscrape/types/node_id.hpp"

namespace bitscrape::dht {

/**
 * @brief DHT find_node message
 * 
 * DHTFindNodeMessage represents a find_node query or response in the DHT protocol.
 * It is used to find nodes close to a target ID.
 */
class DHTFindNodeMessage : public DHTMessage {
public:
    /**
     * @brief Create a find_node query message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Sender's node ID
     * @param target_id Target ID to find nodes close to
     */
    DHTFindNodeMessage(const std::string& transaction_id, const types::NodeID& node_id, const types::NodeID& target_id);
    
    /**
     * @brief Create a find_node response message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Responder's node ID
     * @param nodes List of nodes close to the target
     */
    DHTFindNodeMessage(const std::string& transaction_id, 
                      const types::NodeID& node_id, 
                      const std::vector<types::DHTNode>& nodes);
    
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
     * @brief Get the target ID (query only)
     * 
     * @return Target ID
     */
    const types::NodeID& target_id() const;
    
    /**
     * @brief Set the target ID (query only)
     * 
     * @param target_id Target ID
     */
    void set_target_id(const types::NodeID& target_id);
    
    /**
     * @brief Get the nodes (response only)
     * 
     * @return List of nodes
     */
    const std::vector<types::DHTNode>& nodes() const;
    
    /**
     * @brief Set the nodes (response only)
     * 
     * @param nodes List of nodes
     */
    void set_nodes(const std::vector<types::DHTNode>& nodes);
    
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
    types::NodeID node_id_;                     ///< Node ID
    types::NodeID target_id_;                   ///< Target ID (query only)
    std::vector<types::DHTNode> nodes_;         ///< List of nodes close to the target (response only)
    bool is_response_;                          ///< Whether this is a response message
};

} // namespace bitscrape::dht
