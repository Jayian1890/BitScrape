#include "bitscrape/types/endpoint.hpp"

#include <regex>
#include <stdexcept>
#include <sstream>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace bitscrape::types {

Endpoint::Endpoint() : port_(0), type_(AddressType::IPv4) {
    // Create an invalid endpoint
}

Endpoint::Endpoint(const std::string& address, uint16_t port) : address_(address), port_(port) {
    // Validate and determine the address type
    if (is_valid_ipv4(address)) {
        type_ = AddressType::IPv4;
    } else if (is_valid_ipv6(address)) {
        type_ = AddressType::IPv6;
    } else {
        throw std::invalid_argument("Endpoint: Invalid IP address");
    }
}

Endpoint::Endpoint(std::string_view address, uint16_t port) 
    : Endpoint(std::string(address), port) {
}

Endpoint::Endpoint(const std::string& host, uint16_t port, AddressType type) 
    : port_(port), type_(type) {
    // Resolve the host name
    *this = resolve(host, port, type);
}

bool Endpoint::is_valid() const {
    return !address_.empty() && port_ > 0;
}

std::string Endpoint::to_string() const {
    std::ostringstream oss;
    
    if (type_ == AddressType::IPv6) {
        oss << "[" << address_ << "]:" << port_;
    } else {
        oss << address_ << ":" << port_;
    }
    
    return oss.str();
}

Endpoint Endpoint::resolve(const std::string& host, uint16_t port, AddressType type) {
    struct addrinfo hints = {};
    struct addrinfo* result = nullptr;
    
    // Set up hints for getaddrinfo
    hints.ai_family = (type == AddressType::IPv4) ? AF_INET : AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    
    // Resolve the host name
    int status = getaddrinfo(host.c_str(), nullptr, &hints, &result);
    if (status != 0) {
        throw std::runtime_error("Endpoint: Failed to resolve host: " + 
                                std::string(gai_strerror(status)));
    }
    
    // Extract the IP address
    char ip_str[INET6_ADDRSTRLEN] = {};
    if (result->ai_family == AF_INET) {
        auto* addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
        inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
    } else {
        auto* addr = reinterpret_cast<struct sockaddr_in6*>(result->ai_addr);
        inet_ntop(AF_INET6, &(addr->sin6_addr), ip_str, INET6_ADDRSTRLEN);
    }
    
    freeaddrinfo(result);
    
    // Create and return the endpoint
    return Endpoint(ip_str, port);
}

std::future<Endpoint> Endpoint::resolve_async(const std::string& host, uint16_t port, AddressType type) {
    return std::async(std::launch::async, [host, port, type]() {
        return resolve(host, port, type);
    });
}

bool Endpoint::operator==(const Endpoint& other) const {
    return address_ == other.address_ && port_ == other.port_;
}

bool Endpoint::operator!=(const Endpoint& other) const {
    return !(*this == other);
}

bool Endpoint::operator<(const Endpoint& other) const {
    if (address_ != other.address_) {
        return address_ < other.address_;
    }
    return port_ < other.port_;
}

// Private helper methods
bool Endpoint::is_valid_ipv4(const std::string& address) {
    struct in_addr addr;
    return inet_pton(AF_INET, address.c_str(), &addr) == 1;
}

bool Endpoint::is_valid_ipv6(const std::string& address) {
    struct in6_addr addr;
    return inet_pton(AF_INET6, address.c_str(), &addr) == 1;
}

} // namespace bitscrape::types
