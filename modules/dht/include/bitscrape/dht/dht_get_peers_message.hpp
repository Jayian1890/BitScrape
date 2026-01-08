#pragma once

#include "bitscrape/dht/dht_message.hpp"
#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/info_hash.hpp"

namespace bitscrape::dht {

/**
 * @brief DHT get_peers message
 * 
 * DHTGetPeersMessage represents a get_peers query or response in the DHT protocol.
 * It is used to find peers for a specific infohash.
 */
class DHTGetPeersMessage : public DHTMessage {
public:
    /**
     * @brief Create a get_peers query message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Sender's node ID
     * @param info_hash Target infohash to find peers for
     */
    DHTGetPeersMessage(const std::string& transaction_id, const types::NodeID& node_id, const types::InfoHash& info_hash);
    
    /**
     * @brief Create a get_peers response message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Responder's node ID
     * @param token Token for future announce_peer
     * @param nodes List of nodes close to the target (optional)
     * @param values List of peer endpoints (optional)
     */
    DHTGetPeersMessage(const std::string& transaction_id, 
                      const types::NodeID& node_id, 
                      const types::DHTToken& token,
                      const std::vector<types::DHTNode>& nodes = {},
                      const std::vector<types::Endpoint>& values = {});
    
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
     * @brief Get the infohash (query only)
     * 
     * @return Infohash
     */
    const types::InfoHash& info_hash() const;
    
    /**
     * @brief Set the infohash (query only)
     * 
     * @param info_hash Infohash
     */
    void set_info_hash(const types::InfoHash& info_hash);
    
    /**
     * @brief Get the token (response only)
     * 
     * @return Token
     */
    const types::DHTToken& token() const;
    
    /**
     * @brief Set the token (response only)
     * 
     * @param token Token
     */
    void set_token(const types::DHTToken& token);
    
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
     * @brief Get the values (response only)
     * 
     * @return List of peer endpoints
     */
    const std::vector<types::Endpoint>& values() const;
    
    /**
     * @brief Set the values (response only)
     * 
     * @param values List of peer endpoints
     */
    void set_values(const std::vector<types::Endpoint>& values);
    
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
    types::InfoHash info_hash_;                 ///< Target infohash (query only)
    types::DHTToken token_;                     ///< Token for future announce_peer (response only)
    std::vector<types::DHTNode> nodes_;         ///< List of nodes close to the target (response only)
    std::vector<types::Endpoint> values_;       ///< List of peer endpoints (response only)
    [[maybe_unused]] bool is_response_;         ///< Whether this is a response message
};

} // namespace bitscrape::dht
