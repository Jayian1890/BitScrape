#pragma once

#include <bitscrape/storage/data_models.hpp>

#include <string>
#include <vector>
#include <optional>
#include <future>
#include <memory>
#include <functional>

namespace bitscrape::storage {

/**
 * @brief Query interface for the storage module
 *
 * Provides a high-level interface for querying the database.
 */
class QueryInterface {
public:
    /**
     * @brief Query options for node queries
     */
    struct NodeQueryOptions {
        std::optional<std::chrono::system_clock::time_point> min_last_seen; ///< Minimum last seen time
        std::optional<std::chrono::system_clock::time_point> max_last_seen; ///< Maximum last seen time
        std::optional<bool> is_responsive;                                  ///< Whether the node is responsive
        std::optional<uint32_t> min_ping_count;                             ///< Minimum ping count
        std::optional<uint32_t> min_response_count;                         ///< Minimum response count
        std::optional<uint32_t> limit;                                      ///< Maximum number of results
        std::optional<uint32_t> offset;                                     ///< Offset for pagination
        std::optional<std::string> order_by;                                ///< Field to order by
        std::optional<bool> order_desc;                                     ///< Whether to order in descending order
    };

    /**
     * @brief Query options for infohash queries
     */
    struct InfoHashQueryOptions {
        std::optional<std::chrono::system_clock::time_point> min_last_seen; ///< Minimum last seen time
        std::optional<std::chrono::system_clock::time_point> max_last_seen; ///< Maximum last seen time
        std::optional<bool> has_metadata;                                   ///< Whether metadata is available
        std::optional<uint32_t> min_announce_count;                         ///< Minimum announce count
        std::optional<uint32_t> min_peer_count;                             ///< Minimum peer count
        std::optional<uint32_t> limit;                                      ///< Maximum number of results
        std::optional<uint32_t> offset;                                     ///< Offset for pagination
        std::optional<std::string> order_by;                                ///< Field to order by
        std::optional<bool> order_desc;                                     ///< Whether to order in descending order
    };

    /**
     * @brief Query options for metadata queries
     */
    struct MetadataQueryOptions {
        std::optional<std::string> name_contains;                           ///< Name contains this string
        std::optional<uint64_t> min_size;                                   ///< Minimum total size
        std::optional<uint64_t> max_size;                                   ///< Maximum total size
        std::optional<uint32_t> min_file_count;                             ///< Minimum file count
        std::optional<std::chrono::system_clock::time_point> min_download_time; ///< Minimum download time
        std::optional<std::chrono::system_clock::time_point> max_download_time; ///< Maximum download time
        std::optional<uint32_t> limit;                                      ///< Maximum number of results
        std::optional<uint32_t> offset;                                     ///< Offset for pagination
        std::optional<std::string> order_by;                                ///< Field to order by
        std::optional<bool> order_desc;                                     ///< Whether to order in descending order
    };

    /**
     * @brief Create a query interface
     *
     * @param database Database instance
     */
    explicit QueryInterface(std::shared_ptr<Database> database);

    /**
     * @brief Destructor
     */
    ~QueryInterface();

    /**
     * @brief Get a node by ID
     *
     * @param node_id Node ID
     * @return Node model if found, std::nullopt otherwise
     */
    std::optional<NodeModel> get_node(const types::NodeID& node_id);

    /**
     * @brief Get a node by ID asynchronously
     *
     * @param node_id Node ID
     * @return Future with the node model if found, std::nullopt otherwise
     */
    std::future<std::optional<NodeModel>> get_node_async(const types::NodeID& node_id);

    /**
     * @brief Get nodes by query options
     *
     * @param options Query options
     * @return Vector of node models
     */
    std::vector<NodeModel> get_nodes(const NodeQueryOptions& options = {});

    /**
     * @brief Get nodes by query options asynchronously
     *
     * @param options Query options
     * @return Future with a vector of node models
     */
    std::future<std::vector<NodeModel>> get_nodes_async(const NodeQueryOptions& options = {});

    /**
     * @brief Get an infohash by value
     *
     * @param info_hash InfoHash
     * @return InfoHash model if found, std::nullopt otherwise
     */
    std::optional<InfoHashModel> get_infohash(const types::InfoHash& info_hash);

