#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <optional>

#include "bitscrape/dht/dht_message.hpp"
#include "bitscrape/dht/dht_get_peers_message.hpp"
#include "bitscrape/dht/dht_find_node_message.hpp"
#include "bitscrape/dht/dht_announce_peer_message.hpp"
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

    /**
     * @brief Create a get_peers query message
     *
     * @param transaction_id Transaction ID
     * @param node_id Sender's node ID
     * @param info_hash Target infohash to find peers for
     * @return Shared pointer to the created message
     */
    std::shared_ptr<DHTGetPeersMessage> create_get_peers(const std::string& transaction_id,
                                                       const types::NodeID& node_id,
                                                       const types::InfoHash& info_hash);

    /**
     * @brief Create a get_peers response message
     *
     * @param transaction_id Transaction ID
     * @param node_id Responder's node ID
     * @param token Token for future announce_peer
     * @param nodes List of nodes close to the target (optional)
     * @param values List of peer endpoints (optional)
     * @return Shared pointer to the created message
     */
    std::shared_ptr<DHTGetPeersMessage> create_get_peers_response(const std::string& transaction_id,
                                                                const types::NodeID& node_id,
                                                                const types::DHTToken& token,
                                                                const std::vector<types::DHTNode>& nodes = {},
                                                                const std::vector<types::Endpoint>& values = {});

    /**
     * @brief Create a find_node query message
     *
     * @param transaction_id Transaction ID
     * @param node_id Sender's node ID
     * @param target_id Target ID to find nodes close to
     * @return Shared pointer to the created message
     */
    std::shared_ptr<DHTFindNodeMessage> create_find_node(const std::string& transaction_id,
                                                      const types::NodeID& node_id,
                                                      const types::NodeID& target_id);

    /**
     * @brief Create a find_node response message
     *
     * @param transaction_id Transaction ID
     * @param node_id Responder's node ID
     * @param nodes List of nodes close to the target
     * @return Shared pointer to the created message
     */
    std::shared_ptr<DHTFindNodeMessage> create_find_node_response(const std::string& transaction_id,
                                                               const types::NodeID& node_id,
                                                               const std::vector<types::DHTNode>& nodes);

    /**
     * @brief Create an announce_peer query message
     *
     * @param transaction_id Transaction ID
     * @param node_id Sender's node ID
     * @param info_hash Infohash to announce for
     * @param port Port to announce
     * @param token Token received from a previous get_peers response
     * @param implied_port Whether to use the sender's port instead of the specified port
     * @return Shared pointer to the created message
     */
    std::shared_ptr<DHTAnnouncePeerMessage> create_announce_peer(const std::string& transaction_id,
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
     * @return Shared pointer to the created message
     */
    std::shared_ptr<DHTAnnouncePeerMessage> create_announce_peer_response(const std::string& transaction_id,
                                                                       const types::NodeID& node_id);

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

    /**
     * @brief Parse a get_peers query or response message
     *
     * @param value Bencode value
     * @param transaction_id Transaction ID
     * @param is_response Whether this is a response message
     * @return Shared pointer to the created message, or nullptr if parsing failed
     */
    std::shared_ptr<DHTGetPeersMessage> parse_get_peers(const bencode::BencodeValue& value,
                                                      const std::string& transaction_id,
                                                      bool is_response);

    /**
     * @brief Parse a find_node query or response message
     *
     * @param value Bencode value
     * @param transaction_id Transaction ID
     * @param is_response Whether this is a response message
     * @return Shared pointer to the created message, or nullptr if parsing failed
     */
    std::shared_ptr<DHTFindNodeMessage> parse_find_node(const bencode::BencodeValue& value,
                                                      const std::string& transaction_id,
                                                      bool is_response);

    /**
     * @brief Parse an announce_peer query or response message
     *
     * @param value Bencode value
     * @param transaction_id Transaction ID
     * @param is_response Whether this is a response message
     * @return Shared pointer to the created message, or nullptr if parsing failed
     */
    std::shared_ptr<DHTAnnouncePeerMessage> parse_announce_peer(const bencode::BencodeValue& value,
                                                             const std::string& transaction_id,
                                                             bool is_response);

    /**
     * @brief Parse an error message
     *
     * @param value Bencode value
     * @param transaction_id Transaction ID
     * @return Shared pointer to the created message, or nullptr if parsing failed
     */
    std::shared_ptr<DHTMessage> parse_error(const bencode::BencodeValue& value,
                                          const std::string& transaction_id);
};

} // namespace bitscrape::dht
