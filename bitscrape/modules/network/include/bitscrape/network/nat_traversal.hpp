#pragma once

#include "bitscrape/network/address.hpp"

#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace bitscrape::network {

/**
 * @brief NAT traversal protocol enumeration
 */
enum class NATProtocol { UPnP, NAT_PMP, AUTO };

/**
 * @brief NAT traversal result structure
 */
struct NATTraversalResult {
  bool success = false;
  std::string error_message;
  Address external_address;
  uint16_t external_port = 0;
  uint16_t internal_port = 0;
  std::string protocol;
  int lease_duration = 0;

  NATTraversalResult() = default;
};

/**
 * @brief NATTraversal class for NAT traversal functionality
 *
 * This class provides NAT traversal functionality using UPnP and NAT-PMP.
 * It supports both synchronous and asynchronous APIs for port mapping.
 */
class NATTraversal {
public:
  /**
   * @brief Construct a new NATTraversal object
   *
   * @param protocol The NAT traversal protocol to use
   */
  explicit NATTraversal(NATProtocol protocol = NATProtocol::AUTO);

  /**
   * @brief Destroy the NATTraversal object
   */
  ~NATTraversal();

  /**
   * @brief NATTraversal is non-copyable
   */
  NATTraversal(const NATTraversal &) = delete;
  NATTraversal &operator=(const NATTraversal &) = delete;

  /**
   * @brief NATTraversal is movable
   */
  NATTraversal(NATTraversal &&) noexcept;
  NATTraversal &operator=(NATTraversal &&) noexcept;

  /**
   * @brief Initialize the NAT traversal
   *
   * @return true if successful, false otherwise
   */
  bool initialize();

  /**
   * @brief Initialize the NAT traversal asynchronously
   *
   * @return A future that will contain true if successful, false otherwise
   */
  std::future<bool> initialize_async();

  /**
   * @brief Check if NAT traversal is available
   *
   * @return true if NAT traversal is available, false otherwise
   */
  bool is_available() const;

  /**
   * @brief Get the external address
   *
   * @return The external address
   */
  Address get_external_address() const;

  /**
   * @brief Get the external address asynchronously
   *
   * @return A future that will contain the external address
   */
  std::future<Address> get_external_address_async() const;

  /**
   * @brief Add a port mapping
   *
   * @param internal_port The internal port
   * @param external_port The external port (0 for same as internal)
   * @param protocol The protocol ("tcp" or "udp")
   * @param description The mapping description
   * @param lease_duration The lease duration in seconds (0 for permanent)
   * @return The NAT traversal result
   */
  NATTraversalResult add_port_mapping(uint16_t internal_port,
                                      uint16_t external_port = 0,
                                      const std::string &protocol = "udp",
                                      const std::string &description = "",
                                      int lease_duration = 0);

  /**
   * @brief Add a port mapping asynchronously
   *
   * @param internal_port The internal port
   * @param external_port The external port (0 for same as internal)
   * @param protocol The protocol ("tcp" or "udp")
   * @param description The mapping description
   * @param lease_duration The lease duration in seconds (0 for permanent)
   * @return A future that will contain the NAT traversal result
   */
  std::future<NATTraversalResult>
  add_port_mapping_async(uint16_t internal_port, uint16_t external_port = 0,
                         const std::string &protocol = "udp",
                         const std::string &description = "",
                         int lease_duration = 0);

  /**
   * @brief Delete a port mapping
   *
   * @param external_port The external port
   * @param protocol The protocol ("tcp" or "udp")
   * @return true if successful, false otherwise
   */
  bool delete_port_mapping(uint16_t external_port,
                           const std::string &protocol = "udp");

  /**
   * @brief Delete a port mapping asynchronously
   *
   * @param external_port The external port
   * @param protocol The protocol ("tcp" or "udp")
   * @return A future that will contain true if successful, false otherwise
   */
  std::future<bool>
  delete_port_mapping_async(uint16_t external_port,
                            const std::string &protocol = "udp");

  /**
   * @brief Get all port mappings
   *
   * @return A vector of NAT traversal results
   */
  std::vector<NATTraversalResult> get_all_port_mappings();

  /**
   * @brief Get all port mappings asynchronously
   *
   * @return A future that will contain a vector of NAT traversal results
   */
  std::future<std::vector<NATTraversalResult>> get_all_port_mappings_async();

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace bitscrape::network
