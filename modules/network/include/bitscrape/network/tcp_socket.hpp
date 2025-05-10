#pragma once

#include "bitscrape/network/socket.hpp"
#include "bitscrape/network/address.hpp"
#include "bitscrape/network/buffer.hpp"

#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace bitscrape::network {

/**
 * @brief TCPSocket class for TCP communication
 * 
 * This class provides a cross-platform abstraction for TCP socket operations.
 * It supports both synchronous and asynchronous APIs for connecting, sending, and receiving data.
 */
class TCPSocket {
public:
    /**
     * @brief Construct a new TCPSocket object
     */
    TCPSocket();

    /**
     * @brief Construct a new TCPSocket object from an existing socket
     * 
     * @param socket The socket to take ownership of
     */
    explicit TCPSocket(std::unique_ptr<Socket> socket);

    /**
     * @brief Destroy the TCPSocket object
     */
    ~TCPSocket() = default;

    /**
     * @brief TCPSocket is non-copyable
     */
    TCPSocket(const TCPSocket&) = delete;
    TCPSocket& operator=(const TCPSocket&) = delete;

    /**
     * @brief TCPSocket is movable
     */
    TCPSocket(TCPSocket&&) noexcept = default;
    TCPSocket& operator=(TCPSocket&&) noexcept = default;

    /**
     * @brief Connect to a specific address
     * 
     * @param address The address to connect to
     * @return true if successful, false otherwise
     */
    bool connect(const Address& address);

    /**
     * @brief Connect to a specific address asynchronously
     * 
     * @param address The address to connect to
     * @return A future that will contain true if successful, false otherwise
     */
    std::future<bool> connect_async(const Address& address);

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
     * @brief Check if the socket is connected
     * 
     * @return true if the socket is connected, false otherwise
     */
    bool is_connected() const;

    /**
     * @brief Send data
     * 
     * @param data The data to send
     * @param size The size of the data
     * @return The number of bytes sent, or -1 on error
     */
    int send(const uint8_t* data, size_t size);

    /**
     * @brief Send data asynchronously
     * 
     * @param data The data to send
     * @param size The size of the data
     * @return A future that will contain the number of bytes sent, or -1 on error
     */
    std::future<int> send_async(const uint8_t* data, size_t size);

    /**
     * @brief Send a buffer
     * 
     * @param buffer The buffer to send
     * @return The number of bytes sent, or -1 on error
     */
    int send(const Buffer& buffer);

    /**
     * @brief Send a buffer asynchronously
     * 
     * @param buffer The buffer to send
     * @return A future that will contain the number of bytes sent, or -1 on error
     */
    std::future<int> send_async(const Buffer& buffer);

    /**
     * @brief Receive data
     * 
     * @param data The buffer to receive into
     * @param size The size of the buffer
     * @return The number of bytes received, or -1 on error
     */
    int receive(uint8_t* data, size_t size);

    /**
     * @brief Receive data asynchronously
     * 
     * @param data The buffer to receive into
     * @param size The size of the buffer
     * @return A future that will contain the number of bytes received, or -1 on error
     */
    std::future<int> receive_async(uint8_t* data, size_t size);

    /**
     * @brief Receive data into a buffer
     * 
     * @param buffer The buffer to receive into
     * @return The number of bytes received, or -1 on error
     */
    int receive(Buffer& buffer);

    /**
     * @brief Receive data into a buffer asynchronously
     * 
     * @param buffer The buffer to receive into
     * @return A future that will contain the number of bytes received, or -1 on error
     */
    std::future<int> receive_async(Buffer& buffer);

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

    /**
     * @brief Get the local address
     * 
     * @return The local address
     */
    Address get_local_address() const;

    /**
     * @brief Get the remote address
     * 
     * @return The remote address
     */
    Address get_remote_address() const;

private:
    std::unique_ptr<Socket> socket_;
    bool connected_;
};

} // namespace bitscrape::network
