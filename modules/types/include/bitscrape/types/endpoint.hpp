#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <string>
#include <string_view>

namespace bitscrape::types {

/**
 * @brief Represents a network endpoint (IP address and port)
 *
 * Endpoint encapsulates an IP address (IPv4 or IPv6) and a port number.
 * It provides methods for creating, comparing, and manipulating endpoints.
 */
class Endpoint {
public:
  /**
   * @brief IP address type
   */
  enum class AddressType {
    IPv4, ///< IPv4 address
    IPv6  ///< IPv6 address
  };

  /**
   * @brief Default constructor creates an invalid endpoint
   */
  Endpoint();

  /**
   * @brief Create an endpoint from an IP address string and port
   *
   * @param address IP address string (IPv4 or IPv6)
   * @param port Port number
   * @throws std::invalid_argument if the address is not a valid IP address
   */
  Endpoint(const std::string &address, uint16_t port);

  /**
   * @brief Create an endpoint from an IP address string view and port
   *
   * @param address IP address string view (IPv4 or IPv6)
   * @param port Port number
   * @throws std::invalid_argument if the address is not a valid IP address
   */
  Endpoint(std::string_view address, uint16_t port);

  /**
   * @brief Create an endpoint from a host string and port
   *
   * This constructor will resolve the host name to an IP address.
   *
   * @param host Host name or IP address string
   * @param port Port number
   * @param type Address type preference (IPv4 or IPv6)
   * @throws std::runtime_error if the host name cannot be resolved
   */
  Endpoint(const std::string &host, uint16_t port, AddressType type);

  /**
   * @brief Copy constructor
   */
  Endpoint(const Endpoint &other) = default;

  /**
   * @brief Move constructor
   */
  Endpoint(Endpoint &&other) noexcept = default;

  /**
   * @brief Copy assignment operator
   */
  Endpoint &operator=(const Endpoint &other) = default;

  /**
   * @brief Move assignment operator
   */
  Endpoint &operator=(Endpoint &&other) noexcept = default;

  /**
   * @brief Destructor
   */
  ~Endpoint() = default;

  /**
   * @brief Get the IP address as a string
   *
   * @return IP address string
   */
  const std::string &address() const { return address_; }

  /**
   * @brief Get the port number
   *
   * @return Port number
   */
  uint16_t port() const { return port_; }

  /**
   * @brief Get the address type
   *
   * @return Address type (IPv4 or IPv6)
   */
  AddressType type() const { return type_; }

  /**
   * @brief Check if the endpoint is valid
   *
   * @return true if the endpoint is valid, false otherwise
   */
  bool is_valid() const;

  /**
   * @brief Get the endpoint as a string in the format "address:port"
   *
   * @return String representation of the endpoint
   */
  std::string to_string() const;

  /**
   * @brief Resolve a host name to an endpoint asynchronously
   *
   * @param host Host name or IP address string
   * @param port Port number
   * @param type Address type preference (IPv4 or IPv6)
   * @return A future that will contain the resolved endpoint
   */
  static std::future<Endpoint>
  resolve_async(const std::string &host, uint16_t port,
                AddressType type = AddressType::IPv4);

  /**
   * @brief Resolve a host name to an endpoint
   *
   * @param host Host name or IP address string
   * @param port Port number
   * @param type Address type preference (IPv4 or IPv6)
   * @return The resolved endpoint
   * @throws std::runtime_error if the host name cannot be resolved
   */
  static Endpoint resolve(const std::string &host, uint16_t port,
                          AddressType type = AddressType::IPv4);

  /**
   * @brief Equality operator
   */
  bool operator==(const Endpoint &other) const;

  /**
   * @brief Inequality operator
   */
  bool operator!=(const Endpoint &other) const;

  /**
   * @brief Less than operator for ordering
   */
  bool operator<(const Endpoint &other) const;

private:
  std::string address_; ///< IP address string
  uint16_t port_;       ///< Port number
  AddressType type_;    ///< Address type (IPv4 or IPv6)

  // Helper methods
  static bool is_valid_ipv4(const std::string &address);
  static bool is_valid_ipv6(const std::string &address);
};

} // namespace bitscrape::types
