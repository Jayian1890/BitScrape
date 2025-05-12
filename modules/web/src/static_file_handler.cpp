#include "bitscrape/web/static_file_handler.hpp"
#include "bitscrape/web/http_server.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace bitscrape::web {

std::string StaticFileHandler::static_dir_;
std::unordered_map<std::string, std::string> StaticFileHandler::mime_types_ = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".ttf", "font/ttf"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".eot", "application/vnd.ms-fontobject"},
    {".otf", "font/otf"},
    {".txt", "text/plain"},
    {".md", "text/markdown"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {".webm", "video/webm"},
    {".ogg", "audio/ogg"},
    {".wav", "audio/wav"},
    {".webp", "image/webp"}
};

void StaticFileHandler::register_routes(HTTPRouter& router, const std::string& static_dir) {
    static_dir_ = static_dir;
    
    // Register routes
    router.get("/", handle_index);
    router.get("/*", handle_static_file);
}

HTTPResponse StaticFileHandler::handle_static_file(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Get path from request
    std::string path = request.path;
    
    // Remove leading slash
    if (!path.empty() && path[0] == '/') {
        path = path.substr(1);
    }
    
    // If path is empty, serve index.html
    if (path.empty()) {
        path = "index.html";
    }
    
    // Construct file path
    std::string file_path = static_dir_ + "/" + path;
    
    // Check if file exists
    if (!std::filesystem::exists(file_path)) {
        // Try index.html in the directory
        if (std::filesystem::is_directory(file_path)) {
            file_path += "/index.html";
            if (!std::filesystem::exists(file_path)) {
                return create_not_found_response();
            }
        } else {
            return create_not_found_response();
        }
    }
    
    // Serve the file
    return create_file_response(file_path);
}

HTTPResponse StaticFileHandler::handle_index(const HTTPRequest& request, std::shared_ptr<WebController> controller) {
    // Serve index.html
    std::string file_path = static_dir_ + "/index.html";
    
    // Check if file exists
    if (!std::filesystem::exists(file_path)) {
        return create_not_found_response();
    }
    
    // Serve the file
    return create_file_response(file_path);
}

std::string StaticFileHandler::get_mime_type(const std::string& file_path) {
    // Get file extension
    std::string extension = std::filesystem::path(file_path).extension().string();
    
    // Convert to lowercase
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Look up MIME type
    auto it = mime_types_.find(extension);
    if (it != mime_types_.end()) {
        return it->second;
    }
    
    // Default MIME type
    return "application/octet-stream";
}

HTTPResponse StaticFileHandler::create_file_response(const std::string& file_path) {
    // Open file
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return create_not_found_response();
    }
    
    // Read file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Create response
    HTTPResponse response;
    response.status_code = 200;
    response.status_message = "OK";
    response.headers["Content-Type"] = get_mime_type(file_path);
    response.body = network::Buffer(
        reinterpret_cast<const uint8_t*>(content.c_str()),
        content.length()
    );
    
    return response;
}

HTTPResponse StaticFileHandler::create_not_found_response() {
    HTTPResponse response;
    response.status_code = 404;
    response.status_message = "Not Found";
    response.headers["Content-Type"] = "text/plain";
    response.body = network::Buffer(
        reinterpret_cast<const uint8_t*>("File not found"),
        13
    );
    
    return response;
}

} // namespace bitscrape::web
