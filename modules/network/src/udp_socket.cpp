#include "bitscrape/network/udp_socket.hpp"

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

UDPSocket::UDPSocket() : socket_(std::make_unique<Socket>(SocketType::UDP)) {}

bool UDPSocket::bind(uint16_t port) { return socket_->bind(port); }

bool UDPSocket::bind(const std::string &address, uint16_t port) {
  return socket_->bind(address, port);
}

void UDPSocket::close() { socket_->close(); }

bool UDPSocket::is_valid() const { return socket_->is_valid(); }

int UDPSocket::send_to(const uint8_t *data, size_t size,
                       const Address &address) {
  if (!is_valid() || data == nullptr || size == 0 || !address.is_valid()) {
    return -1;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(address.port());
  addr.sin_addr.s_addr = inet_addr(address.to_string().c_str());

  return sendto(socket_->descriptor(), reinterpret_cast<const char *>(data),
                size, 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
}

std::future<int> UDPSocket::send_to_async(const uint8_t *data, size_t size,
                                          const Address &address) {
  return std::async(std::launch::async, [this, data, size, address]() {
    return send_to(data, size, address);
  });
}

int UDPSocket::send_to(const Buffer &buffer, const Address &address) {
  return send_to(buffer.data(), buffer.size(), address);
}

std::future<int> UDPSocket::send_to_async(const Buffer &buffer,
                                          const Address &address) {
  return std::async(std::launch::async, [this, buffer, address]() {
    return send_to(buffer, address);
  });
}

int UDPSocket::receive_from(uint8_t *data, size_t size, Address &address) {
  if (!is_valid() || data == nullptr || size == 0) {
    return -1;
  }

  sockaddr_in addr{};
  socklen_t addr_len = sizeof(addr);

  int bytes_received =
      recvfrom(socket_->descriptor(), reinterpret_cast<char *>(data), size, 0,
               reinterpret_cast<sockaddr *>(&addr), &addr_len);

  if (bytes_received > 0) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    address = Address(ip_str, ntohs(addr.sin_port));
  }

  return bytes_received;
}

std::future<std::pair<int, Address>>
UDPSocket::receive_from_async(uint8_t *data, size_t size) {
  return std::async(std::launch::async, [this, data, size]() {
    Address address;
    int bytes_received = receive_from(data, size, address);
    return std::make_pair(bytes_received, address);
  });
}

int UDPSocket::receive_from(Buffer &buffer, Address &address) {
  // Resize the buffer to a reasonable size for receiving
  buffer.resize(1500); // MTU size

  int bytes_received = receive_from(buffer.data(), buffer.size(), address);
  if (bytes_received > 0) {
    buffer.resize(bytes_received);
  } else {
    buffer.clear();
  }

  return bytes_received;
}

std::future<std::pair<int, Address>>
UDPSocket::receive_from_async(Buffer &buffer) {
  return std::async(std::launch::async, [this, &buffer]() {
    Address address;
    int bytes_received = receive_from(buffer, address);
    return std::make_pair(bytes_received, address);
  });
}

bool UDPSocket::set_non_blocking(bool non_blocking) {
  return socket_->set_non_blocking(non_blocking);
}

bool UDPSocket::set_receive_buffer_size(int size) {
  return socket_->set_receive_buffer_size(size);
}

bool UDPSocket::set_send_buffer_size(int size) {
  return socket_->set_send_buffer_size(size);
}

bool UDPSocket::set_receive_timeout(int timeout_ms) {
  return socket_->set_receive_timeout(timeout_ms);
}

bool UDPSocket::set_send_timeout(int timeout_ms) {
  return socket_->set_send_timeout(timeout_ms);
}

} // namespace bitscrape::network
