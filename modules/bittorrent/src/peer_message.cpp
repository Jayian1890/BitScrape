#include "bitscrape/bittorrent/peer_message.hpp"
#include "bitscrape/bencode/bencode_decoder.hpp"
#include "bitscrape/bittorrent/extended_message.hpp"

#include <iomanip>
#include <sstream>

namespace bitscrape::bittorrent {

// PeerMessage implementation

PeerMessage::PeerMessage(PeerMessageType type) : type_(type) {}

PeerMessageType PeerMessage::type() const { return type_; }

// HandshakeMessage implementation

HandshakeMessage::HandshakeMessage(const std::vector<uint8_t> &info_hash,
                                   const std::vector<uint8_t> &peer_id,
                                   const std::vector<uint8_t> &reserved)
    : PeerMessage(PeerMessageType::HANDSHAKE), info_hash_(info_hash),
      peer_id_(peer_id), reserved_(reserved) {
  // Ensure info_hash is 20 bytes
  if (info_hash_.size() != 20) {
    throw std::invalid_argument("Info hash must be 20 bytes");
  }

  // Ensure peer_id is 20 bytes
  if (peer_id_.size() != 20) {
    throw std::invalid_argument("Peer ID must be 20 bytes");
  }

  // Ensure reserved is 8 bytes
  if (reserved_.size() != 8) {
    reserved_.resize(8, 0);
  }
}

const std::vector<uint8_t> &HandshakeMessage::info_hash() const {
  return info_hash_;
}

const std::vector<uint8_t> &HandshakeMessage::peer_id() const {
  return peer_id_;
}

const std::vector<uint8_t> &HandshakeMessage::reserved() const {
  return reserved_;
}

std::vector<uint8_t> HandshakeMessage::serialize() const {
  // Handshake format: <pstrlen><pstr><reserved><info_hash><peer_id>
  // pstrlen = 19 (single byte)
  // pstr = "BitTorrent protocol" (19 bytes)
  // reserved = 8 bytes
  // info_hash = 20 bytes
  // peer_id = 20 bytes
  // Total: 1 + 19 + 8 + 20 + 20 = 68 bytes

  std::vector<uint8_t> result;
  result.reserve(68);

  // pstrlen
  result.push_back(19);

  // pstr
  const std::string protocol = "BitTorrent protocol";
  result.insert(result.end(), protocol.begin(), protocol.end());

  // reserved
  result.insert(result.end(), reserved_.begin(), reserved_.end());

  // info_hash
  result.insert(result.end(), info_hash_.begin(), info_hash_.end());

  // peer_id
  result.insert(result.end(), peer_id_.begin(), peer_id_.end());

  return result;
}

std::string HandshakeMessage::to_string() const {
  std::ostringstream oss;

  oss << "HandshakeMessage[info_hash=";

  // Format info_hash as hex
  for (const auto &byte : info_hash_) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(byte);
  }

  oss << ", peer_id=";

  // Format peer_id as hex
  for (const auto &byte : peer_id_) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(byte);
  }

  oss << "]";

  return oss.str();
}

// KeepAliveMessage implementation

KeepAliveMessage::KeepAliveMessage()
    : PeerMessage(PeerMessageType::KEEP_ALIVE) {}

std::vector<uint8_t> KeepAliveMessage::serialize() const {
  // Keep-alive message is just a length prefix with a value of 0
  std::vector<uint8_t> result(4, 0);
  return result;
}

std::string KeepAliveMessage::to_string() const { return "KeepAliveMessage[]"; }

// PeerMessageFactory implementation

