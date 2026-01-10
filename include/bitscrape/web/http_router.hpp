#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace bitscrape::web {

// Forward declarations
struct HTTPRequest;
struct HTTPResponse;
class WebController;

/**
 * @brief HTTP route handler function type
 */
using RouteHandler = std::function<HTTPResponse(const HTTPRequest&, std::shared_ptr<WebController>)>;

/**
 * @brief HTTP router class
 * 
 * This class provides routing functionality for the HTTP server.
 * It maps HTTP methods and paths to handler functions.
 */
class HTTPRouter {
public:
    /**
     * @brief Construct a new HTTPRouter object
     */
    HTTPRouter();

    /**
     * @brief Add a route
     * 
     * @param method The HTTP method (GET, POST, etc.)
     * @param path The path pattern
     * @param handler The handler function
     */
    void add_route(const std::string& method, const std::string& path, RouteHandler handler);

    /**
     * @brief Add a GET route
     * 
     * @param path The path pattern
     * @param handler The handler function
     */
    void get(const std::string& path, RouteHandler handler);

    /**
     * @brief Add a POST route
     * 
     * @param path The path pattern
     * @param handler The handler function
     */
    void post(const std::string& path, RouteHandler handler);

    /**
     * @brief Add a PUT route
     * 
     * @param path The path pattern
     * @param handler The handler function
     */
    void put(const std::string& path, RouteHandler handler);

    /**
     * @brief Add a DELETE route
     * 
     * @param path The path pattern
     * @param handler The handler function
     */
    void del(const std::string& path, RouteHandler handler);

    /**
     * @brief Find a route handler for a request
     * 
     * @param request The HTTP request
     * @return The handler function, or nullptr if not found
     */
    RouteHandler find_handler(HTTPRequest& request) const;

private:
    struct Route {
        std::string method;
        std::string path_pattern;
        RouteHandler handler;
    };

    bool match_route(const Route& route, const std::string& method, const std::string& path, std::map<std::string, std::string>& path_params) const;
    std::vector<std::string> split_path(const std::string& path) const;

    std::vector<Route> routes_;
};

} // namespace bitscrape::web
