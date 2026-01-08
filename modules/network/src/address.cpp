#include "bitscrape/network/address.hpp"

#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace bitscrape::network {

Address::Address()
    : address_("0.0.0.0"), port_(0), family_(AddressFamily::IPv4),
      valid_(true) {}

Address::Address(uint16_t port, AddressFamily family)
    : port_(port), family_(family), valid_(true) {
  address_ = (family == AddressFamily::IPv4) ? "0.0.0.0" : "::";
}

Address::Address(const std::string &address, uint16_t port)
    : address_(address), port_(port), valid_(true) {
  // Determine the address family based on the address format
  if (address.find(':') != std::string::npos) {
    family_ = AddressFamily::IPv6;
  } else {
    family_ = AddressFamily::IPv4;
  }

  // Validate the address
  if (family_ == AddressFamily::IPv4) {
    struct in_addr addr;
    valid_ = (inet_pton(AF_INET, address.c_str(), &addr) == 1);
  } else {
    struct in6_addr addr;
    valid_ = (inet_pton(AF_INET6, address.c_str(), &addr) == 1);
  }
}

std::string Address::to_string() const { return address_; }

uint16_t Address::port() const { return port_; }

AddressFamily Address::family() const { return family_; }

bool Address::is_valid() const { return valid_; }

bool Address::is_loopback() const {
  if (!valid_) {
    return false;
  }

  if (family_ == AddressFamily::IPv4) {
    return address_ == "127.0.0.1" || address_ == "localhost";
  } else {
    return address_ == "::1" || address_ == "localhost";
  }
}

bool Address::is_multicast() const {
  if (!valid_) {
    return false;
  }

  if (family_ == AddressFamily::IPv4) {
    // IPv4 multicast addresses start with 224-239
    struct in_addr addr;
    if (inet_pton(AF_INET, address_.c_str(), &addr) != 1) {
      return false;
    }
    uint32_t ip = ntohl(addr.s_addr);
    return (ip & 0xF0000000) == 0xE0000000;
  } else {
    // IPv6 multicast addresses start with ff
    struct in6_addr addr;
    if (inet_pton(AF_INET6, address_.c_str(), &addr) != 1) {
      return false;
    }
    return addr.s6_addr[0] == 0xFF;
  }
}

std::future<Address> Address::resolve_async(const std::string &hostname,
                                            uint16_t port) {
  return std::async(std::launch::async,
                    [hostname, port]() { return resolve(hostname, port); });
}

Address Address::resolve(const std::string &hostname, uint16_t port) {
  struct addrinfo hints{};
  struct addrinfo *result = nullptr;

  hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // Stream socket
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
  if (status != 0) {
    throw std::runtime_error("Failed to resolve hostname: " +
                             std::string(gai_strerror(status)));
  }

  std::string address;

  if (result->ai_family == AF_INET) {
    // IPv4
    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in *addr =
        reinterpret_cast<struct sockaddr_in *>(result->ai_addr);
    inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
    address = ip_str;
  } else {
    // IPv6
    char ip_str[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *addr =
        reinterpret_cast<struct sockaddr_in6 *>(result->ai_addr);
    inet_ntop(AF_INET6, &(addr->sin6_addr), ip_str, INET6_ADDRSTRLEN);
    address = ip_str;
  }

  freeaddrinfo(result);

  return Address(address, port);
}

Address Address::get_local_address(const std::string &interface_name,
                                   uint16_t port) {
#ifdef _WIN32
  // Windows implementation
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    throw std::system_error(errno, std::generic_category(),
                            "Failed to get hostname");
  }

  struct addrinfo hints{};
  struct addrinfo *result = nullptr;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int status = getaddrinfo(hostname, nullptr, &hints, &result);
  if (status != 0) {
    throw std::runtime_error("Failed to get address info: " +
                             std::string(gai_strerror(status)));
  }

  std::string address;

  for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
    if (rp->ai_family == AF_INET) {
      // IPv4
      char ip_str[INET_ADDRSTRLEN];
      struct sockaddr_in *addr =
          reinterpret_cast<struct sockaddr_in *>(rp->ai_addr);
      inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
      address = ip_str;
      break;
    } else if (rp->ai_family == AF_INET6) {
      // IPv6
      char ip_str[INET6_ADDRSTRLEN];
      struct sockaddr_in6 *addr =
          reinterpret_cast<struct sockaddr_in6 *>(rp->ai_addr);
      inet_ntop(AF_INET6, &(addr->sin6_addr), ip_str, INET6_ADDRSTRLEN);
      address = ip_str;
      break;
    }
  }

  freeaddrinfo(result);

  if (address.empty()) {
    throw std::runtime_error("Failed to get local address");
  }

  return Address(address, port);