std::shared_ptr<PeerMessage>
PeerMessageFactory::create_from_data(const std::vector<uint8_t> &data) {
  // Check if data is empty
  if (data.empty()) {
    return nullptr;
  }

  // Get message type
  auto type = static_cast<PeerMessageType>(data[0]);

  // Create message based on type
  if (type == PeerMessageType::HANDSHAKE) {
    // Handshake message is special and should be parsed separately
    return nullptr;
  }

  if (type == PeerMessageType::KEEP_ALIVE) {
    return create_keep_alive();
  }

  if (type == PeerMessageType::EXTENDED) {
    // Extended message (BEP 10)
    if (data.size() < 2) {
      return nullptr;
    }

    // Get extended message type
    uint8_t extended_type = data[1];

    // Get payload
    std::vector<uint8_t> payload_data(data.begin() + 2, data.end());

    // Decode payload
    try {
      auto decoder = bencode::create_bencode_decoder();
      bencode::BencodeValue payload = decoder->decode(payload_data);

      // Create extended message
      return std::make_shared<ExtendedMessage>(extended_type, payload);
    } catch (const std::exception &) {
      // Failed to decode payload
      return nullptr;
    }
  }

  // Handle standard message types
  if (type == PeerMessageType::CHOKE) {
    return create_choke();
  } else if (type == PeerMessageType::UNCHOKE) {
    return create_unchoke();
  } else if (type == PeerMessageType::INTERESTED) {
    return create_interested();
  } else if (type == PeerMessageType::NOT_INTERESTED) {
    return create_not_interested();
  } else if (type == PeerMessageType::HAVE) {
    if (data.size() < 5) { // 1 byte type + 4 bytes piece index
      return nullptr;
    }
    uint32_t piece_index =
        (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
    return create_have(piece_index);
  } else if (type == PeerMessageType::BITFIELD) {
    if (data.size() < 2) { // 1 byte type + at least 1 byte bitfield
      return nullptr;
    }
    std::vector<uint8_t> bitfield(data.begin() + 1, data.end());
    return create_bitfield(bitfield);
  } else if (type == PeerMessageType::REQUEST) {
    if (data.size() <
        13) { // 1 byte type + 4 bytes index + 4 bytes begin + 4 bytes length
      return nullptr;
    }
    uint32_t index =
        (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
    uint32_t begin =
        (data[5] << 24) | (data[6] << 16) | (data[7] << 8) | data[8];
    uint32_t length =
        (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];
    return create_request(index, begin, length);
  } else if (type == PeerMessageType::PIECE) {
    if (data.size() < 9) { // 1 byte type + 4 bytes index + 4 bytes begin + at
                           // least 0 bytes block
      return nullptr;
    }
    uint32_t index =
        (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
    uint32_t begin =
        (data[5] << 24) | (data[6] << 16) | (data[7] << 8) | data[8];
    std::vector<uint8_t> block(data.begin() + 9, data.end());
    return create_piece(index, begin, block);
  } else if (type == PeerMessageType::CANCEL) {
    if (data.size() <
        13) { // 1 byte type + 4 bytes index + 4 bytes begin + 4 bytes length
      return nullptr;
    }
    uint32_t index =
        (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
    uint32_t begin =
        (data[5] << 24) | (data[6] << 16) | (data[7] << 8) | data[8];
    uint32_t length =
        (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];
    return create_cancel(index, begin, length);
  } else if (type == PeerMessageType::PORT) {
    if (data.size() < 3) { // 1 byte type + 2 bytes port
      return nullptr;
    }
    uint16_t port = (data[1] << 8) | data[2];
    return create_port(port);
  }

  // Unknown message type
  return nullptr;
}

std::shared_ptr<HandshakeMessage>
PeerMessageFactory::create_handshake(const std::vector<uint8_t> &info_hash,
                                     const std::vector<uint8_t> &peer_id,
                                     const std::vector<uint8_t> &reserved) {
  return std::make_shared<HandshakeMessage>(info_hash, peer_id, reserved);
}

std::shared_ptr<KeepAliveMessage> PeerMessageFactory::create_keep_alive() {
  return std::make_shared<KeepAliveMessage>();
}

// ChokeMessage implementation

ChokeMessage::ChokeMessage() : PeerMessage(PeerMessageType::CHOKE) {}

std::vector<uint8_t> ChokeMessage::serialize() const {
  // Choke message format: <len=0001><id=0>
  std::vector<uint8_t> result(5, 0);

  // Set length (4 bytes, big-endian)
  result[3] = 1; // Length is 1 byte

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::CHOKE);

  return result;
}

std::string ChokeMessage::to_string() const { return "ChokeMessage[]"; }

// UnchokeMessage implementation

UnchokeMessage::UnchokeMessage() : PeerMessage(PeerMessageType::UNCHOKE) {}

std::vector<uint8_t> UnchokeMessage::serialize() const {
  // Unchoke message format: <len=0001><id=1>
  std::vector<uint8_t> result(5, 0);

  // Set length (4 bytes, big-endian)
  result[3] = 1; // Length is 1 byte

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::UNCHOKE);

  return result;
}

std::string UnchokeMessage::to_string() const { return "UnchokeMessage[]"; }

// InterestedMessage implementation

InterestedMessage::InterestedMessage()
    : PeerMessage(PeerMessageType::INTERESTED) {}

std::vector<uint8_t> InterestedMessage::serialize() const {
  // Interested message format: <len=0001><id=2>
  std::vector<uint8_t> result(5, 0);

  // Set length (4 bytes, big-endian)
  result[3] = 1; // Length is 1 byte

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::INTERESTED);

  return result;
}

std::string InterestedMessage::to_string() const {
  return "InterestedMessage[]";
}

// NotInterestedMessage implementation

NotInterestedMessage::NotInterestedMessage()
    : PeerMessage(PeerMessageType::NOT_INTERESTED) {}

std::vector<uint8_t> NotInterestedMessage::serialize() const {
  // Not interested message format: <len=0001><id=3>
  std::vector<uint8_t> result(5, 0);

  // Set length (4 bytes, big-endian)
  result[3] = 1; // Length is 1 byte

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::NOT_INTERESTED);

  return result;
}

