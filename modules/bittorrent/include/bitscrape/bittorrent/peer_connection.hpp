#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <future>
#include <mutex>
#include <atomic>
#include <functional>

#include "bitscrape/network/tcp_socket.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/bittorrent/peer_message.hpp"

namespace bitscrape::bittorrent {

/**
 * @brief Represents a connection to a BitTorrent peer
 *
 * This class handles the connection to a BitTorrent peer, including
 * handshaking, message sending/receiving, and connection state management.
 */
class PeerConnection {
public:
    /**
     * @brief Connection state
     */
    enum class State {
        DISCONNECTED,    ///< Not connected
        CONNECTING,      ///< Connection in progress
        HANDSHAKING,     ///< Handshake in progress
        CONNECTED,       ///< Connected and handshaked
        DISCONNECTING    ///< Disconnection in progress
    };

    /**
     * @brief Constructor
     *
     * @param address Peer address
     * @param info_hash Torrent info hash
     * @param peer_id Local peer ID
     */
    PeerConnection(network::Address address,
                  types::InfoHash info_hash,
                  std::vector<uint8_t> peer_id);

    /**
     * @brief Destructor
     */
    ~PeerConnection();

    /**
     * @brief Connect to the peer
     *
     * @return True if connection was successful, false otherwise
     */
    bool connect();

    /**
     * @brief Connect to the peer asynchronously
     *
     * @return Future that will be completed when the connection is established
     */
    std::future<bool> connect_async();

    /**
     * @brief Disconnect from the peer
     */
    void disconnect();

    /**
     * @brief Send a message to the peer
     *
     * @param message Message to send
     * @return True if the message was sent successfully, false otherwise
     */
    bool send_message(const PeerMessage& message);

    /**
     * @brief Send a message to the peer asynchronously
     *
     * @param message Message to send
     * @return Future that will be completed when the message is sent
     */
    std::future<bool> send_message_async(const PeerMessage& message);

    /**
     * @brief Receive a message from the peer
     *
     * @return Received message, or nullptr if no message was received
     */
    std::shared_ptr<PeerMessage> receive_message();

    /**
     * @brief Receive a message from the peer asynchronously
     *
     * @return Future that will be completed when a message is received
     */
    std::future<std::shared_ptr<PeerMessage>> receive_message_async();

    /**
     * @brief Get the current connection state
     *
     * @return Connection state
     */
    State state() const;

    /**
     * @brief Get the peer address
     *
     * @return Peer address
     */
    const network::Address& address() const;

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
     * @brief Get the remote peer ID
     *
     * @return Remote peer ID, or empty vector if not connected
     */
    const std::vector<uint8_t>& remote_peer_id() const;

    /**
     * @brief Check if the peer is choked
     *
     * @return True if the peer is choked, false otherwise
     */
    bool is_choked() const;

    /**
     * @brief Check if the peer is interested
     *
     * @return True if the peer is interested, false otherwise
     */
    bool is_interested() const;

    /**
     * @brief Check if we are choked by the peer
     *
     * @return True if we are choked by the peer, false otherwise
     */
    bool am_choked() const;

    /**
     * @brief Check if we are interested in the peer
     *
     * @return True if we are interested in the peer, false otherwise
     */
    bool am_interested() const;

    /**
     * @brief Send raw data to the peer
     *
     * @param data The data to send
     * @param size The size of the data
     * @return The number of bytes sent, or -1 on error
     */
    int send_raw_data(const uint8_t* data, size_t size);

private:
    /**
     * @brief Perform the BitTorrent handshake
     *
     * @return True if handshake was successful, false otherwise
     */
    bool handshake();

    /**
     * @brief Process a received message
     *
     * @param message Received message
     */
    void process_message(const PeerMessage& message);

    network::Address address_;                ///< Peer address
    types::InfoHash info_hash_;               ///< Torrent info hash
    std::vector<uint8_t> peer_id_;            ///< Local peer ID
    std::vector<uint8_t> remote_peer_id_;     ///< Remote peer ID
    std::unique_ptr<network::TCPSocket> socket_; ///< TCP socket
    std::atomic<State> state_;                ///< Connection state
    std::mutex mutex_;                        ///< Mutex for thread safety

    std::atomic<bool> peer_choked_;           ///< Whether the peer is choked
    std::atomic<bool> peer_interested_;       ///< Whether the peer is interested
    std::atomic<bool> am_choked_;             ///< Whether we are choked by the peer
    std::atomic<bool> am_interested_;         ///< Whether we are interested in the peer
};

} // namespace bitscrape::bittorrent
