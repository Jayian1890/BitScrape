#pragma once

#include "bitscrape/network/tcp_socket.hpp"
#include "bitscrape/web/web_controller.hpp"
#include "bitscrape/web/http_server.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace bitscrape::web {

/**
 * @brief WebSocket server class
 *
 * This class provides a WebSocket server implementation for real-time updates.
 */
class WebSocketServer {
public:
    /**
     * @brief Construct a new WebSocketServer object
     *
     * @param web_controller The web controller
     */
    explicit WebSocketServer(std::shared_ptr<WebController> web_controller);

    /**
     * @brief Destroy the WebSocketServer object
     */
    ~WebSocketServer();

    /**
     * @brief Handle a WebSocket upgrade request
     *
     * @param request The HTTP request
     * @param socket The TCP socket
     * @return true if successful, false otherwise
     */
    bool handle_upgrade(const HTTPRequest& request, std::unique_ptr<network::TCPSocket> socket);

    /**
     * @brief Send a message to all clients
     *
     * @param message The message
     */
    void broadcast(const std::string& message);

private:
    void handle_client(std::unique_ptr<network::TCPSocket> socket);
    bool perform_handshake(const HTTPRequest& request, network::TCPSocket& socket);
    std::string generate_accept_key(const std::string& key);
    std::vector<uint8_t> encode_frame(const std::string& message);
    std::string decode_frame(const std::vector<uint8_t>& frame);

    std::shared_ptr<WebController> web_controller_;
    std::unordered_map<network::TCPSocket*, std::unique_ptr<network::TCPSocket>> clients_;
    std::mutex clients_mutex_;
    size_t callback_id_;
};

} // namespace bitscrape::web