std::string NotInterestedMessage::to_string() const {
  return "NotInterestedMessage[]";
}

// HaveMessage implementation

HaveMessage::HaveMessage(uint32_t piece_index)
    : PeerMessage(PeerMessageType::HAVE), piece_index_(piece_index) {}

uint32_t HaveMessage::piece_index() const { return piece_index_; }

std::vector<uint8_t> HaveMessage::serialize() const {
  // Have message format: <len=0005><id=4><piece index>
  std::vector<uint8_t> result(9, 0);

  // Set length (4 bytes, big-endian)
  result[3] = 5; // Length is 5 bytes (1 byte ID + 4 bytes piece index)

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::HAVE);

  // Set piece index (4 bytes, big-endian)
  result[5] = static_cast<uint8_t>((piece_index_ >> 24) & 0xFF);
  result[6] = static_cast<uint8_t>((piece_index_ >> 16) & 0xFF);
  result[7] = static_cast<uint8_t>((piece_index_ >> 8) & 0xFF);
  result[8] = static_cast<uint8_t>(piece_index_ & 0xFF);

  return result;
}

std::string HaveMessage::to_string() const {
  std::ostringstream oss;
  oss << "HaveMessage[piece_index=" << piece_index_ << "]";
  return oss.str();
}

// BitfieldMessage implementation

BitfieldMessage::BitfieldMessage(const std::vector<uint8_t> &bitfield)
    : PeerMessage(PeerMessageType::BITFIELD), bitfield_(bitfield) {}

const std::vector<uint8_t> &BitfieldMessage::bitfield() const {
  return bitfield_;
}

std::vector<uint8_t> BitfieldMessage::serialize() const {
  // Bitfield message format: <len=0001+X><id=5><bitfield>
  // where X is the length of the bitfield
  std::vector<uint8_t> result(5 + bitfield_.size(), 0);

  // Set length (4 bytes, big-endian)
  uint32_t length =
      1 + static_cast<uint32_t>(bitfield_.size()); // 1 byte ID + bitfield
  result[0] = static_cast<uint8_t>((length >> 24) & 0xFF);
  result[1] = static_cast<uint8_t>((length >> 16) & 0xFF);
  result[2] = static_cast<uint8_t>((length >> 8) & 0xFF);
  result[3] = static_cast<uint8_t>(length & 0xFF);

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::BITFIELD);

  // Set bitfield
  std::copy(bitfield_.begin(), bitfield_.end(), result.begin() + 5);

  return result;
}

std::string BitfieldMessage::to_string() const {
  std::ostringstream oss;
  oss << "BitfieldMessage[size=" << bitfield_.size() << "]";
  return oss.str();
}

// RequestMessage implementation

RequestMessage::RequestMessage(uint32_t index, uint32_t begin, uint32_t length)
    : PeerMessage(PeerMessageType::REQUEST), index_(index), begin_(begin),
      length_(length) {}

uint32_t RequestMessage::index() const { return index_; }

uint32_t RequestMessage::begin() const { return begin_; }

uint32_t RequestMessage::length() const { return length_; }

