#include "bitscrape/web/api_handler.hpp"
#include "bitscrape/web/http_server.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/web/json.hpp"

#include <sstream>
#include <string>

namespace bitscrape::web {

void APIHandler::register_routes(HTTPRouter& router) {
    // Status endpoints
    router.get("/api/status", handle_status);
    router.get("/api/statistics", handle_statistics);

    // Crawling endpoints
    router.post("/api/crawling/start", handle_start_crawling);
    router.post("/api/crawling/stop", handle_stop_crawling);

    // Data endpoints
    router.get("/api/nodes", handle_get_nodes);
    router.get("/api/infohashes", handle_get_infohashes);
    router.get("/api/metadata", handle_get_metadata);
    router.get("/api/metadata/:infohash", handle_get_metadata_by_infohash);
    router.get("/api/files/:infohash", handle_get_files);
    router.get("/api/peers/:infohash", handle_get_peers);
    router.get("/api/trackers/:infohash", handle_get_trackers);

    // Search endpoints
    router.get("/api/search", handle_search);
}

HTTPResponse APIHandler::handle_status(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    JSON json = JSON::object();

    // Get statistics
    auto stats = controller->get_statistics();

    // Extract status information
    json["running"] = (stats["controller.running"] == "true");
    json["crawling"] = (stats["controller.crawling"] == "true");

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_statistics(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    JSON json = JSON::object();

    // Get statistics
    auto stats = controller->get_statistics();

    // Convert to JSON
    for (const auto& [key, value] : stats) {
        json[key] = value;
    }

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_start_crawling(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    JSON json = JSON::object();

    // Start crawling
    bool success = controller->start_crawling();

    // Create response
    json["success"] = success;

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_stop_crawling(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    JSON json = JSON::object();

    // Stop crawling
    bool success = controller->stop_crawling();

    // Create response
    json["success"] = success;

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_get_nodes(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Parse query parameters
    size_t limit = parse_size_param(request.query_params, "limit", 100);
    size_t offset = parse_size_param(request.query_params, "offset", 0);

    // Get nodes
    auto nodes = controller->get_nodes(limit, offset);

    // Convert to JSON
    JSON json = JSON::array();
    for (const auto& node : nodes) {
        JSON node_json = JSON::object();
        node_json["node_id"] = node.node_id.to_hex();
        node_json["ip"] = node.endpoint.address();
        node_json["port"] = node.endpoint.port();
        node_json["first_seen"] = static_cast<long>(std::chrono::system_clock::to_time_t(node.first_seen));
        node_json["last_seen"] = static_cast<long>(std::chrono::system_clock::to_time_t(node.last_seen));
        node_json["ping_count"] = node.ping_count;
        node_json["query_count"] = node.query_count;
        node_json["response_count"] = node.response_count;
        node_json["is_responsive"] = node.is_responsive;

        json.push_back(node_json);
    }

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_get_infohashes(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Parse query parameters
    size_t limit = parse_size_param(request.query_params, "limit", 100);
    size_t offset = parse_size_param(request.query_params, "offset", 0);

    // Get infohashes
    auto infohashes = controller->get_infohashes(limit, offset);

    // Convert to JSON
    JSON json = JSON::array();
    for (const auto& infohash : infohashes) {
        JSON infohash_json = JSON::object();
        infohash_json["info_hash"] = infohash.info_hash.to_hex();
        infohash_json["first_seen"] = static_cast<long>(std::chrono::system_clock::to_time_t(infohash.first_seen));
        infohash_json["last_seen"] = static_cast<long>(std::chrono::system_clock::to_time_t(infohash.last_seen));
        // Node count is not available in the InfoHashModel
        infohash_json["node_count"] = 0;
        infohash_json["peer_count"] = infohash.peer_count;
        infohash_json["has_metadata"] = infohash.has_metadata;

        json.push_back(infohash_json);
    }

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_get_metadata(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Parse query parameters
    size_t limit = parse_size_param(request.query_params, "limit", 100);
    size_t offset = parse_size_param(request.query_params, "offset", 0);

    // Get metadata
    auto metadata_list = controller->get_metadata(limit, offset);

    // Convert to JSON
    JSON json = JSON::array();
    for (const auto& metadata : metadata_list) {
        JSON metadata_json = JSON::object();
        metadata_json["info_hash"] = metadata.info_hash.to_hex();
        metadata_json["name"] = metadata.name;
        metadata_json["total_size"] = metadata.total_size;
        metadata_json["piece_count"] = metadata.piece_count;
        metadata_json["file_count"] = metadata.file_count;
        metadata_json["comment"] = metadata.comment;
        metadata_json["download_time"] = static_cast<long>(std::chrono::system_clock::to_time_t(metadata.download_time));

        json.push_back(metadata_json);
    }

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_get_metadata_by_infohash(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Get infohash from path parameters
    auto it = request.path_params.find("infohash");
    if (it == request.path_params.end()) {
        return create_error_response("Missing infohash parameter");
    }

    // Parse infohash
    types::InfoHash info_hash;
    try {
        info_hash = types::InfoHash::from_hex(it->second);
    } catch (const std::exception& e) {
        return create_error_response("Invalid infohash: " + std::string(e.what()));
    }

    // Get metadata
    auto metadata_opt = controller->get_metadata_by_infohash(info_hash);
    if (!metadata_opt) {
        return create_error_response("Metadata not found", 404);
    }

    // Convert to JSON
    const auto& metadata = *metadata_opt;
    JSON json = JSON::object();
    json["info_hash"] = metadata.info_hash.to_hex();
    json["name"] = metadata.name;
    json["total_size"] = metadata.total_size;
    json["piece_count"] = metadata.piece_count;
    json["file_count"] = metadata.file_count;
    json["comment"] = metadata.comment;
    json["download_time"] = static_cast<long>(std::chrono::system_clock::to_time_t(metadata.download_time));

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_get_files(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Get infohash from path parameters
    auto it = request.path_params.find("infohash");
    if (it == request.path_params.end()) {
        return create_error_response("Missing infohash parameter");
    }

    // Parse infohash
    types::InfoHash info_hash;
    try {
        info_hash = types::InfoHash::from_hex(it->second);
    } catch (const std::exception& e) {
        return create_error_response("Invalid infohash: " + std::string(e.what()));
    }

    // Get files
    auto files = controller->get_files(info_hash);

    // Convert to JSON
    JSON json = JSON::array();
    for (const auto& file : files) {
        JSON file_json = JSON::object();
        file_json["info_hash"] = file.info_hash.to_hex();
        file_json["path"] = file.path;
        file_json["size"] = file.size;

        json.push_back(file_json);
    }

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_get_peers(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Get infohash from path parameters
    auto it = request.path_params.find("infohash");
    if (it == request.path_params.end()) {
        return create_error_response("Missing infohash parameter");
    }

    // Parse infohash
    types::InfoHash info_hash;
    try {
        info_hash = types::InfoHash::from_hex(it->second);
    } catch (const std::exception& e) {
        return create_error_response("Invalid infohash: " + std::string(e.what()));
    }

    // Get peers
    auto peers = controller->get_peers(info_hash);

    // Convert to JSON
    JSON json = JSON::array();
    for (const auto& peer : peers) {
        JSON peer_json = JSON::object();
        peer_json["info_hash"] = peer.info_hash.to_hex();
        peer_json["ip"] = peer.endpoint.address();
        peer_json["port"] = peer.endpoint.port();
        if (peer.peer_id) {
            peer_json["peer_id"] = peer.peer_id->to_hex();
        }
        peer_json["first_seen"] = std::chrono::system_clock::to_time_t(peer.first_seen);
        peer_json["last_seen"] = std::chrono::system_clock::to_time_t(peer.last_seen);
        peer_json["supports_dht"] = peer.supports_dht;
        peer_json["supports_extension_protocol"] = peer.supports_extension_protocol;
        peer_json["supports_fast_protocol"] = peer.supports_fast_protocol;

        json.push_back(peer_json);
    }

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_get_trackers(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Get infohash from path parameters
    auto it = request.path_params.find("infohash");
    if (it == request.path_params.end()) {
        return create_error_response("Missing infohash parameter");
    }

    // Parse infohash
    types::InfoHash info_hash;
    try {
        info_hash = types::InfoHash::from_hex(it->second);
    } catch (const std::exception& e) {
        return create_error_response("Invalid infohash: " + std::string(e.what()));
    }

    // Get trackers
    auto trackers = controller->get_trackers(info_hash);

    // Convert to JSON
    JSON json = JSON::array();
    for (const auto& tracker : trackers) {
        JSON tracker_json = JSON::object();
        tracker_json["info_hash"] = tracker.info_hash.to_hex();
        tracker_json["url"] = tracker.url;
        tracker_json["announce_count"] = tracker.announce_count;
        tracker_json["scrape_count"] = tracker.scrape_count;
        tracker_json["last_seen"] = static_cast<long>(std::chrono::system_clock::to_time_t(tracker.last_seen));
        tracker_json["first_seen"] = static_cast<long>(std::chrono::system_clock::to_time_t(tracker.first_seen));

        json.push_back(tracker_json);
    }

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::handle_search(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Get query parameter
    auto it = request.query_params.find("q");
    if (it == request.query_params.end()) {
        return create_error_response("Missing query parameter");
    }

    // Parse query parameters
    size_t limit = parse_size_param(request.query_params, "limit", 100);
    size_t offset = parse_size_param(request.query_params, "offset", 0);

    // Search metadata
    auto metadata_list = controller->search_metadata(it->second, limit, offset);

    // Convert to JSON
    JSON json = JSON::array();
    for (const auto& metadata : metadata_list) {
        JSON metadata_json = JSON::object();
        metadata_json["info_hash"] = metadata.info_hash.to_hex();
        metadata_json["name"] = metadata.name;
        metadata_json["total_size"] = metadata.total_size;
        metadata_json["piece_count"] = metadata.piece_count;
        metadata_json["file_count"] = metadata.file_count;
        metadata_json["comment"] = metadata.comment;
        metadata_json["download_time"] = std::chrono::system_clock::to_time_t(metadata.download_time);

        json.push_back(metadata_json);
    }

    return create_json_response(json.dump());
}

HTTPResponse APIHandler::create_json_response(const std::string& json, int status_code) {
    HTTPResponse response;
    response.status_code = status_code;
    response.status_message = "OK";
    response.headers["Content-Type"] = "application/json";
    response.body = network::Buffer(
        reinterpret_cast<const uint8_t*>(json.c_str()),
        json.length()
    );

    return response;
}

HTTPResponse APIHandler::create_error_response(const std::string& message, int status_code) {
    JSON json = JSON::object();
    json["error"] = message;

    HTTPResponse response;
    response.status_code = status_code;
    response.status_message = "Error";
    response.headers["Content-Type"] = "application/json";
    response.body = network::Buffer(
        reinterpret_cast<const uint8_t*>(json.dump().c_str()),
        json.dump().length()
    );

    return response;
}

size_t APIHandler::parse_size_param(const std::map<std::string, std::string>& query_params, const std::string& param_name, size_t default_value) {
    auto it = query_params.find(param_name);
    if (it == query_params.end()) {
        return default_value;
    }

    try {
        return std::stoul(it->second);
    } catch (const std::exception&) {
        return default_value;
    }
}

} // namespace bitscrape::web
