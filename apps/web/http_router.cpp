#include "bitscrape/web/http_router.hpp"
#include "bitscrape/web/http_server.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace bitscrape::web {

HTTPRouter::HTTPRouter() {
}

void HTTPRouter::add_route(const std::string& method, const std::string& path, RouteHandler handler) {
    routes_.push_back({method, path, handler});
}

void HTTPRouter::get(const std::string& path, RouteHandler handler) {
    add_route("GET", path, handler);
}

void HTTPRouter::post(const std::string& path, RouteHandler handler) {
    add_route("POST", path, handler);
}

void HTTPRouter::put(const std::string& path, RouteHandler handler) {
    add_route("PUT", path, handler);
}

void HTTPRouter::del(const std::string& path, RouteHandler handler) {
    add_route("DELETE", path, handler);
}

RouteHandler HTTPRouter::find_handler(HTTPRequest& request) const {
    for (const auto& route : routes_) {
        if (match_route(route, request.method, request.path, request.path_params)) {
            return route.handler;
        }
    }

    return nullptr;
}

bool HTTPRouter::match_route(const Route& route, const std::string& method, const std::string& path, std::map<std::string, std::string>& path_params) const {
    // Check method
    if (route.method != method) {
        return false;
    }

    // Split paths into segments
    std::vector<std::string> route_segments = split_path(route.path_pattern);
    std::vector<std::string> path_segments = split_path(path);

    // Check each segment
    for (size_t i = 0; i < route_segments.size(); ++i) {
        const std::string& route_segment = route_segments[i];

        // Check if segment is a wildcard
        if (route_segment == "*") {
            // Match all remaining segments
            if (i < path_segments.size()) {
                // Find where this segment starts in the original path
                size_t pos = 0;
                for (size_t j = 0; j < i; ++j) {
                    pos = path.find('/', pos + 1);
                }
                path_params["*"] = path.substr(pos + 1);
            } else {
                path_params["*"] = "";
            }
            return true;
        }

        // Check if we have a path segment to match
        if (i >= path_segments.size()) {
            return false;
        }

        const std::string& path_segment = path_segments[i];

        // Check if segment is a parameter
        if (!route_segment.empty() && route_segment[0] == ':') {
            // Extract parameter name
            std::string param_name = route_segment.substr(1);

            // Store parameter value
            path_params[param_name] = path_segment;
        } else if (route_segment != path_segment) {
            // Segments don't match
            return false;
        }
    }

    // All segments matched, check if path has extra segments
    return route_segments.size() == path_segments.size();
}

std::vector<std::string> HTTPRouter::split_path(const std::string& path) const {
    std::vector<std::string> segments;
    std::istringstream stream(path);
    std::string segment;

    // Skip leading slash
    if (!path.empty() && path[0] == '/') {
        stream.ignore(1);
    }

    // Split by slash
    while (std::getline(stream, segment, '/')) {
        segments.push_back(segment);
    }

    return segments;
}

} // namespace bitscrape::web
