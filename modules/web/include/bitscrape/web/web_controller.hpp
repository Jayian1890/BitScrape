#pragma once

#include "bitscrape/core/controller.hpp"
#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/types/endpoint.hpp"
#include "bitscrape/types/metadata_info.hpp"
#include "bitscrape/storage/data_models.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace bitscrape::web {

/**
 * @brief WebSocket message callback function type
 */
using WebSocketMessageCallback = std::function<void(const std::string&)>;

/**
 * @brief Web controller class
 * 
 * This class wraps the core Controller and provides methods for the web interface.
 */
class WebController {
public:
    /**
     * @brief Construct a new WebController object
     * 
     * @param config_path Path to the configuration file
     */
    explicit WebController(const std::string& config_path);

    /**
     * @brief Destroy the WebController object
     */
    ~WebController();

    /**
     * @brief Initialize the controller
     * 
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Start the controller
     * 
     * @return true if successful, false otherwise
     */
    bool start();

    /**
     * @brief Stop the controller
     * 
     * @return true if successful, false otherwise
     */
    bool stop();

    /**
     * @brief Start crawling
     * 
     * @return true if successful, false otherwise
     */
    bool start_crawling();

    /**
     * @brief Stop crawling
     * 
     * @return true if successful, false otherwise
     */
    bool stop_crawling();

    /**
     * @brief Get statistics
     * 
     * @return Statistics map
     */
    std::unordered_map<std::string, std::string> get_statistics() const;

    /**
     * @brief Get nodes
     * 
     * @param limit Maximum number of nodes to return
     * @param offset Offset to start from
     * @return Vector of node models
     */
    std::vector<storage::NodeModel> get_nodes(size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Get infohashes
     * 
     * @param limit Maximum number of infohashes to return
     * @param offset Offset to start from
     * @return Vector of infohash models
     */
    std::vector<storage::InfoHashModel> get_infohashes(size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Get metadata
     * 
     * @param limit Maximum number of metadata entries to return
     * @param offset Offset to start from
     * @return Vector of metadata models
     */
    std::vector<storage::MetadataModel> get_metadata(size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Get metadata by infohash
     * 
     * @param info_hash The infohash
     * @return Metadata model if found, std::nullopt otherwise
     */
    std::optional<storage::MetadataModel> get_metadata_by_infohash(const types::InfoHash& info_hash) const;

    /**
     * @brief Get files for an infohash
     * 
     * @param info_hash The infohash
     * @return Vector of file models
     */
    std::vector<storage::FileModel> get_files(const types::InfoHash& info_hash) const;

    /**
     * @brief Get peers for an infohash
     * 
     * @param info_hash The infohash
     * @return Vector of peer models
     */
    std::vector<storage::PeerModel> get_peers(const types::InfoHash& info_hash) const;

    /**
     * @brief Get trackers for an infohash
     * 
     * @param info_hash The infohash
     * @return Vector of tracker models
     */
    std::vector<storage::TrackerModel> get_trackers(const types::InfoHash& info_hash) const;

    /**
     * @brief Search for metadata by name
     * 
     * @param query The search query
     * @param limit Maximum number of results to return
     * @param offset Offset to start from
     * @return Vector of metadata models
     */
    std::vector<storage::MetadataModel> search_metadata(const std::string& query, size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Register a WebSocket message callback
     * 
     * @param callback The callback function
     * @return Callback ID
     */
    size_t register_websocket_callback(WebSocketMessageCallback callback);

    /**
     * @brief Unregister a WebSocket message callback
     * 
     * @param callback_id The callback ID
     */
    void unregister_websocket_callback(size_t callback_id);

    /**
     * @brief Get the core controller
     * 
     * @return The core controller
     */
    std::shared_ptr<core::Controller> get_controller() const;

private:
    void handle_event(const types::Event& event);

    std::shared_ptr<core::Controller> controller_;
    std::unordered_map<size_t, WebSocketMessageCallback> websocket_callbacks_;
    std::mutex websocket_callbacks_mutex_;
    size_t next_callback_id_;
};

} // namespace bitscrape::web
