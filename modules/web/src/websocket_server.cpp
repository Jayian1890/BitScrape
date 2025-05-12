#include "bitscrape/web/websocket_server.hpp"
#include "bitscrape/web/http_server.hpp"
#include "bitscrape/web/sha1.hpp"
#include "bitscrape/web/base64.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

namespace bitscrape::web {

WebSocketServer::WebSocketServer(std::shared_ptr<WebController> web_controller)
    : web_controller_(web_controller) {
    // Register callback for events
    callback_id_ = web_controller_->register_websocket_callback(
        [this](const std::string& message) {
            broadcast(message);
        }
    );
}

WebSocketServer::~WebSocketServer() {
    // Unregister callback
    web_controller_->unregister_websocket_callback(callback_id_);

    // Close all clients
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.clear();
}

bool WebSocketServer::handle_upgrade(const HTTPRequest& request, std::unique_ptr<network::TCPSocket> socket) {
    // Perform WebSocket handshake
    if (!perform_handshake(request, *socket)) {
        return false;
    }

    // Start client handler thread
    std::thread([this, socket = std::move(socket)]() mutable {
        handle_client(std::move(socket));
    }).detach();

    return true;
}

void WebSocketServer::broadcast(const std::string& message) {
    // Encode message
    auto frame = encode_frame(message);

    // Send to all clients
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto it = clients_.begin(); it != clients_.end();) {
        try {
            it->second->send(frame.data(), frame.size());
            ++it;
        } catch (const std::exception& e) {
            // Remove client on error
            std::cerr << "Error sending to client: " << e.what() << std::endl;
            it = clients_.erase(it);
        }
    }
}

void WebSocketServer::handle_client(std::unique_ptr<network::TCPSocket> socket) {
    // Add client to list
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_[socket.get()] = std::move(socket);
    }

    // Handle client messages
    try {
        while (true) {
            // Receive message
            network::Buffer buffer(1024);
            int bytes_received = clients_[socket.get()]->receive(buffer);

            if (bytes_received <= 0) {
                // Connection closed
                break;
            }

            // Decode frame
            std::string message = decode_frame(std::vector<uint8_t>(buffer.data(), buffer.data() + bytes_received));

            // TODO: Handle message
            std::cout << "Received message: " << message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }

    // Remove client from list
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(socket.get());
}

bool WebSocketServer::perform_handshake(const HTTPRequest& request, network::TCPSocket& socket) {
    // Check for WebSocket upgrade request
    auto upgrade_it = request.headers.find("Upgrade");
    if (upgrade_it == request.headers.end() || upgrade_it->second != "websocket") {
        return false;
    }

    // Check for WebSocket key
    auto key_it = request.headers.find("Sec-WebSocket-Key");
    if (key_it == request.headers.end()) {
        return false;
    }

    // Generate accept key
    std::string accept_key = generate_accept_key(key_it->second);

    // Create response
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << accept_key << "\r\n";
    response << "\r\n";

    // Send response
    std::string response_str = response.str();
    socket.send(reinterpret_cast<const uint8_t*>(response_str.c_str()), response_str.length());

    return true;
}

std::string WebSocketServer::generate_accept_key(const std::string& key) {
    // Concatenate key with WebSocket GUID
    std::string combined = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    // Compute SHA-1 hash
    auto hash = SHA1::compute(combined);

    // Base64 encode
    return Base64::encode(hash.data(), hash.size());
}

std::vector<uint8_t> WebSocketServer::encode_frame(const std::string& message) {
    std::vector<uint8_t> frame;

    // Add FIN bit and opcode (0x81 = FIN + text)
    frame.push_back(0x81);

    // Add payload length
    if (message.length() < 126) {
        frame.push_back(static_cast<uint8_t>(message.length()));
    } else if (message.length() < 65536) {
        frame.push_back(126);
        frame.push_back(static_cast<uint8_t>((message.length() >> 8) & 0xff));
        frame.push_back(static_cast<uint8_t>(message.length() & 0xff));
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((message.length() >> (i * 8)) & 0xff));
        }
    }

    // Add payload
    frame.insert(frame.end(), message.begin(), message.end());

    return frame;
}

std::string WebSocketServer::decode_frame(const std::vector<uint8_t>& frame) {
    // Check if we have enough data
    if (frame.size() < 2) {
        return "";
    }

    // Check FIN bit and opcode
    bool fin = (frame[0] & 0x80) != 0;
    uint8_t opcode = frame[0] & 0x0f;

    // Check if masked
    bool masked = (frame[1] & 0x80) != 0;

    // Get payload length
    uint64_t payload_length = frame[1] & 0x7f;
    size_t payload_offset = 2;

    if (payload_length == 126) {
        if (frame.size() < 4) {
            return "";
        }
        payload_length = (frame[2] << 8) | frame[3];
        payload_offset = 4;
    } else if (payload_length == 127) {
        if (frame.size() < 10) {
            return "";
        }
        payload_length = 0;
        for (int i = 0; i < 8; ++i) {
            payload_length = (payload_length << 8) | frame[2 + i];
        }
        payload_offset = 10;
    }

    // Get masking key
    uint8_t masking_key[4] = {0};
    if (masked) {
        if (frame.size() < payload_offset + 4) {
            return "";
        }
        for (int i = 0; i < 4; ++i) {
            masking_key[i] = frame[payload_offset + i];
        }
        payload_offset += 4;
    }

    // Check if we have enough data
    if (frame.size() < payload_offset + payload_length) {
        return "";
    }

    // Decode payload
    std::string payload;
    for (size_t i = 0; i < payload_length; ++i) {
        uint8_t byte = frame[payload_offset + i];
        if (masked) {
            byte ^= masking_key[i % 4];
        }
        payload.push_back(static_cast<char>(byte));
    }

    return payload;
}

} // namespace bitscrape::web
