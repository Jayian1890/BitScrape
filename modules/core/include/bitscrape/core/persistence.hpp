#pragma once

#include <string>
#include <vector>
#include <memory>
#include <future>
#include <optional>
#include <functional>

#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/node_id.hpp>
#include <bitscrape/types/endpoint.hpp>
#include <bitscrape/types/metadata_info.hpp>
#include <bitscrape/types/torrent_info.hpp>

namespace bitscrape::core {

/**
 * @brief The Persistence class manages data storage and retrieval.
 * 
 * It provides methods for storing and retrieving discovered nodes, infohashes,
 * and metadata.
 */
class Persistence {
public:
    /**
     * @brief Construct a new Persistence object
     * 
     * @param db_path Path to the database file
     */
    explicit Persistence(const std::string& db_path = "bitscrape.db");

    /**
     * @brief Destroy the Persistence object
     */
    ~Persistence();

    /**
     * @brief Initialize the persistence layer
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();

    /**
     * @brief Initialize the persistence layer asynchronously
     * 
     * @return std::future<bool> Future that will contain the result of initialization
     */
    std::future<bool> initialize_async();

    /**
     * @brief Close the persistence layer
     * 
     * @return true if closing was successful
     * @return false if closing failed
     */
    bool close();

    /**
     * @brief Close the persistence layer asynchronously
     * 
     * @return std::future<bool> Future that will contain the result of closing
     */
    std::future<bool> close_async();

    /**
     * @brief Store a node in the database
     * 
     * @param node_id Node ID
     * @param endpoint Node endpoint
     * @return true if storing was successful
     * @return false if storing failed
     */
    bool store_node(const types::NodeID& node_id, const types::Endpoint& endpoint);

    /**
     * @brief Store a node in the database asynchronously
     * 
     * @param node_id Node ID
     * @param endpoint Node endpoint
     * @return std::future<bool> Future that will contain the result of storing
     */
    std::future<bool> store_node_async(const types::NodeID& node_id, const types::Endpoint& endpoint);

    /**
     * @brief Store an infohash in the database
     * 
     * @param info_hash Infohash
     * @return true if storing was successful
     * @return false if storing failed
     */
    bool store_infohash(const types::InfoHash& info_hash);

    /**
     * @brief Store an infohash in the database asynchronously
     * 
     * @param info_hash Infohash
     * @return std::future<bool> Future that will contain the result of storing
     */
    std::future<bool> store_infohash_async(const types::InfoHash& info_hash);

    /**
     * @brief Store metadata in the database
     * 
     * @param info_hash Infohash
     * @param metadata Metadata
     * @return true if storing was successful
     * @return false if storing failed
     */
    bool store_metadata(const types::InfoHash& info_hash, const types::MetadataInfo& metadata);

    /**
     * @brief Store metadata in the database asynchronously
     * 
     * @param info_hash Infohash
     * @param metadata Metadata
     * @return std::future<bool> Future that will contain the result of storing
     */
    std::future<bool> store_metadata_async(const types::InfoHash& info_hash, const types::MetadataInfo& metadata);

    /**
     * @brief Store torrent info in the database
     * 
     * @param info_hash Infohash
     * @param torrent_info Torrent info
     * @return true if storing was successful
     * @return false if storing failed
     */
    bool store_torrent_info(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info);

    /**
     * @brief Store torrent info in the database asynchronously
     * 
     * @param info_hash Infohash
     * @param torrent_info Torrent info
     * @return std::future<bool> Future that will contain the result of storing
     */
    std::future<bool> store_torrent_info_async(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info);

    /**
     * @brief Get a node from the database
     * 
     * @param node_id Node ID
     * @return std::optional<types::Endpoint> Node endpoint if found, std::nullopt otherwise
     */
    std::optional<types::Endpoint> get_node(const types::NodeID& node_id) const;

    /**
     * @brief Get a node from the database asynchronously
     * 
     * @param node_id Node ID
     * @return std::future<std::optional<types::Endpoint>> Future that will contain the node endpoint if found
     */
    std::future<std::optional<types::Endpoint>> get_node_async(const types::NodeID& node_id) const;

    /**
     * @brief Get nodes from the database
     * 
     * @param limit Maximum number of nodes to return
     * @param offset Offset to start from
     * @return std::vector<std::pair<types::NodeID, types::Endpoint>> Vector of node ID and endpoint pairs
     */
    std::vector<std::pair<types::NodeID, types::Endpoint>> get_nodes(size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Get nodes from the database asynchronously
     * 
     * @param limit Maximum number of nodes to return
     * @param offset Offset to start from
     * @return std::future<std::vector<std::pair<types::NodeID, types::Endpoint>>> Future that will contain the nodes
     */
    std::future<std::vector<std::pair<types::NodeID, types::Endpoint>>> get_nodes_async(size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Get infohashes from the database
     * 
     * @param limit Maximum number of infohashes to return
     * @param offset Offset to start from
     * @return std::vector<types::InfoHash> Vector of infohashes
     */
    std::vector<types::InfoHash> get_infohashes(size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Get infohashes from the database asynchronously
     * 
     * @param limit Maximum number of infohashes to return
     * @param offset Offset to start from
     * @return std::future<std::vector<types::InfoHash>> Future that will contain the infohashes
     */
    std::future<std::vector<types::InfoHash>> get_infohashes_async(size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Get metadata from the database
     * 
     * @param info_hash Infohash
     * @return std::optional<types::MetadataInfo> Metadata if found, std::nullopt otherwise
     */
    std::optional<types::MetadataInfo> get_metadata(const types::InfoHash& info_hash) const;

    /**
     * @brief Get metadata from the database asynchronously
     * 
     * @param info_hash Infohash
     * @return std::future<std::optional<types::MetadataInfo>> Future that will contain the metadata if found
     */
    std::future<std::optional<types::MetadataInfo>> get_metadata_async(const types::InfoHash& info_hash) const;

    /**
     * @brief Get torrent info from the database
     * 
     * @param info_hash Infohash
     * @return std::optional<types::TorrentInfo> Torrent info if found, std::nullopt otherwise
     */
    std::optional<types::TorrentInfo> get_torrent_info(const types::InfoHash& info_hash) const;

    /**
     * @brief Get torrent info from the database asynchronously
     * 
     * @param info_hash Infohash
     * @return std::future<std::optional<types::TorrentInfo>> Future that will contain the torrent info if found
     */
    std::future<std::optional<types::TorrentInfo>> get_torrent_info_async(const types::InfoHash& info_hash) const;

    /**
     * @brief Get statistics about the database
     * 
     * @return std::unordered_map<std::string, std::string> Map of statistics
     */
    std::unordered_map<std::string, std::string> get_statistics() const;

private:
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace bitscrape::core
