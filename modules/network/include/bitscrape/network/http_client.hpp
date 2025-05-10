#pragma once

#include "bitscrape/network/buffer.hpp"
#include "bitscrape/network/tcp_socket.hpp"

#include <cstdint>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace bitscrape::network {

/**
 * @brief HTTP method enumeration
 */
enum class HTTPMethod {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH
};

/**
 * @brief HTTP response structure
 */
struct HTTPResponse {
    int status_code;
    std::map<std::string, std::string> headers;
    Buffer body;
};

/**
 * @brief HTTPClient class for making HTTP requests
 * 
 * This class provides a simple HTTP client for making HTTP requests.
 * It supports both synchronous and asynchronous APIs for making requests.
 */
class HTTPClient {
public:
    /**
     * @brief Construct a new HTTPClient object
     */
    HTTPClient();

    /**
     * @brief Destroy the HTTPClient object
     */
    ~HTTPClient() = default;

    /**
     * @brief HTTPClient is non-copyable
     */
    HTTPClient(const HTTPClient&) = delete;
    HTTPClient& operator=(const HTTPClient&) = delete;

    /**
     * @brief HTTPClient is movable
     */
    HTTPClient(HTTPClient&&) noexcept = default;
    HTTPClient& operator=(HTTPClient&&) noexcept = default;

    /**
     * @brief Make an HTTP request
     * 
     * @param method The HTTP method
     * @param url The URL to request
     * @param headers The request headers
     * @param body The request body
     * @return The HTTP response
     */
    HTTPResponse request(
        HTTPMethod method,
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const Buffer& body = Buffer()
    );

    /**
     * @brief Make an HTTP request asynchronously
     * 
     * @param method The HTTP method
     * @param url The URL to request
     * @param headers The request headers
     * @param body The request body
     * @return A future that will contain the HTTP response
     */
    std::future<HTTPResponse> request_async(
        HTTPMethod method,
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const Buffer& body = Buffer()
    );

    /**
     * @brief Make an HTTP GET request
     * 
     * @param url The URL to request
     * @param headers The request headers
     * @return The HTTP response
     */
    HTTPResponse get(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {}
    );

    /**
     * @brief Make an HTTP GET request asynchronously
     * 
     * @param url The URL to request
     * @param headers The request headers
     * @return A future that will contain the HTTP response
     */
    std::future<HTTPResponse> get_async(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {}
    );

    /**
     * @brief Make an HTTP POST request
     * 
     * @param url The URL to request
     * @param headers The request headers
     * @param body The request body
     * @return The HTTP response
     */
    HTTPResponse post(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const Buffer& body = Buffer()
    );

    /**
     * @brief Make an HTTP POST request asynchronously
     * 
     * @param url The URL to request
     * @param headers The request headers
     * @param body The request body
     * @return A future that will contain the HTTP response
     */
    std::future<HTTPResponse> post_async(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const Buffer& body = Buffer()
    );

    /**
     * @brief Set the connection timeout
     * 
     * @param timeout_ms The timeout in milliseconds
     */
    void set_connection_timeout(int timeout_ms);

    /**
     * @brief Set the request timeout
     * 
     * @param timeout_ms The timeout in milliseconds
     */
    void set_request_timeout(int timeout_ms);

    /**
     * @brief Set whether to follow redirects
     * 
     * @param follow_redirects true to follow redirects, false otherwise
     */
    void set_follow_redirects(bool follow_redirects);

    /**
     * @brief Set the maximum number of redirects to follow
     * 
     * @param max_redirects The maximum number of redirects
     */
    void set_max_redirects(int max_redirects);

private:
    int connection_timeout_ms_;
    int request_timeout_ms_;
    bool follow_redirects_;
    int max_redirects_;
};

} // namespace bitscrape::network
