#pragma once

#include <bitscrape/types/node_id.hpp>
#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/endpoint.hpp>
#include <bitscrape/types/metadata_info.hpp>
#include <bitscrape/types/torrent_info.hpp>

#include <string>
#include <memory>
#include <future>
#include <vector>
#include <optional>
#include <functional>

namespace bitscrape::storage {

// Forward declarations
class Database;
class QueryInterface;

// Forward declarations for data models
struct NodeModel;
struct InfoHashModel;
struct MetadataModel;
struct FileModel;
struct TrackerModel;
struct PeerModel;

/**
 * @brief Storage manager for the BitScrape application
 *
 * Provides a high-level interface for storing and retrieving data.
 */
class StorageManager {
public:
    /**
     * @brief Create a storage manager
     *
     * @param db_path Path to the database file. If empty, a default path will be used.
     * @param persistent Whether to persist the database to disk. Always true for disk-based storage.
     */
    explicit StorageManager(const std::string& db_path, bool persistent = true);

    /**
     * @brief Destructor
     */
    ~StorageManager();

    /**
     * @brief Initialize the storage manager
     *
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Initialize the storage manager asynchronously
     *
     * @return Future with the result of the initialization
     */
    std::future<bool> initialize_async();

    /**
     * @brief Close the storage manager
     *
     * @return true if successful, false otherwise
     */
    bool close();

    /**
     * @brief Close the storage manager asynchronously
     *
     * @return Future with the result of the close operation
     */
    std::future<bool> close_async();

    /**
     * @brief Store a node
     *
     * @param node_id Node ID
     * @param endpoint Node endpoint
     * @param is_responsive Whether the node is responsive
     * @return true if successful, false otherwise
     */
    bool store_node(const types::NodeID& node_id, const types::Endpoint& endpoint, bool is_responsive = true, std::optional<uint32_t> rtt_ms = std::nullopt);

    /**
     * @brief Store a node asynchronously
     *
     * @param node_id Node ID
     * @param endpoint Node endpoint
     * @param is_responsive Whether the node is responsive
     * @return Future with the result of the operation
     */
    std::future<bool> store_node_async(const types::NodeID& node_id, const types::Endpoint& endpoint, bool is_responsive = true, std::optional<uint32_t> rtt_ms = std::nullopt);

    /**
     * @brief Update a node's responsiveness
     *
     * @param node_id Node ID
     * @param is_responsive Whether the node is responsive
     * @return true if successful, false otherwise
     */
    bool update_node_responsiveness(const types::NodeID& node_id, bool is_responsive);

    /**
     * @brief Update a node's responsiveness asynchronously
     *
     * @param node_id Node ID
     * @param is_responsive Whether the node is responsive
     * @return Future with the result of the operation
     */
    std::future<bool> update_node_responsiveness_async(const types::NodeID& node_id, bool is_responsive);

    /**
     * @brief Increment a node's ping count
     *
     * @param node_id Node ID
     * @return true if successful, false otherwise
     */
    bool increment_node_ping_count(const types::NodeID& node_id);

    /**
     * @brief Increment a node's ping count asynchronously
     *
     * @param node_id Node ID
     * @return Future with the result of the operation
     */
    std::future<bool> increment_node_ping_count_async(const types::NodeID& node_id);

    /**
     * @brief Increment a node's query count
     *
     * @param node_id Node ID
     * @return true if successful, false otherwise
     */
    bool increment_node_query_count(const types::NodeID& node_id);

    /**
     * @brief Increment a node's query count asynchronously
     *
     * @param node_id Node ID
     * @return Future with the result of the operation
     */
    std::future<bool> increment_node_query_count_async(const types::NodeID& node_id);

    /**
     * @brief Increment a node's response count
     *
     * @param node_id Node ID
     * @return true if successful, false otherwise
     */
    bool increment_node_response_count(const types::NodeID& node_id);

    /**
     * @brief Increment a node's response count asynchronously
     *
     * @param node_id Node ID
     * @return Future with the result of the operation
     */
    std::future<bool> increment_node_response_count_async(const types::NodeID& node_id);

    /**
     * @brief Store an infohash
     *
     * @param info_hash InfoHash
     * @return true if successful, false otherwise
     */
    bool store_infohash(const types::InfoHash& info_hash);

    /**
     * @brief Store an infohash asynchronously
     *
     * @param info_hash InfoHash
     * @return Future with the result of the operation
     */
    std::future<bool> store_infohash_async(const types::InfoHash& info_hash);

    /**
     * @brief Increment an infohash's announce count
     *
     * @param info_hash InfoHash
     * @return true if successful, false otherwise
     */
    bool increment_infohash_announce_count(const types::InfoHash& info_hash);

    /**
     * @brief Increment an infohash's announce count asynchronously
     *
     * @param info_hash InfoHash
     * @return Future with the result of the operation
     */
    std::future<bool> increment_infohash_announce_count_async(const types::InfoHash& info_hash);

    /**
     * @brief Increment an infohash's peer count
     *
     * @param info_hash InfoHash
     * @return true if successful, false otherwise
     */
    bool increment_infohash_peer_count(const types::InfoHash& info_hash);

