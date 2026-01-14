#include "bitscrape/bittorrent/extended_message.hpp"
#include "bitscrape/bencode/bencode_encoder.hpp"

#include <iomanip>
#include <sstream>

namespace bitscrape::bittorrent {

ExtendedMessage::ExtendedMessage(uint8_t extended_type,
                                 const bencode::BencodeValue &payload)
    : PeerMessage(PeerMessageType::EXTENDED), extended_type_(extended_type),
      payload_(payload), trailing_data_() {}

ExtendedMessage::ExtendedMessage(uint8_t extended_type,
                                 const bencode::BencodeValue &payload,
                                 const std::vector<uint8_t> &trailing_data)
    : PeerMessage(PeerMessageType::EXTENDED), extended_type_(extended_type),
      payload_(payload), trailing_data_(trailing_data) {}

uint8_t ExtendedMessage::extended_type() const { return extended_type_; }

const bencode::BencodeValue &ExtendedMessage::payload() const {
  return payload_;
}

const std::vector<uint8_t> &ExtendedMessage::trailing_data() const {
  return trailing_data_;
}

std::vector<uint8_t> ExtendedMessage::serialize() const {
  // Encode the payload
  auto encoder = bencode::create_bencode_encoder();
  std::vector<uint8_t> payload_data = encoder->encode(payload_);

  // Calculate the message length
  uint32_t length =
      2 + static_cast<uint32_t>(
              payload_data.size()); // 1 byte for message type + 1 byte for
                                    // extended type + payload

  // Create the message
  std::vector<uint8_t> result;
  result.reserve(4 + length); // 4 bytes for length prefix + message data

  // Add length prefix (big-endian)
  result.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
  result.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
  result.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
  result.push_back(static_cast<uint8_t>(length & 0xFF));

  // Add message type
  result.push_back(static_cast<uint8_t>(PeerMessageType::EXTENDED));

  // Add extended message type
  result.push_back(extended_type_);

  // Add payload
  result.insert(result.end(), payload_data.begin(), payload_data.end());

  return result;
}

std::string ExtendedMessage::to_string() const {
  std::ostringstream oss;

  oss << "ExtendedMessage[type=" << static_cast<int>(extended_type_)
      << ", payload=";

  // Format payload as string
  if (payload_.is_dict()) {
    oss << "{";
    bool first = true;
    for (const auto &pair : payload_.as_dict()) {
      if (!first) {
        oss << ", ";
      }
      first = false;
      oss << pair.first << "=";
      if (pair.second.is_string()) {
        oss << "\"" << pair.second.as_string() << "\"";
      } else if (pair.second.is_integer()) {
        oss << pair.second.as_integer();
      } else if (pair.second.is_list()) {
        oss << "[list]";
      } else if (pair.second.is_dict()) {
        oss << "{dict}";
      }
    }
    oss << "}";
  } else if (payload_.is_list()) {
    oss << "[list]";
  } else if (payload_.is_string()) {
    oss << "\"" << payload_.as_string() << "\"";
  } else if (payload_.is_integer()) {
    oss << payload_.as_integer();
  }

  oss << "]";

  return oss.str();
}

} // namespace bitscrape::bittorrent
