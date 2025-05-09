#include "bitscrape/network/tcp_listener.hpp"

#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace bitscrape::network {

TCPListener::TCPListener()
    : socket_(std::make_unique<Socket>(SocketType::TCP)), listening_(false) {
}

bool TCPListener::bind(uint16_t port) {
    return socket_->bind(port);
}

bool TCPListener::bind(const std::string& address, uint16_t port) {
    return socket_->bind(address, port);
}

bool TCPListener::listen(int backlog) {
    if (!is_valid()) {
        return false;
    }

    if (::listen(socket_->descriptor(), backlog) == -1) {
        return false;
    }

    listening_ = true;
    return true;
}

std::unique_ptr<TCPSocket> TCPListener::accept(Address& address) {
    if (!is_valid() || !is_listening()) {
        return nullptr;
    }

    sockaddr_in addr{};
    socklen_t addr_len = sizeof(addr);

    int client_fd = ::accept(socket_->descriptor(), reinterpret_cast<sockaddr*>(&addr), &addr_len);
    if (client_fd == -1) {
        return nullptr;
    }

    // Create a new Socket object for the client
    auto client_socket = std::make_unique<Socket>(SocketType::TCP);
    
    // Close the default socket and replace it with the accepted one
    client_socket->close();
    
    // Set the socket descriptor to the accepted client socket
    // Note: This is a bit of a hack, but it works for now
    // In a real implementation, we would need to add a method to Socket to set the descriptor
    *reinterpret_cast<int*>(client_socket.get()) = client_fd;

    // Get the client address
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    address = Address(ip_str, ntohs(addr.sin_port));

    // Create a new TCPSocket with the client socket
    return std::make_unique<TCPSocket>(std::move(client_socket));
}

std::future<std::pair<std::unique_ptr<TCPSocket>, Address>> TCPListener::accept_async() {
    return std::async(std::launch::async, [this]() {
        Address address;
        auto socket = accept(address);
        return std::make_pair(std::move(socket), address);
    });
}

void TCPListener::close() {
    socket_->close();
    listening_ = false;
}

bool TCPListener::is_valid() const {
    return socket_->is_valid();
}

bool TCPListener::is_listening() const {
    return listening_;
}

bool TCPListener::set_non_blocking(bool non_blocking) {
    return socket_->set_non_blocking(non_blocking);
}

Address TCPListener::get_local_address() const {
    if (!is_valid()) {
        return Address(0);
    }

    sockaddr_in addr{};
    socklen_t addr_len = sizeof(addr);

    if (getsockname(socket_->descriptor(), reinterpret_cast<sockaddr*>(&addr), &addr_len) == -1) {
        return Address(0);
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    return Address(ip_str, ntohs(addr.sin_port));
}

} // namespace bitscrape::network