    /**
     * @brief Increment an infohash's peer count asynchronously
     *
     * @param info_hash InfoHash
     * @return Future with the result of the operation
     */
    std::future<bool> increment_infohash_peer_count_async(const types::InfoHash& info_hash);

    /**
     * @brief Store metadata for an infohash
     *
     * @param info_hash InfoHash
     * @param metadata Metadata information
     * @return true if successful, false otherwise
     */
    bool store_metadata(const types::InfoHash& info_hash, const types::MetadataInfo& metadata);

    /**
     * @brief Store metadata for an infohash asynchronously
     *
     * @param info_hash InfoHash
     * @param metadata Metadata information
     * @return Future with the result of the operation
     */
    std::future<bool> store_metadata_async(const types::InfoHash& info_hash, const types::MetadataInfo& metadata);

    /**
     * @brief Store a torrent
     *
     * @param info_hash InfoHash
     * @param torrent_info Torrent information
     * @return true if successful, false otherwise
     */
    bool store_torrent(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info);

    /**
     * @brief Store a torrent asynchronously
     *
     * @param info_hash InfoHash
     * @param torrent_info Torrent information
     * @return Future with the result of the operation
     */
    std::future<bool> store_torrent_async(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info);

    /**
     * @brief Store a peer
     *
     * @param info_hash InfoHash
     * @param endpoint Peer endpoint
     * @param peer_id Peer ID (if available)
     * @param supports_dht Whether the peer supports DHT
     * @param supports_extension_protocol Whether the peer supports the extension protocol
     * @param supports_fast_protocol Whether the peer supports the fast protocol
     * @return true if successful, false otherwise
     */
    bool store_peer(const types::InfoHash& info_hash, const types::Endpoint& endpoint,
                   const std::optional<types::NodeID>& peer_id = std::nullopt,
                   bool supports_dht = false,
                   bool supports_extension_protocol = false,
                   bool supports_fast_protocol = false);

    /**
     * @brief Store a peer asynchronously
     *
     * @param info_hash InfoHash
     * @param endpoint Peer endpoint
     * @param peer_id Peer ID (if available)
     * @param supports_dht Whether the peer supports DHT
     * @param supports_extension_protocol Whether the peer supports the extension protocol
     * @param supports_fast_protocol Whether the peer supports the fast protocol
     * @return Future with the result of the operation
     */
    std::future<bool> store_peer_async(const types::InfoHash& info_hash, const types::Endpoint& endpoint,
                                      const std::optional<types::NodeID>& peer_id = std::nullopt,
                                      bool supports_dht = false,
                                      bool supports_extension_protocol = false,
                                      bool supports_fast_protocol = false);

    /**
     * @brief Store a tracker
     *
     * @param info_hash InfoHash
     * @param url Tracker URL
     * @return true if successful, false otherwise
     */
    bool store_tracker(const types::InfoHash& info_hash, const std::string& url);

    /**
     * @brief Store a tracker asynchronously
     *
     * @param info_hash InfoHash
     * @param url Tracker URL
     * @return Future with the result of the operation
     */
    std::future<bool> store_tracker_async(const types::InfoHash& info_hash, const std::string& url);

    /**
     * @brief Increment a tracker's announce count
     *
     * @param info_hash InfoHash
     * @param url Tracker URL
     * @return true if successful, false otherwise
     */
    bool increment_tracker_announce_count(const types::InfoHash& info_hash, const std::string& url);

    /**
     * @brief Increment a tracker's announce count asynchronously
     *
     * @param info_hash InfoHash
     * @param url Tracker URL
     * @return Future with the result of the operation
     */
    std::future<bool> increment_tracker_announce_count_async(const types::InfoHash& info_hash, const std::string& url);

    /**
     * @brief Increment a tracker's scrape count
     *
     * @param info_hash InfoHash
     * @param url Tracker URL
     * @return true if successful, false otherwise
     */
    bool increment_tracker_scrape_count(const types::InfoHash& info_hash, const std::string& url);

    /**
     * @brief Increment a tracker's scrape count asynchronously
     *
     * @param info_hash InfoHash
     * @param url Tracker URL
     * @return Future with the result of the operation
     */
    std::future<bool> increment_tracker_scrape_count_async(const types::InfoHash& info_hash, const std::string& url);

    /**
     * @brief Get the query interface
     *
     * @return Query interface
     */
    std::shared_ptr<QueryInterface> query_interface() const;

    /**
     * @brief Get the database
     *
     * @return Database
     */
    std::shared_ptr<Database> database() const;



    /**
     * @brief Get statistics about the stored data
     *
     * @return Map of statistics
     */
    std::unordered_map<std::string, std::string> get_statistics() const;

    /**
     * @brief Get statistics about the stored data asynchronously
     *
     * @return Future with a map of statistics
     */
    std::future<std::unordered_map<std::string, std::string>> get_statistics_async() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Disable copy
    StorageManager(const StorageManager&) = delete;
    StorageManager& operator=(const StorageManager&) = delete;
};

/**
 * @brief Create a new storage manager
 *
 * @param db_path Path to the database file. If empty, a default path will be used.
 * @param persistent Whether to persist the database to disk. Always true for disk-based storage.
 * @return Shared pointer to a new storage manager
 */
std::shared_ptr<StorageManager> create_storage_manager(const std::string& db_path, bool persistent = true);

} // namespace bitscrape::storage