std::vector<uint8_t> RequestMessage::serialize() const {
  // Request message format: <len=0013><id=6><index><begin><length>
  std::vector<uint8_t> result(17, 0);

  // Set length (4 bytes, big-endian)
  result[3] = 13; // Length is 13 bytes (1 byte ID + 4 bytes index + 4 bytes
                  // begin + 4 bytes length)

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::REQUEST);

  // Set index (4 bytes, big-endian)
  result[5] = static_cast<uint8_t>((index_ >> 24) & 0xFF);
  result[6] = static_cast<uint8_t>((index_ >> 16) & 0xFF);
  result[7] = static_cast<uint8_t>((index_ >> 8) & 0xFF);
  result[8] = static_cast<uint8_t>(index_ & 0xFF);

  // Set begin (4 bytes, big-endian)
  result[9] = static_cast<uint8_t>((begin_ >> 24) & 0xFF);
  result[10] = static_cast<uint8_t>((begin_ >> 16) & 0xFF);
  result[11] = static_cast<uint8_t>((begin_ >> 8) & 0xFF);
  result[12] = static_cast<uint8_t>(begin_ & 0xFF);

  // Set length (4 bytes, big-endian)
  result[13] = static_cast<uint8_t>((length_ >> 24) & 0xFF);
  result[14] = static_cast<uint8_t>((length_ >> 16) & 0xFF);
  result[15] = static_cast<uint8_t>((length_ >> 8) & 0xFF);
  result[16] = static_cast<uint8_t>(length_ & 0xFF);

  return result;
}

std::string RequestMessage::to_string() const {
  std::ostringstream oss;
  oss << "RequestMessage[index=" << index_ << ", begin=" << begin_
      << ", length=" << length_ << "]";
  return oss.str();
}

// PieceMessage implementation

PieceMessage::PieceMessage(uint32_t index, uint32_t begin,
                           const std::vector<uint8_t> &block)
    : PeerMessage(PeerMessageType::PIECE), index_(index), begin_(begin),
      block_(block) {}

uint32_t PieceMessage::index() const { return index_; }

uint32_t PieceMessage::begin() const { return begin_; }

const std::vector<uint8_t> &PieceMessage::block() const { return block_; }

std::vector<uint8_t> PieceMessage::serialize() const {
  // Piece message format: <len=0009+X><id=7><index><begin><block>
  // where X is the length of the block
  std::vector<uint8_t> result(9 + block_.size(), 0);

  // Set length (4 bytes, big-endian)
  uint32_t length =
      9 +
      static_cast<uint32_t>(
          block_.size()); // 1 byte ID + 4 bytes index + 4 bytes begin + block
  result[0] = static_cast<uint8_t>((length >> 24) & 0xFF);
  result[1] = static_cast<uint8_t>((length >> 16) & 0xFF);
  result[2] = static_cast<uint8_t>((length >> 8) & 0xFF);
  result[3] = static_cast<uint8_t>(length & 0xFF);

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::PIECE);

  // Set index (4 bytes, big-endian)
  result[5] = static_cast<uint8_t>((index_ >> 24) & 0xFF);
  result[6] = static_cast<uint8_t>((index_ >> 16) & 0xFF);
  result[7] = static_cast<uint8_t>((index_ >> 8) & 0xFF);
  result[8] = static_cast<uint8_t>(index_ & 0xFF);

  // Set begin (4 bytes, big-endian)
  result[9] = static_cast<uint8_t>((begin_ >> 24) & 0xFF);
  result[10] = static_cast<uint8_t>((begin_ >> 16) & 0xFF);
  result[11] = static_cast<uint8_t>((begin_ >> 8) & 0xFF);
  result[12] = static_cast<uint8_t>(begin_ & 0xFF);

  // Set block
  std::copy(block_.begin(), block_.end(), result.begin() + 13);

  return result;
}

std::string PieceMessage::to_string() const {
  std::ostringstream oss;
  oss << "PieceMessage[index=" << index_ << ", begin=" << begin_
      << ", block_size=" << block_.size() << "]";
  return oss.str();
}

// CancelMessage implementation

CancelMessage::CancelMessage(uint32_t index, uint32_t begin, uint32_t length)
    : PeerMessage(PeerMessageType::CANCEL), index_(index), begin_(begin),
      length_(length) {}

uint32_t CancelMessage::index() const { return index_; }

uint32_t CancelMessage::begin() const { return begin_; }

uint32_t CancelMessage::length() const { return length_; }

