#pragma once

#include "bitscrape/web/http_router.hpp"
#include "bitscrape/web/web_controller.hpp"

namespace bitscrape::web {

/**
 * @brief API handler class
 * 
 * This class provides handlers for API endpoints.
 */
class APIHandler {
public:
    /**
     * @brief Register API routes with the router
     * 
     * @param router The HTTP router
     */
    static void register_routes(HTTPRouter& router);

private:
    // Status endpoints
    static HTTPResponse handle_status(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    static HTTPResponse handle_statistics(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    
    // Crawling endpoints
    static HTTPResponse handle_start_crawling(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    static HTTPResponse handle_stop_crawling(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    
    // Data endpoints
    static HTTPResponse handle_get_nodes(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    static HTTPResponse handle_get_infohashes(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    static HTTPResponse handle_get_metadata(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    static HTTPResponse handle_get_metadata_by_infohash(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    static HTTPResponse handle_get_files(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    static HTTPResponse handle_get_peers(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    static HTTPResponse handle_get_trackers(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    
    // Search endpoints
    static HTTPResponse handle_search(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    
    // Helper methods
    static HTTPResponse create_json_response(const std::string& json, int status_code = 200);
    static HTTPResponse create_error_response(const std::string& message, int status_code = 400);
    static size_t parse_size_param(const std::map<std::string, std::string>& query_params, const std::string& param_name, size_t default_value);
};

} // namespace bitscrape::web
