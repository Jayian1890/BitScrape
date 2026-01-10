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
#include <unordered_set>

#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/network/address.hpp"
#include "bitscrape/bittorrent/peer_wire_protocol.hpp"

namespace bitscrape::bittorrent {

/**
 * @brief Manages BitTorrent peers for a torrent
 *
 * This class provides functionality for discovering, connecting to, and
 * managing BitTorrent peers for a specific torrent.
 */
class PeerManager {
public:
    /**
     * @brief Constructor
     *
     * @param info_hash Torrent info hash
     * @param peer_id Local peer ID
     * @param max_connections Maximum number of simultaneous connections
     */
    PeerManager(const types::InfoHash& info_hash,
               const std::vector<uint8_t>& peer_id,
               int max_connections = 50);

    /**
     * @brief Destructor
     */
    ~PeerManager();

    /**
     * @brief Start the peer manager
     *
     * @return True if started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Start the peer manager asynchronously
     *
     * @return Future that will be completed when the peer manager is started
     */
    std::future<bool> start_async();

    /**
     * @brief Stop the peer manager
     */
    void stop();

    /**
     * @brief Add a peer to the manager
     *
     * @param address Peer address
     */
    void add_peer(const network::Address& address);

    /**
     * @brief Add multiple peers to the manager
     *
     * @param addresses List of peer addresses
     */
    void add_peers(const std::vector<network::Address>& addresses);

    /**
     * @brief Remove a peer from the manager
     *
     * @param address Peer address
     */
    void remove_peer(const network::Address& address);

    /**
     * @brief Get the list of known peers
     *
     * @return List of peer addresses
     */
    std::vector<network::Address> known_peers() const;

    /**
     * @brief Get the list of connected peers
     *
     * @return List of peer addresses
     */
    std::vector<network::Address> connected_peers() const;

    /**
     * @brief Get the peer wire protocol instance
     *
     * @return Peer wire protocol instance
     */
    PeerWireProtocol& protocol();

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
     * @brief Get the maximum number of connections
     *
     * @return Maximum number of connections
     */
    int max_connections() const;

    /**
     * @brief Set the maximum number of connections
     *
     * @param max_connections Maximum number of connections
     */
    void set_max_connections(int max_connections);

private:
    /**
     * @brief Connect to peers
     *
     * This method attempts to connect to peers until the maximum number of
     * connections is reached or there are no more peers to connect to.
     */
    void connect_to_peers();

    /**
     * @brief Manage connections
     *
     * This method periodically checks the status of connections and
     * reconnects to peers as needed.
     */
    void manage_connections();

    types::InfoHash info_hash_;               ///< Torrent info hash
    std::vector<uint8_t> peer_id_;            ///< Local peer ID
    std::unique_ptr<PeerWireProtocol> protocol_; ///< Peer wire protocol instance

    std::unordered_set<std::string> known_peers_; ///< Known peers
    mutable std::mutex peers_mutex_;          ///< Mutex for peers set

    std::atomic<int> max_connections_;        ///< Maximum number of connections
    std::atomic<bool> running_;               ///< Whether the peer manager is running

    std::thread connection_thread_;           ///< Thread for managing connections
};

} // namespace bitscrape::bittorrent