std::vector<uint8_t> CancelMessage::serialize() const {
  // Cancel message format: <len=0013><id=8><index><begin><length>
  std::vector<uint8_t> result(17, 0);

  // Set length (4 bytes, big-endian)
  result[3] = 13; // Length is 13 bytes (1 byte ID + 4 bytes index + 4 bytes
                  // begin + 4 bytes length)

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::CANCEL);

  // Set index (4 bytes, big-endian)
  result[5] = static_cast<uint8_t>((index_ >> 24) & 0xFF);
  result[6] = static_cast<uint8_t>((index_ >> 16) & 0xFF);
  result[7] = static_cast<uint8_t>((index_ >> 8) & 0xFF);
  result[8] = static_cast<uint8_t>(index_ & 0xFF);

  // Set begin (4 bytes, big-endian)
  result[9] = static_cast<uint8_t>((begin_ >> 24) & 0xFF);
  result[10] = static_cast<uint8_t>((begin_ >> 16) & 0xFF);
  result[11] = static_cast<uint8_t>((begin_ >> 8) & 0xFF);
  result[12] = static_cast<uint8_t>(begin_ & 0xFF);

  // Set length (4 bytes, big-endian)
  result[13] = static_cast<uint8_t>((length_ >> 24) & 0xFF);
  result[14] = static_cast<uint8_t>((length_ >> 16) & 0xFF);
  result[15] = static_cast<uint8_t>((length_ >> 8) & 0xFF);
  result[16] = static_cast<uint8_t>(length_ & 0xFF);

  return result;
}

std::string CancelMessage::to_string() const {
  std::ostringstream oss;
  oss << "CancelMessage[index=" << index_ << ", begin=" << begin_
      << ", length=" << length_ << "]";
  return oss.str();
}

// PortMessage implementation

PortMessage::PortMessage(uint16_t port)
    : PeerMessage(PeerMessageType::PORT), port_(port) {}

uint16_t PortMessage::port() const { return port_; }

std::vector<uint8_t> PortMessage::serialize() const {
  // Port message format: <len=0003><id=9><port>
  std::vector<uint8_t> result(7, 0);

  // Set length (4 bytes, big-endian)
  result[3] = 3; // Length is 3 bytes (1 byte ID + 2 bytes port)

  // Set message ID
  result[4] = static_cast<uint8_t>(PeerMessageType::PORT);

  // Set port (2 bytes, big-endian)
  result[5] = static_cast<uint8_t>((port_ >> 8) & 0xFF);
  result[6] = static_cast<uint8_t>(port_ & 0xFF);

  return result;
}

std::string PortMessage::to_string() const {
  std::ostringstream oss;
  oss << "PortMessage[port=" << port_ << "]";
  return oss.str();
}

// Factory methods

std::shared_ptr<ChokeMessage> PeerMessageFactory::create_choke() {
  return std::make_shared<ChokeMessage>();
}

std::shared_ptr<UnchokeMessage> PeerMessageFactory::create_unchoke() {
  return std::make_shared<UnchokeMessage>();
}

std::shared_ptr<InterestedMessage> PeerMessageFactory::create_interested() {
  return std::make_shared<InterestedMessage>();
}

std::shared_ptr<NotInterestedMessage>
PeerMessageFactory::create_not_interested() {
  return std::make_shared<NotInterestedMessage>();
}

std::shared_ptr<HaveMessage>
PeerMessageFactory::create_have(uint32_t piece_index) {
  return std::make_shared<HaveMessage>(piece_index);
}

std::shared_ptr<BitfieldMessage>
PeerMessageFactory::create_bitfield(const std::vector<uint8_t> &bitfield) {
  return std::make_shared<BitfieldMessage>(bitfield);
}

std::shared_ptr<RequestMessage>
PeerMessageFactory::create_request(uint32_t index, uint32_t begin,
                                   uint32_t length) {
  return std::make_shared<RequestMessage>(index, begin, length);
}

std::shared_ptr<PieceMessage>
PeerMessageFactory::create_piece(uint32_t index, uint32_t begin,
                                 const std::vector<uint8_t> &block) {
  return std::make_shared<PieceMessage>(index, begin, block);
}

std::shared_ptr<CancelMessage>
PeerMessageFactory::create_cancel(uint32_t index, uint32_t begin,
                                  uint32_t length) {
  return std::make_shared<CancelMessage>(index, begin, length);
}

std::shared_ptr<PortMessage> PeerMessageFactory::create_port(uint16_t port) {
  return std::make_shared<PortMessage>(port);
}

} // namespace bitscrape::bittorrent
