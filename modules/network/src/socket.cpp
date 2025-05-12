#include "bitscrape/network/socket.hpp"

#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace bitscrape::network {

namespace {

#ifdef _WIN32
class WSAInitializer {
public:
    WSAInitializer() {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            throw std::runtime_error("Failed to initialize Winsock");
        }
    }

    ~WSAInitializer() {
        WSACleanup();
    }
};

static WSAInitializer wsa_initializer;
#endif

} // namespace

Socket::Socket(SocketType type)
    : socket_fd_(-1), type_(type) {
    int socket_type = (type == SocketType::UDP) ? SOCK_DGRAM : SOCK_STREAM;
    int protocol = (type == SocketType::UDP) ? IPPROTO_UDP : IPPROTO_TCP;

    socket_fd_ = socket(AF_INET, socket_type, protocol);
    if (socket_fd_ == -1) {
        throw std::system_error(errno, std::generic_category(), "Failed to create socket");
    }
}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept
    : socket_fd_(other.socket_fd_), type_(other.type_) {
    other.socket_fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        socket_fd_ = other.socket_fd_;
        type_ = other.type_;
        other.socket_fd_ = -1;
    }
    return *this;
}

bool Socket::bind(uint16_t port) {
    return bind("0.0.0.0", port);
}

bool Socket::bind(const std::string& address, uint16_t port) {
    if (!is_valid()) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(address.c_str());

    if (::bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        return false;
    }

    return true;
}

void Socket::close() {
    if (is_valid()) {
#ifdef _WIN32
        closesocket(socket_fd_);
#else
        ::close(socket_fd_);
#endif
        socket_fd_ = -1;
    }
}

bool Socket::is_valid() const {
    return socket_fd_ != -1;
}

SocketType Socket::type() const {
    return type_;
}

int Socket::descriptor() const {
    return socket_fd_;
}

void Socket::set_descriptor(int fd) {
    // Close the current socket if it's valid
    close();

    // Set the new descriptor
    socket_fd_ = fd;
}

bool Socket::set_non_blocking(bool non_blocking) {
    if (!is_valid()) {
        return false;
    }

#ifdef _WIN32
    u_long mode = non_blocking ? 1 : 0;
    return ioctlsocket(socket_fd_, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }

    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    return fcntl(socket_fd_, F_SETFL, flags) == 0;
#endif
}

bool Socket::set_receive_buffer_size(int size) {
    if (!is_valid()) {
        return false;
    }

    return setsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&size), sizeof(size)) == 0;
}

bool Socket::set_send_buffer_size(int size) {
    if (!is_valid()) {
        return false;
    }

    return setsockopt(socket_fd_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&size), sizeof(size)) == 0;
}

bool Socket::set_receive_timeout(int timeout_ms) {
    if (!is_valid()) {
        return false;
    }

#ifdef _WIN32
    DWORD timeout = timeout_ms;
    return setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0;
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
#endif
}

bool Socket::set_send_timeout(int timeout_ms) {
    if (!is_valid()) {
        return false;
    }

#ifdef _WIN32
    DWORD timeout = timeout_ms;
    return setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0;
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
#endif
}

} // namespace bitscrape::network
