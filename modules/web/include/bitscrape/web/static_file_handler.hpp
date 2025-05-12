#pragma once

#include "bitscrape/web/http_router.hpp"
#include "bitscrape/web/web_controller.hpp"

#include <string>

namespace bitscrape::web {

/**
 * @brief Static file handler class
 * 
 * This class provides handlers for serving static files.
 */
class StaticFileHandler {
public:
    /**
     * @brief Register static file routes with the router
     * 
     * @param router The HTTP router
     * @param static_dir The directory containing static files
     */
    static void register_routes(HTTPRouter& router, const std::string& static_dir);

private:
    static HTTPResponse handle_static_file(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    static HTTPResponse handle_index(const HTTPRequest& request, std::shared_ptr<WebController> controller);
    
    static std::string static_dir_;
    static std::unordered_map<std::string, std::string> mime_types_;
    
    static std::string get_mime_type(const std::string& file_path);
    static HTTPResponse create_file_response(const std::string& file_path);
    static HTTPResponse create_not_found_response();
};

} // namespace bitscrape::web
