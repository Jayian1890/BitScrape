#include "bitscrape/web/web_controller.hpp"
#include "bitscrape/web/json.hpp"
#include "bitscrape/storage/query_interface.hpp"
#include "bitscrape/types/event_types.hpp"
#include "bitscrape/bittorrent/bittorrent_event_processor.hpp"
#include "bitscrape/tracker/tracker_event.hpp"

#include <iostream>
#include <mutex>

namespace bitscrape::web {

WebController::WebController(const std::string& config_path)
    : controller_(std::make_shared<core::Controller>(config_path)),
      next_callback_id_(0) {
}

WebController::WebController(std::shared_ptr<core::Controller> controller)
    : controller_(controller),
      next_callback_id_(0) {
}

WebController::~WebController() {
    // We don't stop the controller here because it might be shared with the CLI
    // The CLI will handle stopping the controller
}

bool WebController::initialize() {
    // If the controller was created by this WebController, initialize it
    // Otherwise, assume it's already initialized
    return true;
}

bool WebController::start() {
    // If the controller was created by this WebController, start it
    // Otherwise, assume it's already started
    return true;
}

bool WebController::stop() {
    // Don't stop the controller as it might be shared with the CLI
    // The CLI will handle stopping the controller
    return true;
}

bool WebController::start_crawling() {
    return controller_->start_crawling();
}

bool WebController::stop_crawling() {
    return controller_->stop_crawling();
}

std::unordered_map<std::string, std::string> WebController::get_statistics() const {
    return controller_->get_statistics();
}

std::vector<storage::NodeModel> WebController::get_nodes(size_t limit, size_t offset) const {
    auto& storage_manager = controller_->get_storage_manager();
    auto query = storage_manager.query_interface();

    storage::QueryInterface::NodeQueryOptions options;
    options.limit = limit;
    options.offset = offset;
    options.order_by = "last_seen";
    options.order_desc = true;

    return query->get_nodes(options);
}

std::vector<storage::InfoHashModel> WebController::get_infohashes(size_t limit, size_t offset) const {
    auto& storage_manager = controller_->get_storage_manager();
    auto query = storage_manager.query_interface();

    storage::QueryInterface::InfoHashQueryOptions options;
    options.limit = limit;
    options.offset = offset;
    options.order_by = "last_seen";
    options.order_desc = true;

    return query->get_infohashes(options);
}

std::vector<storage::MetadataModel> WebController::get_metadata(size_t limit, size_t offset) const {
    auto& storage_manager = controller_->get_storage_manager();
    auto query = storage_manager.query_interface();

    storage::QueryInterface::MetadataQueryOptions options;
    options.limit = limit;
    options.offset = offset;
    options.order_by = "download_time";
    options.order_desc = true;

    return query->get_metadata_list(options);
}

std::optional<storage::MetadataModel> WebController::get_metadata_by_infohash(const types::InfoHash& info_hash) const {
    auto& storage_manager = controller_->get_storage_manager();
    auto query = storage_manager.query_interface();

    return query->get_metadata(info_hash);
}

std::vector<storage::FileModel> WebController::get_files(const types::InfoHash& info_hash) const {
    auto& storage_manager = controller_->get_storage_manager();
    auto query = storage_manager.query_interface();

    return query->get_files(info_hash);
}

std::vector<storage::PeerModel> WebController::get_peers(const types::InfoHash& info_hash) const {
    auto& storage_manager = controller_->get_storage_manager();
    auto query = storage_manager.query_interface();

    return query->get_peers(info_hash);
}

std::vector<storage::TrackerModel> WebController::get_trackers(const types::InfoHash& info_hash) const {
    auto& storage_manager = controller_->get_storage_manager();
    auto query = storage_manager.query_interface();

    return query->get_trackers(info_hash);
}

std::vector<storage::MetadataModel> WebController::search_metadata(const std::string& query, size_t limit, size_t offset) const {
    auto& storage_manager = controller_->get_storage_manager();
    auto query_interface = storage_manager.query_interface();

    storage::QueryInterface::MetadataQueryOptions options;
    options.name_contains = query;
    options.limit = limit;
    options.offset = offset;
    options.order_by = "download_time";
    options.order_desc = true;

    return query_interface->get_metadata_list(options);
}

size_t WebController::register_websocket_callback(WebSocketMessageCallback callback) {
    std::lock_guard<std::mutex> lock(websocket_callbacks_mutex_);
    size_t id = next_callback_id_++;
    websocket_callbacks_[id] = callback;
    return id;
}

void WebController::unregister_websocket_callback(size_t callback_id) {
    std::lock_guard<std::mutex> lock(websocket_callbacks_mutex_);
    websocket_callbacks_.erase(callback_id);
}

std::shared_ptr<core::Controller> WebController::get_controller() const {
    return controller_;
}

void WebController::handle_event(const types::Event& event) {
    // Convert event to JSON
    JSON json = JSON::object();
    json["type"] = static_cast<int>(event.type());
    json["timestamp"] = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
        event.timestamp().time_since_epoch()).count());

    // Add event-specific data based on event type
    switch (event.type()) {
        case types::Event::Type::DHT_NODE_FOUND:
            // For now, just add basic event info
            json["event_name"] = "DHT_NODE_FOUND";
            break;

        case types::Event::Type::DHT_INFOHASH_FOUND:
            // For now, just add basic event info
            json["event_name"] = "DHT_INFOHASH_FOUND";
            break;

        case types::Event::Type::BT_METADATA_RECEIVED:
            // For now, just add basic event info
            json["event_name"] = "BT_METADATA_RECEIVED";
            break;

        case types::Event::Type::BT_PEER_CONNECTED:
            // For now, just add basic event info
            json["event_name"] = "BT_PEER_CONNECTED";
            break;

        case types::Event::Type::TRACKER_ANNOUNCE_COMPLETE:
            // For now, just add basic event info
            json["event_name"] = "TRACKER_ANNOUNCE_COMPLETE";
            break;

        default:
            // For other event types, just include the string representation
            json["message"] = event.to_string();
            break;
    }

    // Serialize to string
    std::string message = json.dump();

    // Send to all WebSocket clients
    std::lock_guard<std::mutex> lock(websocket_callbacks_mutex_);
    for (const auto& [id, callback] : websocket_callbacks_) {
        callback(message);
    }
}

} // namespace bitscrape::web
