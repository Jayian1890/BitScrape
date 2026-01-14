#pragma once

#include "bitscrape/bencode/bencode_value.hpp"
#include "bitscrape/bittorrent/peer_message.hpp"

namespace bitscrape::bittorrent {

/**
 * @brief Extended message (BEP 10)
 *
 * Extended messages are used to implement extensions to the BitTorrent
 * protocol. Format: <length prefix><message id = 20><extended message
 * id><payload>
 */
class ExtendedMessage : public PeerMessage {
public:
  /**
   * @brief Constructor
   *
   * @param extended_type Extended message type (0 = handshake)
   * @param payload Payload data
   */
  ExtendedMessage(uint8_t extended_type, const bencode::BencodeValue &payload);

  /**
   * @brief Constructor with trailing data (for BEP-9 data messages)
   *
   * @param extended_type Extended message type
   * @param payload Bencoded payload dictionary
   * @param trailing_data Raw bytes appended after the bencoded dictionary
   */
  ExtendedMessage(uint8_t extended_type, const bencode::BencodeValue &payload,
                  const std::vector<uint8_t> &trailing_data);

  /**
   * @brief Get the extended message type
   *
   * @return Extended message type
   */
  [[nodiscard]] uint8_t extended_type() const;

  [[nodiscard]] const bencode::BencodeValue &payload() const;

  /**
   * @brief Get the trailing data (for BEP-9 data messages)
   *
   * @return Trailing data bytes (may be empty)
   */
  [[nodiscard]] const std::vector<uint8_t> &trailing_data() const;

  /**
   * @brief Serialize the message to a byte vector
   *
   * @return Serialized message
   */
  [[nodiscard]] std::vector<uint8_t> serialize() const override;

  /**
   * @brief Get a string representation of the message
   *
   * @return String representation
   */
  [[nodiscard]] std::string to_string() const override;

private:
  uint8_t extended_type_;         ///< Extended message type (0 = handshake)
  bencode::BencodeValue payload_; ///< Payload data
  std::vector<uint8_t>
      trailing_data_; ///< Raw bytes after bencoded dict (BEP-9)
};

} // namespace bitscrape::bittorrent
