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
 * @brief UDPSocket class for UDP communication
 * 
 * This class provides a cross-platform abstraction for UDP socket operations.
 * It supports both synchronous and asynchronous APIs for sending and receiving data.
 */
class UDPSocket {
public:
    /**
     * @brief Construct a new UDPSocket object
     */
    UDPSocket();

    /**
     * @brief Destroy the UDPSocket object
     */
    ~UDPSocket() = default;

    /**
     * @brief UDPSocket is non-copyable
     */
    UDPSocket(const UDPSocket&) = delete;
    UDPSocket& operator=(const UDPSocket&) = delete;

    /**
     * @brief UDPSocket is movable
     */
    UDPSocket(UDPSocket&&) noexcept = default;
    UDPSocket& operator=(UDPSocket&&) noexcept = default;

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
     * @brief Send data to a specific address
     * 
     * @param data The data to send
     * @param size The size of the data
     * @param address The destination address
     * @return The number of bytes sent, or -1 on error
     */
    int send_to(const uint8_t* data, size_t size, const Address& address);

    /**
     * @brief Send data to a specific address asynchronously
     * 
     * @param data The data to send
     * @param size The size of the data
     * @param address The destination address
     * @return A future that will contain the number of bytes sent, or -1 on error
     */
    std::future<int> send_to_async(const uint8_t* data, size_t size, const Address& address);

    /**
     * @brief Send a buffer to a specific address
     * 
     * @param buffer The buffer to send
     * @param address The destination address
     * @return The number of bytes sent, or -1 on error
     */
    int send_to(const Buffer& buffer, const Address& address);

    /**
     * @brief Send a buffer to a specific address asynchronously
     * 
     * @param buffer The buffer to send
     * @param address The destination address
     * @return A future that will contain the number of bytes sent, or -1 on error
     */
    std::future<int> send_to_async(const Buffer& buffer, const Address& address);

    /**
     * @brief Receive data from any address
     * 
     * @param data The buffer to receive into
     * @param size The size of the buffer
     * @param address The source address (output)
     * @return The number of bytes received, or -1 on error
     */
    int receive_from(uint8_t* data, size_t size, Address& address);

    /**
     * @brief Receive data from any address asynchronously
     * 
     * @param data The buffer to receive into
     * @param size The size of the buffer
     * @return A future that will contain a pair of the number of bytes received and the source address
     */
    std::future<std::pair<int, Address>> receive_from_async(uint8_t* data, size_t size);

    /**
     * @brief Receive data into a buffer from any address
     * 
     * @param buffer The buffer to receive into
     * @param address The source address (output)
     * @return The number of bytes received, or -1 on error
     */
    int receive_from(Buffer& buffer, Address& address);

    /**
     * @brief Receive data into a buffer from any address asynchronously
     * 
     * @param buffer The buffer to receive into
     * @return A future that will contain a pair of the number of bytes received and the source address
     */
    std::future<std::pair<int, Address>> receive_from_async(Buffer& buffer);

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
    std::unique_ptr<Socket> socket_;
};

} // namespace bitscrape::network
