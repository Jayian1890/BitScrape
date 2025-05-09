#include "bitscrape/network/tcp_socket.hpp"

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

TCPSocket::TCPSocket()
    : socket_(std::make_unique<Socket>(SocketType::TCP)), connected_(false) {
}

TCPSocket::TCPSocket(std::unique_ptr<Socket> socket)
    : socket_(std::move(socket)), connected_(true) {
}

bool TCPSocket::connect(const Address& address) {
    if (!is_valid() || !address.is_valid()) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(address.port());
    addr.sin_addr.s_addr = inet_addr(address.to_string().c_str());

    if (::connect(socket_->descriptor(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        return false;
    }

    connected_ = true;
    return true;
}

std::future<bool> TCPSocket::connect_async(const Address& address) {
    return std::async(std::launch::async, [this, address]() {
        return connect(address);
    });
}

void TCPSocket::close() {
    socket_->close();
    connected_ = false;
}

bool TCPSocket::is_valid() const {
    return socket_->is_valid();
}

bool TCPSocket::is_connected() const {
    return connected_;
}

int TCPSocket::send(const uint8_t* data, size_t size) {
    if (!is_valid() || !is_connected() || data == nullptr || size == 0) {
        return -1;
    }

    return ::send(
        socket_->descriptor(),
        reinterpret_cast<const char*>(data),
        size,
        0
    );
}

std::future<int> TCPSocket::send_async(const uint8_t* data, size_t size) {
    return std::async(std::launch::async, [this, data, size]() {
        return send(data, size);
    });
}

int TCPSocket::send(const Buffer& buffer) {
    return send(buffer.data(), buffer.size());
}

std::future<int> TCPSocket::send_async(const Buffer& buffer) {
    return std::async(std::launch::async, [this, buffer]() {
        return send(buffer);
    });
}

int TCPSocket::receive(uint8_t* data, size_t size) {
    if (!is_valid() || !is_connected() || data == nullptr || size == 0) {
        return -1;
    }

    return recv(
        socket_->descriptor(),
        reinterpret_cast<char*>(data),
        size,
        0
    );
}

std::future<int> TCPSocket::receive_async(uint8_t* data, size_t size) {
    return std::async(std::launch::async, [this, data, size]() {
        return receive(data, size);
    });
}

int TCPSocket::receive(Buffer& buffer) {
    // Resize the buffer to a reasonable size for receiving
    buffer.resize(4096);

    int bytes_received = receive(buffer.data(), buffer.size());
    if (bytes_received > 0) {
        buffer.resize(bytes_received);
    } else {
        buffer.clear();
    }

    return bytes_received;
}

std::future<int> TCPSocket::receive_async(Buffer& buffer) {
    return std::async(std::launch::async, [this, &buffer]() {
        return receive(buffer);
    });
}

bool TCPSocket::set_non_blocking(bool non_blocking) {
    return socket_->set_non_blocking(non_blocking);
}

bool TCPSocket::set_receive_buffer_size(int size) {
    return socket_->set_receive_buffer_size(size);
}

bool TCPSocket::set_send_buffer_size(int size) {
    return socket_->set_send_buffer_size(size);
}

bool TCPSocket::set_receive_timeout(int timeout_ms) {
    return socket_->set_receive_timeout(timeout_ms);
}

bool TCPSocket::set_send_timeout(int timeout_ms) {
    return socket_->set_send_timeout(timeout_ms);
}

Address TCPSocket::get_local_address() const {
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

Address TCPSocket::get_remote_address() const {
    if (!is_valid() || !is_connected()) {
        return Address(0);
    }

    sockaddr_in addr{};
    socklen_t addr_len = sizeof(addr);

    if (getpeername(socket_->descriptor(), reinterpret_cast<sockaddr*>(&addr), &addr_len) == -1) {
        return Address(0);
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    return Address(ip_str, ntohs(addr.sin_port));
}

} // namespace bitscrape::network
