#include "bitscrape/web/http_server.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

namespace bitscrape::web {

HTTPServer::HTTPServer(uint16_t port, std::shared_ptr<WebController> web_controller)
    : port_(port), web_controller_(web_controller), running_(false) {
    listener_ = std::make_unique<network::TCPListener>();
}

HTTPServer::~HTTPServer() {
    if (running_) {
        stop();
    }
}

bool HTTPServer::start() {
    if (running_) {
        return true;
    }

    // Bind to port
    if (!listener_->bind(port_)) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        return false;
    }

    // Start listening
    if (!listener_->listen()) {
        std::cerr << "Failed to start listening" << std::endl;
        return false;
    }

    // Set running flag
    running_ = true;

    // Start accept thread
    accept_thread_ = std::thread(&HTTPServer::accept_connections, this);

    return true;
}

bool HTTPServer::stop() {
    if (!running_) {
        return true;
    }

    // Set running flag
    running_ = false;

    // Close listener
    listener_->close();

    // Wait for accept thread to finish
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    // Wait for worker threads to finish
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Clear worker threads
    worker_threads_.clear();

    return true;
}

bool HTTPServer::is_running() const {
    return running_;
}

uint16_t HTTPServer::port() const {
    return port_;
}

HTTPRouter& HTTPServer::router() {
    return router_;
}

void HTTPServer::accept_connections() {
    while (running_) {
        try {
            // Accept a new connection
            network::Address client_address;
            auto client_socket = listener_->accept(client_address);

            if (!client_socket) {
                // Accept failed, check if we're still running
                if (!running_) {
                    break;
                }
                
                // Sleep for a bit and try again
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // Handle the connection in a new thread
            std::thread worker(&HTTPServer::handle_connection, this, std::move(client_socket), client_address);
            
            // Store the worker thread
            std::lock_guard<std::mutex> lock(mutex_);
            worker_threads_.push_back(std::move(worker));
            
            // Clean up finished worker threads
            worker_threads_.erase(
                std::remove_if(worker_threads_.begin(), worker_threads_.end(),
                    [](std::thread& t) { return !t.joinable(); }),
                worker_threads_.end()
            );
        } catch (const std::exception& e) {
            std::cerr << "Exception in accept_connections: " << e.what() << std::endl;
            
            // Sleep for a bit and try again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void HTTPServer::handle_connection(std::unique_ptr<network::TCPSocket> socket, const network::Address& address) {
    try {
        // Set a timeout for the socket
        // TODO: Implement socket timeout

        // Receive request
        network::Buffer request_buffer(4096);
        int bytes_received = socket->receive(request_buffer);
        
        if (bytes_received <= 0) {
            // Connection closed or error
            return;
        }
        
        // Parse request
        HTTPRequest request = parse_request(request_buffer);
        
        // Find handler
        auto handler = router_.find_handler(request);
        
        // Handle request
        HTTPResponse response;
        if (handler) {
            try {
                response = handler(request, web_controller_);
            } catch (const std::exception& e) {
                // Handler threw an exception
                response.status_code = 500;
                response.status_message = "Internal Server Error";
                response.body = network::Buffer(
                    reinterpret_cast<const uint8_t*>(e.what()),
                    strlen(e.what())
                );
            }
        } else {
            // No handler found
            response.status_code = 404;
            response.status_message = "Not Found";
            response.body = network::Buffer(
                reinterpret_cast<const uint8_t*>("Not Found"),
                9
            );
        }
        
        // Generate response
        network::Buffer response_buffer = generate_response(response);
        
        // Send response
        socket->send(response_buffer);
    } catch (const std::exception& e) {
        std::cerr << "Exception in handle_connection: " << e.what() << std::endl;
    }
}

HTTPRequest HTTPServer::parse_request(const network::Buffer& buffer) {
    HTTPRequest request;
    
    // Convert buffer to string
    std::string request_str(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    
    // Parse request line
    std::istringstream stream(request_str);
    std::string line;
    std::getline(stream, line);
    
    // Remove carriage return
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    
    // Parse method, path, and version
    std::istringstream line_stream(line);
    line_stream >> request.method >> request.path >> request.version;
    
    // Parse query parameters
    size_t query_pos = request.path.find('?');
    if (query_pos != std::string::npos) {
        std::string query_string = request.path.substr(query_pos + 1);
        request.path = request.path.substr(0, query_pos);
        request.query_params = parse_query_params(query_string);
    }
    
    // Parse headers
    while (std::getline(stream, line) && !line.empty()) {
        // Remove carriage return
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Empty line indicates end of headers
        if (line.empty()) {
            break;
        }
        
        // Parse header
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // Trim leading and trailing whitespace from value
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            request.headers[name] = value;
        }
    }
    
    // Parse body
    std::string body_str;
    std::getline(stream, body_str, '\0');
    
    // Remove leading newline
    if (!body_str.empty() && body_str.front() == '\n') {
        body_str.erase(0, 1);
    }
    
    // Set body
    request.body = network::Buffer(
        reinterpret_cast<const uint8_t*>(body_str.c_str()),
        body_str.length()
    );
    
    return request;
}

network::Buffer HTTPServer::generate_response(const HTTPResponse& response) {
    std::ostringstream stream;
    
    // Write status line
    stream << "HTTP/1.1 " << response.status_code << " " << response.status_message << "\r\n";
    
    // Write headers
    for (const auto& header : response.headers) {
        stream << header.first << ": " << header.second << "\r\n";
    }
    
    // Write content length if not already set
    if (response.headers.find("Content-Length") == response.headers.end()) {
        stream << "Content-Length: " << response.body.size() << "\r\n";
    }
    
    // Write content type if not already set
    if (response.headers.find("Content-Type") == response.headers.end()) {
        stream << "Content-Type: text/plain\r\n";
    }
    
    // Write server header
    stream << "Server: BitScrape Web Server\r\n";
    
    // End headers
    stream << "\r\n";
    
    // Convert to buffer
    std::string headers_str = stream.str();
    network::Buffer buffer(
        reinterpret_cast<const uint8_t*>(headers_str.c_str()),
        headers_str.length()
    );
    
    // Append body
    buffer.append(response.body);
    
    return buffer;
}

std::map<std::string, std::string> HTTPServer::parse_query_params(const std::string& query_string) {
    std::map<std::string, std::string> params;
    
    // Split by &
    std::istringstream stream(query_string);
    std::string pair;
    while (std::getline(stream, pair, '&')) {
        // Split by =
        size_t equals_pos = pair.find('=');
        if (equals_pos != std::string::npos) {
            std::string name = pair.substr(0, equals_pos);
            std::string value = pair.substr(equals_pos + 1);
            
            // URL decode
            // TODO: Implement URL decoding
            
            params[name] = value;
        } else {
            // No value, just a flag
            params[pair] = "";
        }
    }
    
    return params;
}

} // namespace bitscrape::web
