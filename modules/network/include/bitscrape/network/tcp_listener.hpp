#pragma once

#include "bitscrape/network/socket.hpp"
#include "bitscrape/network/address.hpp"
#include "bitscrape/network/tcp_socket.hpp"

#include <cstdint>
#include <future>
#include <memory>
#include <string>

namespace bitscrape::network {

/**
 * @brief TCPListener class for accepting TCP connections
 * 
 * This class provides a cross-platform abstraction for TCP listening socket operations.
 * It supports both synchronous and asynchronous APIs for accepting connections.
 */
class TCPListener {
public:
    /**
     * @brief Construct a new TCPListener object
     */
    TCPListener();

    /**
     * @brief Destroy the TCPListener object
     */
    ~TCPListener() = default;

    /**
     * @brief TCPListener is non-copyable
     */
    TCPListener(const TCPListener&) = delete;
    TCPListener& operator=(const TCPListener&) = delete;

    /**
     * @brief TCPListener is movable
     */
    TCPListener(TCPListener&&) noexcept = default;
    TCPListener& operator=(TCPListener&&) noexcept = default;

    /**
     * @brief Bind the listener to a specific port
     * 
     * @param port The port to bind to
     * @return true if successful, false otherwise
     */
    bool bind(uint16_t port);

    /**
     * @brief Bind the listener to a specific address and port
     * 
     * @param address The address to bind to
     * @param port The port to bind to
     * @return true if successful, false otherwise
     */
    bool bind(const std::string& address, uint16_t port);

    /**
     * @brief Start listening for connections
     * 
     * @param backlog The maximum number of pending connections
     * @return true if successful, false otherwise
     */
    bool listen(int backlog = 10);

    /**
     * @brief Accept a new connection
     * 
     * @param address The address of the client (output)
     * @return A new TCPSocket for the accepted connection, or nullptr on error
     */
    std::unique_ptr<TCPSocket> accept(Address& address);

    /**
     * @brief Accept a new connection asynchronously
     * 
     * @return A future that will contain a pair of a new TCPSocket for the accepted connection and the client address
     */
    std::future<std::pair<std::unique_ptr<TCPSocket>, Address>> accept_async();

    /**
     * @brief Close the listener
     */
    void close();

    /**
     * @brief Check if the listener is valid
     * 
     * @return true if the listener is valid, false otherwise
     */
    bool is_valid() const;

    /**
     * @brief Check if the listener is listening
     * 
     * @return true if the listener is listening, false otherwise
     */
    bool is_listening() const;

    /**
     * @brief Set the listener to non-blocking mode
     * 
     * @param non_blocking true to set non-blocking, false for blocking
     * @return true if successful, false otherwise
     */
    bool set_non_blocking(bool non_blocking);

    /**
     * @brief Get the local address
     * 
     * @return The local address
     */
    Address get_local_address() const;

private:
    std::unique_ptr<Socket> socket_;
    bool listening_;
};

} // namespace bitscrape::network
