#pragma once

#include "bitscrape/dht/dht_message.hpp"
#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/types/dht_token.hpp"

namespace bitscrape::dht {

/**
 * @brief DHT announce_peer message
 * 
 * DHTAnnouncePeerMessage represents an announce_peer query or response in the DHT protocol.
 * It is used to announce as a peer for a specific infohash.
 */
class DHTAnnouncePeerMessage : public DHTMessage {
public:
    /**
     * @brief Create an announce_peer query message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Sender's node ID
     * @param info_hash Infohash to announce for
     * @param port Port to announce
     * @param token Token received from a previous get_peers response
     * @param implied_port Whether to use the sender's port instead of the specified port
     */
    DHTAnnouncePeerMessage(const std::string& transaction_id, 
                          const types::NodeID& node_id, 
                          const types::InfoHash& info_hash,
                          uint16_t port,
                          const types::DHTToken& token,
                          bool implied_port = false);
    
    /**
     * @brief Create an announce_peer response message
     * 
     * @param transaction_id Transaction ID
     * @param node_id Responder's node ID
     */
    DHTAnnouncePeerMessage(const std::string& transaction_id, const types::NodeID& node_id);
    
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
     * @brief Get the port (query only)
     * 
     * @return Port
     */
    uint16_t port() const;
    
    /**
     * @brief Set the port (query only)
     * 
     * @param port Port
     */
    void set_port(uint16_t port);
    
    /**
     * @brief Get the token (query only)
     * 
     * @return Token
     */
    const types::DHTToken& token() const;
    
    /**
     * @brief Set the token (query only)
     * 
     * @param token Token
     */
    void set_token(const types::DHTToken& token);
    
    /**
     * @brief Get whether to use the sender's port (query only)
     * 
     * @return true if the sender's port should be used, false otherwise
     */
    bool implied_port() const;
    
    /**
     * @brief Set whether to use the sender's port (query only)
     * 
     * @param implied_port Whether to use the sender's port
     */
    void set_implied_port(bool implied_port);
    
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
    types::InfoHash info_hash_;                 ///< Infohash to announce for (query only)
    uint16_t port_;                             ///< Port to announce (query only)
    types::DHTToken token_;                     ///< Token received from a previous get_peers response (query only)
    bool implied_port_;                         ///< Whether to use the sender's port instead of the specified port (query only)
    bool is_response_;                          ///< Whether this is a response message
};

} // namespace bitscrape::dht
