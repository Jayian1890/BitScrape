#include "bitscrape/network/http_client.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>

namespace bitscrape::network {

namespace {

// Parse a URL into its components
struct URLComponents {
  std::string protocol;
  std::string host;
  uint16_t port;
  std::string path;
  std::string query;

  static URLComponents parse(const std::string &url) {
    URLComponents components;

    // Default values
    components.port = 80;
    components.path = "/";

    // Find the protocol
    size_t protocol_end = url.find("://");
    if (protocol_end != std::string::npos) {
      components.protocol = url.substr(0, protocol_end);
      protocol_end += 3; // Skip "://"
    } else {
      // Default to HTTP
      components.protocol = "http";
      protocol_end = 0;
    }

    // Convert protocol to lowercase
    std::transform(components.protocol.begin(), components.protocol.end(),
                   components.protocol.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Reject HTTPS - TLS not yet implemented
    if (components.protocol == "https") {
      throw std::runtime_error("HTTPS not supported. TLS implementation "
                               "required. Use HTTP tracker URLs.");
    }

    // Find the host and port
    size_t host_end = url.find("/", protocol_end);
    if (host_end == std::string::npos) {
      host_end = url.length();
    }

    std::string host_port = url.substr(protocol_end, host_end - protocol_end);

    // Check for port
    size_t port_start = host_port.find(":");
    if (port_start != std::string::npos) {
      components.host = host_port.substr(0, port_start);
      components.port = std::stoi(host_port.substr(port_start + 1));
    } else {
      components.host = host_port;
    }

    // Find the path and query
    if (host_end < url.length()) {
      std::string path_query = url.substr(host_end);

      size_t query_start = path_query.find("?");
      if (query_start != std::string::npos) {
        components.path = path_query.substr(0, query_start);
        components.query = path_query.substr(query_start);
      } else {
        components.path = path_query;
      }
    }

    // Ensure path is not empty
    if (components.path.empty()) {
      components.path = "/";
    }

    return components;
  }
};

// Convert HTTP method to string
std::string method_to_string(HTTPMethod method) {
  switch (method) {
  case HTTPMethod::GET:
    return "GET";
  case HTTPMethod::POST:
    return "POST";
  case HTTPMethod::PUT:
    return "PUT";
  case HTTPMethod::DELETE:
    return "DELETE";
  case HTTPMethod::HEAD:
    return "HEAD";
  case HTTPMethod::OPTIONS:
    return "OPTIONS";
  case HTTPMethod::PATCH:
    return "PATCH";
  default:
    return "GET";
  }
}

// Parse HTTP response
HTTPResponse parse_response(const Buffer &buffer) {
  HTTPResponse response;

  // Convert buffer to string for easier parsing
  std::string response_str(reinterpret_cast<const char *>(buffer.data()),
                           buffer.size());

  // Find the end of the headers
  size_t headers_end = response_str.find("\r\n\r\n");
  if (headers_end == std::string::npos) {
    throw std::runtime_error("Invalid HTTP response: no end of headers found");
  }

  // Extract headers
  std::string headers_str = response_str.substr(0, headers_end);

  // Extract body
  size_t body_start = headers_end + 4; // Skip "\r\n\r\n"
  if (body_start < response_str.length()) {
    response.body =
        Buffer(buffer.data() + body_start, buffer.size() - body_start);
  }

  // Parse status line and headers
  std::istringstream headers_stream(headers_str);
  std::string line;

  // Parse status line
  if (std::getline(headers_stream, line, '\r')) {
    // Skip '\n'
    headers_stream.get();

    // Parse status code
    size_t status_start = line.find(" ");
    if (status_start != std::string::npos) {
      size_t status_end = line.find(" ", status_start + 1);
      if (status_end != std::string::npos) {
        std::string status_code_str =
            line.substr(status_start + 1, status_end - status_start - 1);
        response.status_code = std::stoi(status_code_str);
      }
    }
  }

  // Parse headers
  while (std::getline(headers_stream, line, '\r')) {
    // Skip '\n'
    headers_stream.get();

    size_t colon_pos = line.find(":");
    if (colon_pos != std::string::npos) {
      std::string name = line.substr(0, colon_pos);
      std::string value = line.substr(colon_pos + 1);

      // Trim leading and trailing whitespace from value
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t") + 1);

      response.headers[name] = value;
    }
  }

  return response;
}

} // namespace

HTTPClient::HTTPClient()
    : connection_timeout_ms_(30000), request_timeout_ms_(30000),
      follow_redirects_(true), max_redirects_(5) {}

HTTPResponse
HTTPClient::request(HTTPMethod method, const std::string &url,
                    const std::map<std::string, std::string> &headers,
                    const Buffer &body) {
  // Parse URL
  URLComponents components = URLComponents::parse(url);

  // Create TCP socket
  TCPSocket socket;

  // Apply configured timeouts
  socket.set_send_timeout(connection_timeout_ms_);
  socket.set_receive_timeout(request_timeout_ms_);

  // Connect to server
  if (!socket.connect(Address(components.host, components.port))) {
    throw std::runtime_error("Failed to connect to server: " + components.host +
                             ":" + std::to_string(components.port));
  }

  // Build request
  std::ostringstream request_stream;
  request_stream << method_to_string(method) << " " << components.path;
  if (!components.query.empty()) {
    request_stream << components.query;
  }
  request_stream << " HTTP/1.1\r\n";

  // Add headers
  request_stream << "Host: " << components.host;
  if ((components.protocol == "http" && components.port != 80) ||
      (components.protocol == "https" && components.port != 443)) {
    request_stream << ":" << components.port;
  }
  request_stream << "\r\n";

  // Add user-provided headers
  for (const auto &header : headers) {
    request_stream << header.first << ": " << header.second << "\r\n";
  }

  // Add content length if body is not empty
  if (body.size() > 0) {
    request_stream << "Content-Length: " << body.size() << "\r\n";
  }

  // Add default headers if not provided by user
  if (headers.find("Connection") == headers.end()) {
    request_stream << "Connection: close\r\n";
  }

  // End of headers
  request_stream << "\r\n";

  // Convert request to buffer
  std::string request_str = request_stream.str();
  Buffer request_buffer(reinterpret_cast<const uint8_t *>(request_str.c_str()),
                        request_str.length());

  // Add body if present
  if (body.size() > 0) {
    request_buffer.append(body);
  }

  // Send request
  if (socket.send(request_buffer) == -1) {
    throw std::runtime_error("Failed to send HTTP request");
  }

  // Receive response
  Buffer response_buffer;
  int bytes_received;

  do {
    Buffer chunk(4096);
    bytes_received = socket.receive(chunk);

    if (bytes_received > 0) {
      response_buffer.append(chunk.data(), bytes_received);
    }
  } while (bytes_received > 0);

  // Parse response
  HTTPResponse response = parse_response(response_buffer);

  // Handle redirects
  if (follow_redirects_ &&
      (response.status_code == 301 || response.status_code == 302 ||
       response.status_code == 303 || response.status_code == 307 ||
       response.status_code == 308)) {
    auto it = response.headers.find("Location");
    if (it != response.headers.end() && max_redirects_ > 0) {
      // Decrement max redirects
      max_redirects_--;

      // Follow redirect
      return request(method, it->second, headers, body);
    }
  }

  return response;
}

std::future<HTTPResponse>
HTTPClient::request_async(HTTPMethod method, const std::string &url,
                          const std::map<std::string, std::string> &headers,
                          const Buffer &body) {
  return std::async(std::launch::async, [this, method, url, headers, body]() {
    return request(method, url, headers, body);
  });
}

HTTPResponse
HTTPClient::get(const std::string &url,
                const std::map<std::string, std::string> &headers) {
  return request(HTTPMethod::GET, url, headers);
}

std::future<HTTPResponse>
HTTPClient::get_async(const std::string &url,
                      const std::map<std::string, std::string> &headers) {
  return request_async(HTTPMethod::GET, url, headers);
}

HTTPResponse HTTPClient::post(const std::string &url,
                              const std::map<std::string, std::string> &headers,
                              const Buffer &body) {
  return request(HTTPMethod::POST, url, headers, body);
}

std::future<HTTPResponse>
HTTPClient::post_async(const std::string &url,
                       const std::map<std::string, std::string> &headers,
                       const Buffer &body) {
  return request_async(HTTPMethod::POST, url, headers, body);
}

void HTTPClient::set_connection_timeout(int timeout_ms) {
  connection_timeout_ms_ = timeout_ms;
}

void HTTPClient::set_request_timeout(int timeout_ms) {
  request_timeout_ms_ = timeout_ms;
}

void HTTPClient::set_follow_redirects(bool follow_redirects) {
  follow_redirects_ = follow_redirects;
}

void HTTPClient::set_max_redirects(int max_redirects) {
  max_redirects_ = max_redirects;
}

} // namespace bitscrape::network
