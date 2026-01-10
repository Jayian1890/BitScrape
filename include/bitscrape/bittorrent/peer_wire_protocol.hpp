#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <future>
#include <mutex>
#include <atomic>
#include <functional>
#include <unordered_map>

#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/bittorrent/peer_connection.hpp"
#include "bitscrape/bittorrent/peer_message.hpp"

namespace bitscrape::bittorrent {

/**
 * @brief Implements the BitTorrent peer wire protocol
 *
 * This class provides high-level functionality for the BitTorrent peer wire protocol,
 * including message handling, state management, and protocol extensions.
 */
class PeerWireProtocol {
public:
    /**
     * @brief Constructor
     *
     * @param info_hash Torrent info hash
     * @param peer_id Local peer ID
     */
    PeerWireProtocol(const types::InfoHash& info_hash, const std::vector<uint8_t>& peer_id);

    /**
     * @brief Destructor
     */
    ~PeerWireProtocol();

    /**
     * @brief Connect to a peer
     *
     * @param address Peer address
     * @return True if connection was successful, false otherwise
     */
    bool connect_to_peer(const network::Address& address);

    /**
     * @brief Connect to a peer asynchronously
     *
     * @param address Peer address
     * @return Future that will be completed when the connection is established
     */
    std::future<bool> connect_to_peer_async(const network::Address& address);

    /**
     * @brief Disconnect from a peer
     *
     * @param address Peer address
     */
    void disconnect_from_peer(const network::Address& address);

    /**
     * @brief Disconnect from all peers
     */
    void disconnect_all_peers();

    /**
     * @brief Send a message to a peer
     *
     * @param address Peer address
     * @param message Message to send
     * @return True if the message was sent successfully, false otherwise
     */
    bool send_message(const network::Address& address, const PeerMessage& message);

    /**
     * @brief Send a message to a peer asynchronously
     *
     * @param address Peer address
     * @param message Message to send
     * @return Future that will be completed when the message is sent
     */
    std::future<bool> send_message_async(const network::Address& address, const PeerMessage& message);

    /**
     * @brief Send raw data to a peer
     *
     * @param address Peer address
     * @param data Raw data to send
     * @return True if the data was sent successfully, false otherwise
     */
    bool send_raw_data(const network::Address& address, const std::vector<uint8_t>& data);

    /**
     * @brief Register a message handler
     *
     * @param type Message type to handle
     * @param handler Function to call when a message of the specified type is received
     */
    void register_message_handler(PeerMessageType type,
                                 std::function<void(const network::Address&, const PeerMessage&)> handler);

    /**
     * @brief Get the info hash
     *
     * @return Info hash
     */
    const types::InfoHash& info_hash() const;

    /**
     * @brief Get the peer ID
     *
     * @return Peer ID
     */
    const std::vector<uint8_t>& peer_id() const;

    /**
     * @brief Get the list of connected peers
     *
     * @return List of peer addresses
     */
    std::vector<network::Address> connected_peers() const;

    /**
     * @brief Check if a peer is connected
     *
     * @param address Peer address
     * @return True if the peer is connected, false otherwise
     */
    bool is_peer_connected(const network::Address& address) const;

private:
    /**
     * @brief Process a received message
     *
     * @param address Peer address
     * @param message Received message
     */
    void process_message(const network::Address& address, const PeerMessage& message);

    /**
     * @brief Start the message receive loop for a peer
     *
     * @param connection Peer connection
     */
    void start_receive_loop(std::shared_ptr<PeerConnection> connection);

    types::InfoHash info_hash_;               ///< Torrent info hash
    std::vector<uint8_t> peer_id_;            ///< Local peer ID
    std::unordered_map<std::string, std::shared_ptr<PeerConnection>> connections_; ///< Active connections
    mutable std::mutex connections_mutex_;    ///< Mutex for connections map

    std::unordered_map<PeerMessageType,
                      std::function<void(const network::Address&, const PeerMessage&)>>
        message_handlers_;                    ///< Message handlers
    mutable std::mutex handlers_mutex_;       ///< Mutex for message handlers
};

} // namespace bitscrape::bittorrent
