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

    // Check segment count
    if (route_segments.size() != path_segments.size()) {
        // Special case: route ends with a parameter
        if (!route_segments.empty() && route_segments.back() == "*") {
            // Match all remaining segments
            if (route_segments.size() >= 2 && path_segments.size() >= 1) {
                // Safe to access route_segments.size() - 2
                size_t prev_segment_index = route_segments.size() - 2;
                if (prev_segment_index < path_segments.size()) {
                    path_params["*"] = path.substr(path.find(path_segments[prev_segment_index]) + path_segments[prev_segment_index].length());
                } else {
                    // Handle case where path doesn't have enough segments
                    path_params["*"] = "";
                }
            } else {
                // Handle the case where there's only one segment (e.g., /*)
                path_params["*"] = path;
            }
            return true;
        }

        return false;
    }

    // Check each segment
    for (size_t i = 0; i < route_segments.size(); ++i) {
        const std::string& route_segment = route_segments[i];
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

    return true;
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
