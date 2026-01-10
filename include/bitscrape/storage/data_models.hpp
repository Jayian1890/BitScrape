#pragma once

#include <bitscrape/types/node_id.hpp>
#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/endpoint.hpp>
#include <bitscrape/types/metadata_info.hpp>
#include <bitscrape/types/torrent_info.hpp>
#include <bitscrape/storage/detail/database_result.hpp>

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <cstdint>

namespace bitscrape::storage {

// Forward declaration
class Database;

// Forward declaration for Database::Result
namespace detail {
    class DatabaseResult;
}

/**
 * @brief Node data model
 *
 * Represents a DHT node in the database.
 */
struct NodeModel {
    types::NodeID node_id;                                  ///< Node ID
    types::Endpoint endpoint;                               ///< Node endpoint (IP and port)
    std::chrono::system_clock::time_point first_seen;       ///< When the node was first seen
    std::chrono::system_clock::time_point last_seen;        ///< When the node was last seen
    uint32_t ping_count = 0;                                ///< Number of successful pings
    uint32_t query_count = 0;                               ///< Number of queries sent to this node
    uint32_t response_count = 0;                            ///< Number of responses received from this node
    bool is_responsive = false;                             ///< Whether the node is responsive

    /**
     * @brief Create a node model from a database result
     *
     * @param result Database result
     * @return Node model
     */
    static NodeModel from_db_result(const detail::DatabaseResult& result);

    // Friend declarations
    friend class Database;

    /**
     * @brief Convert to SQL parameters for insertion
     *
     * @return Vector of SQL parameters
     */
    std::vector<std::string> to_sql_params() const;
};

/**
 * @brief InfoHash data model
 *
 * Represents a BitTorrent infohash in the database.
 */
struct InfoHashModel {
    types::InfoHash info_hash;                              ///< InfoHash
    std::chrono::system_clock::time_point first_seen;       ///< When the infohash was first seen
    std::chrono::system_clock::time_point last_seen;        ///< When the infohash was last seen
    uint32_t announce_count = 0;                            ///< Number of announces for this infohash
    uint32_t peer_count = 0;                                ///< Number of peers for this infohash
    bool has_metadata = false;                              ///< Whether metadata is available

    /**
     * @brief Create an infohash model from a database result
     *
     * @param result Database result
     * @return InfoHash model
     */
    static InfoHashModel from_db_result(const detail::DatabaseResult& result);

    // Friend declarations
    friend class Database;

    /**
     * @brief Convert to SQL parameters for insertion
     *
     * @return Vector of SQL parameters
     */
    std::vector<std::string> to_sql_params() const;
};

/**
 * @brief Metadata data model
 *
 * Represents BitTorrent metadata in the database.
 */
struct MetadataModel {
    types::InfoHash info_hash;                              ///< InfoHash
    types::MetadataInfo metadata;                           ///< Metadata information
    std::chrono::system_clock::time_point download_time;    ///< When the metadata was downloaded
    std::string name;                                       ///< Torrent name
    uint64_t total_size = 0;                                ///< Total size in bytes
    uint32_t piece_count = 0;                               ///< Number of pieces
    uint32_t file_count = 0;                                ///< Number of files
    std::string comment;                                    ///< Torrent comment
    std::string created_by;                                 ///< Creator information
    std::optional<std::chrono::system_clock::time_point> creation_date; ///< Creation date

    /**
     * @brief Create a metadata model from a database result
     *
     * @param result Database result
     * @return Metadata model
     */
    static MetadataModel from_db_result(const detail::DatabaseResult& result);

    // Friend declarations
    friend class Database;

    /**
     * @brief Convert to SQL parameters for insertion
     *
     * @return Vector of SQL parameters
     */
    std::vector<std::string> to_sql_params() const;
};

/**
 * @brief File data model
 *
 * Represents a file in a torrent in the database.
 */
struct FileModel {
    types::InfoHash info_hash;                              ///< InfoHash
    std::string path;                                       ///< File path
    uint64_t size = 0;                                      ///< File size in bytes

    /**
     * @brief Create a file model from a database result
     *
     * @param result Database result
     * @return File model
     */
    static FileModel from_db_result(const detail::DatabaseResult& result);

    // Friend declarations
    friend class Database;

    /**
     * @brief Convert to SQL parameters for insertion
     *
     * @return Vector of SQL parameters
     */
    std::vector<std::string> to_sql_params() const;
};

/**
 * @brief Tracker data model
 *
 * Represents a tracker in the database.
 */
struct TrackerModel {
    types::InfoHash info_hash;                              ///< InfoHash
    std::string url;                                        ///< Tracker URL
    std::chrono::system_clock::time_point first_seen;       ///< When the tracker was first seen
    std::chrono::system_clock::time_point last_seen;        ///< When the tracker was last seen
    uint32_t announce_count = 0;                            ///< Number of announces to this tracker
    uint32_t scrape_count = 0;                              ///< Number of scrapes to this tracker

    /**
     * @brief Create a tracker model from a database result
     *
     * @param result Database result
     * @return Tracker model
     */
    static TrackerModel from_db_result(const detail::DatabaseResult& result);

    // Friend declarations
    friend class Database;

    /**
     * @brief Convert to SQL parameters for insertion
     *
     * @return Vector of SQL parameters
     */
    std::vector<std::string> to_sql_params() const;
};

/**
 * @brief Peer data model
 *
 * Represents a peer in the database.
 */
struct PeerModel {
    types::InfoHash info_hash;                              ///< InfoHash
    types::Endpoint endpoint;                               ///< Peer endpoint (IP and port)
    std::optional<types::NodeID> peer_id;                   ///< Peer ID (if available)
    std::chrono::system_clock::time_point first_seen;       ///< When the peer was first seen
    std::chrono::system_clock::time_point last_seen;        ///< When the peer was last seen
    bool supports_dht = false;                              ///< Whether the peer supports DHT
    bool supports_extension_protocol = false;               ///< Whether the peer supports the extension protocol
    bool supports_fast_protocol = false;                    ///< Whether the peer supports the fast protocol

    /**
     * @brief Create a peer model from a database result
     *
     * @param result Database result
     * @return Peer model
     */
    static PeerModel from_db_result(const detail::DatabaseResult& result);

    // Friend declarations
    friend class Database;

    /**
     * @brief Convert to SQL parameters for insertion
     *
     * @return Vector of SQL parameters
     */
    std::vector<std::string> to_sql_params() const;
};

} // namespace bitscrape::storage
