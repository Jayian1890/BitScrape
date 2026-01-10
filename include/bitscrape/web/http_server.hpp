#pragma once

#include "bitscrape/network/tcp_listener.hpp"
#include "bitscrape/network/buffer.hpp"
#include "bitscrape/web/http_router.hpp"
#include "bitscrape/web/web_controller.hpp"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace bitscrape::web {

/**
 * @brief HTTP request structure
 */
struct HTTPRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    network::Buffer body;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> path_params;
};

/**
 * @brief HTTP response structure
 */
struct HTTPResponse {
    int status_code = 200;
    std::string status_message = "OK";
    std::map<std::string, std::string> headers;
    network::Buffer body;
};

/**
 * @brief HTTP server class
 *
 * This class provides an HTTP server implementation using the TCP listener.
 * It supports routing requests to different handlers based on the path.
 */
class HTTPServer {
public:
    /**
     * @brief Construct a new HTTPServer object
     *
     * @param port The port to listen on
     * @param web_controller The web controller to use
     */
    HTTPServer(uint16_t port, std::shared_ptr<WebController> web_controller);

    /**
     * @brief Destroy the HTTPServer object
     */
    ~HTTPServer();

    /**
     * @brief Start the server
     *
     * @return true if successful, false otherwise
     */
    bool start();

    /**
     * @brief Stop the server
     *
     * @return true if successful, false otherwise
     */
    bool stop();

    /**
     * @brief Check if the server is running
     *
     * @return true if running, false otherwise
     */
    bool is_running() const;

    /**
     * @brief Get the port the server is listening on
     *
     * @return The port
     */
    uint16_t port() const;

    /**
     * @brief Get the router
     *
     * @return The router
     */
    HTTPRouter& router();

private:
    void accept_connections();
    void handle_connection(std::unique_ptr<network::TCPSocket> socket, const network::Address& address);
    HTTPRequest parse_request(const network::Buffer& buffer);
    network::Buffer generate_response(const HTTPResponse& response);
    std::string url_decode(const std::string& encoded);
    std::map<std::string, std::string> parse_query_params(const std::string& query_string);

    uint16_t port_;
    std::shared_ptr<WebController> web_controller_;
    std::unique_ptr<network::TCPListener> listener_;
    HTTPRouter router_;
    std::atomic<bool> running_;
    std::thread accept_thread_;
    std::vector<std::thread> worker_threads_;
    std::mutex mutex_;
};

} // namespace bitscrape::web
