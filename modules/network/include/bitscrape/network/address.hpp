#pragma once

#include <cstdint>
#include <future>
#include <string>
#include <vector>

namespace bitscrape::network {

/**
 * @brief IP address family enumeration
 */
enum class AddressFamily { IPv4, IPv6 };

/**
 * @brief Address class for IP address and port handling
 *
 * This class provides a cross-platform abstraction for IP addresses and ports.
 * It supports both IPv4 and IPv6 addresses with synchronous and asynchronous
 * APIs.
 */
class Address {
public:
  /**
   * @brief Construct a new Address object with default values
   */
  Address();

  /**
   * @brief Construct a new Address object with any address and the specified
   * port
   *
   * @param port The port number
   * @param family The address family (IPv4 or IPv6)
   */
  explicit Address(uint16_t port, AddressFamily family = AddressFamily::IPv4);

  /**
   * @brief Construct a new Address object with the specified address and port
   *
   * @param address The IP address string
   * @param port The port number
   */
  Address(const std::string &address, uint16_t port);

  /**
   * @brief Destroy the Address object
   */
  ~Address() = default;

  /**
   * @brief Address is copyable
   */
  Address(const Address &) = default;
  Address &operator=(const Address &) = default;

  /**
   * @brief Address is movable
   */
  Address(Address &&) noexcept = default;
  Address &operator=(Address &&) noexcept = default;

  /**
   * @brief Equality operator
   *
   * @param other The address to compare with
   * @return true if the addresses are equal, false otherwise
   */
  bool operator==(const Address& other) const {
    return address_ == other.address_ && port_ == other.port_ && family_ == other.family_;
  }

  /**
   * @brief Inequality operator
   *
   * @param other The address to compare with
   * @return true if the addresses are not equal, false otherwise
   */
  bool operator!=(const Address& other) const {
    return !(*this == other);
  }

  /**
   * @brief Get the IP address as a string
   *
   * @return The IP address string
   */
  std::string to_string() const;

  /**
   * @brief Get the port number
   *
   * @return The port number
   */
  uint16_t port() const;

  /**
   * @brief Get the address family
   *
   * @return The address family
   */
  AddressFamily family() const;

  /**
   * @brief Check if the address is valid
   *
   * @return true if the address is valid, false otherwise
   */
  bool is_valid() const;

  /**
   * @brief Check if the address is a loopback address
   *
   * @return true if the address is a loopback address, false otherwise
   */
  bool is_loopback() const;

  /**
   * @brief Check if the address is a multicast address
   *
   * @return true if the address is a multicast address, false otherwise
   */
  bool is_multicast() const;

  /**
   * @brief Resolve a hostname to an address asynchronously
   *
   * @param hostname The hostname to resolve
   * @param port The port number
   * @return A future that will contain the resolved address
   */
  static std::future<Address> resolve_async(const std::string &hostname,
                                            uint16_t port);

  /**
   * @brief Resolve a hostname to an address
   *
   * @param hostname The hostname to resolve
   * @param port The port number
   * @return The resolved address
   */
  static Address resolve(const std::string &hostname, uint16_t port);

  /**
   * @brief Get the local address for a specific network interface
   *
   * @param interface_name The network interface name
   * @param port The port number
   * @return The local address
   */
  static Address get_local_address(const std::string &interface_name,
                                   uint16_t port);

  /**
   * @brief Get all local addresses
   *
   * @param port The port number
   * @return A vector of local addresses
   */
  static std::vector<Address> get_all_local_addresses(uint16_t port);

private:
  std::string address_;
  uint16_t port_;
  AddressFamily family_;
  bool valid_;
};

} // namespace bitscrape::network