    /**
     * @brief Get an infohash by value asynchronously
     *
     * @param info_hash InfoHash
     * @return Future with the infohash model if found, std::nullopt otherwise
     */
    std::future<std::optional<InfoHashModel>> get_infohash_async(const types::InfoHash& info_hash);

    /**
     * @brief Get infohashes by query options
     *
     * @param options Query options
     * @return Vector of infohash models
     */
    std::vector<InfoHashModel> get_infohashes(const InfoHashQueryOptions& options = {});

    /**
     * @brief Get infohashes by query options asynchronously
     *
     * @param options Query options
     * @return Future with a vector of infohash models
     */
    std::future<std::vector<InfoHashModel>> get_infohashes_async(const InfoHashQueryOptions& options = {});

    /**
     * @brief Get metadata by infohash
     *
     * @param info_hash InfoHash
     * @return Metadata model if found, std::nullopt otherwise
     */
    std::optional<MetadataModel> get_metadata(const types::InfoHash& info_hash);

    /**
     * @brief Get metadata by infohash asynchronously
     *
     * @param info_hash InfoHash
     * @return Future with the metadata model if found, std::nullopt otherwise
     */
    std::future<std::optional<MetadataModel>> get_metadata_async(const types::InfoHash& info_hash);

    /**
     * @brief Get metadata by query options
     *
     * @param options Query options
     * @return Vector of metadata models
     */
    std::vector<MetadataModel> get_metadata_list(const MetadataQueryOptions& options = {});

    /**
     * @brief Get metadata by query options asynchronously
     *
     * @param options Query options
     * @return Future with a vector of metadata models
     */
    std::future<std::vector<MetadataModel>> get_metadata_list_async(const MetadataQueryOptions& options = {});

    /**
     * @brief Get files for an infohash
     *
     * @param info_hash InfoHash
     * @return Vector of file models
     */
    std::vector<FileModel> get_files(const types::InfoHash& info_hash);

    /**
     * @brief Get files for an infohash asynchronously
     *
     * @param info_hash InfoHash
     * @return Future with a vector of file models
     */
    std::future<std::vector<FileModel>> get_files_async(const types::InfoHash& info_hash);

    /**
     * @brief Get trackers for an infohash
     *
     * @param info_hash InfoHash
     * @return Vector of tracker models
     */
    std::vector<TrackerModel> get_trackers(const types::InfoHash& info_hash);

    /**
     * @brief Get trackers for an infohash asynchronously
     *
     * @param info_hash InfoHash
     * @return Future with a vector of tracker models
     */
    std::future<std::vector<TrackerModel>> get_trackers_async(const types::InfoHash& info_hash);

    /**
     * @brief Get peers for an infohash
     *
     * @param info_hash InfoHash
     * @return Vector of peer models
     */
    std::vector<PeerModel> get_peers(const types::InfoHash& info_hash);

    /**
     * @brief Get peers for an infohash asynchronously
     *
     * @param info_hash InfoHash
     * @return Future with a vector of peer models
     */
    std::future<std::vector<PeerModel>> get_peers_async(const types::InfoHash& info_hash);

    /**
     * @brief Count nodes
     *
     * @param options Query options
     * @return Number of nodes
     */
    uint64_t count_nodes(const NodeQueryOptions& options = {});

    /**
     * @brief Count nodes asynchronously
     *
     * @param options Query options
     * @return Future with the number of nodes
     */
    std::future<uint64_t> count_nodes_async(const NodeQueryOptions& options = {});

    /**
     * @brief Count infohashes
     *
     * @param options Query options
     * @return Number of infohashes
     */
    uint64_t count_infohashes(const InfoHashQueryOptions& options = {});

    /**
     * @brief Count infohashes asynchronously
     *
     * @param options Query options
     * @return Future with the number of infohashes
     */
    std::future<uint64_t> count_infohashes_async(const InfoHashQueryOptions& options = {});

    /**
     * @brief Count metadata
     *
     * @param options Query options
     * @return Number of metadata entries
     */
    uint64_t count_metadata(const MetadataQueryOptions& options = {});

    /**
     * @brief Count metadata asynchronously
     *
     * @param options Query options
     * @return Future with the number of metadata entries
     */
    std::future<uint64_t> count_metadata_async(const MetadataQueryOptions& options = {});

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Disable copy
    QueryInterface(const QueryInterface&) = delete;
    QueryInterface& operator=(const QueryInterface&) = delete;
};

} // namespace bitscrape::storage
