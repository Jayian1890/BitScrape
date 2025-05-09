#pragma once

#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace bitscrape::network {

/**
 * @brief Socket types enumeration
 */
enum class SocketType {
    UDP,
    TCP
};

/**
 * @brief Socket class for low-level socket operations
 * 
 * This class provides a cross-platform abstraction for socket operations.
 * It supports both UDP and TCP sockets with synchronous and asynchronous APIs.
 */
class Socket {
public:
    /**
     * @brief Construct a new Socket object
     * 
     * @param type The socket type (UDP or TCP)
     */
    explicit Socket(SocketType type);

    /**
     * @brief Destroy the Socket object
     */
    virtual ~Socket();

    /**
     * @brief Socket is non-copyable
     */
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    /**
     * @brief Socket is movable
     */
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    /**
     * @brief Bind the socket to a specific port
     * 
     * @param port The port to bind to
     * @return true if successful, false otherwise
     */
    bool bind(uint16_t port);

    /**
     * @brief Bind the socket to a specific address and port
     * 
     * @param address The address to bind to
     * @param port The port to bind to
     * @return true if successful, false otherwise
     */
    bool bind(const std::string& address, uint16_t port);

    /**
     * @brief Close the socket
     */
    void close();

    /**
     * @brief Check if the socket is valid
     * 
     * @return true if the socket is valid, false otherwise
     */
    bool is_valid() const;

    /**
     * @brief Get the socket type
     * 
     * @return The socket type
     */
    SocketType type() const;

    /**
     * @brief Get the socket descriptor
     * 
     * @return The socket descriptor
     */
    int descriptor() const;

    /**
     * @brief Set the socket to non-blocking mode
     * 
     * @param non_blocking true to set non-blocking, false for blocking
     * @return true if successful, false otherwise
     */
    bool set_non_blocking(bool non_blocking);

    /**
     * @brief Set the socket receive buffer size
     * 
     * @param size The buffer size in bytes
     * @return true if successful, false otherwise
     */
    bool set_receive_buffer_size(int size);

    /**
     * @brief Set the socket send buffer size
     * 
     * @param size The buffer size in bytes
     * @return true if successful, false otherwise
     */
    bool set_send_buffer_size(int size);

    /**
     * @brief Set the socket receive timeout
     * 
     * @param timeout_ms The timeout in milliseconds
     * @return true if successful, false otherwise
     */
    bool set_receive_timeout(int timeout_ms);

    /**
     * @brief Set the socket send timeout
     * 
     * @param timeout_ms The timeout in milliseconds
     * @return true if successful, false otherwise
     */
    bool set_send_timeout(int timeout_ms);

private:
    int socket_fd_;
    SocketType type_;
};

} // namespace bitscrape::network