#else
  // Unix implementation
  struct ifaddrs *ifaddr = nullptr;
  if (getifaddrs(&ifaddr) == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "Failed to get interface addresses");
  }

  std::string address;
  bool found = false;

  for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr) {
      continue;
    }

    if (interface_name.empty() || interface_name == ifa->ifa_name) {
      if (ifa->ifa_addr->sa_family == AF_INET) {
        // IPv4
        char ip_str[INET_ADDRSTRLEN];
        struct sockaddr_in *addr =
            reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
        inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
        address = ip_str;
        found = true;
        break;
      } else if (ifa->ifa_addr->sa_family == AF_INET6) {
        // IPv6
        char ip_str[INET6_ADDRSTRLEN];
        struct sockaddr_in6 *addr =
            reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
        inet_ntop(AF_INET6, &(addr->sin6_addr), ip_str, INET6_ADDRSTRLEN);
        address = ip_str;
        found = true;
        break;
      }
    }
  }

  freeifaddrs(ifaddr);

  if (!found) {
    throw std::runtime_error("Failed to get local address for interface: " +
                             interface_name);
  }

  return Address(address, port);
#endif
}

std::vector<Address> Address::get_all_local_addresses(uint16_t port) {
  std::vector<Address> addresses;

#ifdef _WIN32
  // Windows implementation
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    throw std::system_error(errno, std::generic_category(),
                            "Failed to get hostname");
  }

  struct addrinfo hints{};
  struct addrinfo *result = nullptr;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int status = getaddrinfo(hostname, nullptr, &hints, &result);
  if (status != 0) {
    throw std::runtime_error("Failed to get address info: " +
                             std::string(gai_strerror(status)));
  }

  for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
    if (rp->ai_family == AF_INET) {
      // IPv4
      char ip_str[INET_ADDRSTRLEN];
      struct sockaddr_in *addr =
          reinterpret_cast<struct sockaddr_in *>(rp->ai_addr);
      inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
      addresses.emplace_back(ip_str, port);
    } else if (rp->ai_family == AF_INET6) {
      // IPv6
      char ip_str[INET6_ADDRSTRLEN];
      struct sockaddr_in6 *addr =
          reinterpret_cast<struct sockaddr_in6 *>(rp->ai_addr);
      inet_ntop(AF_INET6, &(addr->sin6_addr), ip_str, INET6_ADDRSTRLEN);
      addresses.emplace_back(ip_str, port);
    }
  }

  freeaddrinfo(result);
#else
  // Unix implementation
  struct ifaddrs *ifaddr = nullptr;
  if (getifaddrs(&ifaddr) == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "Failed to get interface addresses");
  }

  for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr) {
      continue;
    }

    if (ifa->ifa_addr->sa_family == AF_INET) {
      // IPv4
      char ip_str[INET_ADDRSTRLEN];
      struct sockaddr_in *addr =
          reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
      inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
      addresses.emplace_back(ip_str, port);
    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
      // IPv6
      char ip_str[INET6_ADDRSTRLEN];
      struct sockaddr_in6 *addr =
          reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
      inet_ntop(AF_INET6, &(addr->sin6_addr), ip_str, INET6_ADDRSTRLEN);
      addresses.emplace_back(ip_str, port);
    }
  }

  freeifaddrs(ifaddr);
#endif

  return addresses;
}

} // namespace bitscrape::network
