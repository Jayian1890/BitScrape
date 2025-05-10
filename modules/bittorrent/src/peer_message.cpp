#include "bitscrape/bittorrent/peer_message.hpp"
#include "bitscrape/bittorrent/extended_message.hpp"
#include "bitscrape/bencode/bencode_decoder.hpp"

#include <sstream>
#include <iomanip>

namespace bitscrape::bittorrent {

// PeerMessage implementation

PeerMessage::PeerMessage(PeerMessageType type)
    : type_(type) {
}

PeerMessageType PeerMessage::type() const {
    return type_;
}

// HandshakeMessage implementation

HandshakeMessage::HandshakeMessage(const std::vector<uint8_t>& info_hash,
                                 const std::vector<uint8_t>& peer_id,
                                 const std::vector<uint8_t>& reserved)
    : PeerMessage(PeerMessageType::HANDSHAKE),
      info_hash_(info_hash),
      peer_id_(peer_id),
      reserved_(reserved) {
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

const std::vector<uint8_t>& HandshakeMessage::info_hash() const {
    return info_hash_;
}

const std::vector<uint8_t>& HandshakeMessage::peer_id() const {
    return peer_id_;
}

const std::vector<uint8_t>& HandshakeMessage::reserved() const {
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
    for (const auto& byte : info_hash_) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    oss << ", peer_id=";

    // Format peer_id as hex
    for (const auto& byte : peer_id_) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }

    oss << "]";

    return oss.str();
}

// KeepAliveMessage implementation

KeepAliveMessage::KeepAliveMessage()
    : PeerMessage(PeerMessageType::KEEP_ALIVE) {
}

std::vector<uint8_t> KeepAliveMessage::serialize() const {
    // Keep-alive message is just a length prefix with a value of 0
    std::vector<uint8_t> result(4, 0);
    return result;
}

std::string KeepAliveMessage::to_string() const {
    return "KeepAliveMessage[]";
}

// PeerMessageFactory implementation

std::shared_ptr<PeerMessage> PeerMessageFactory::create_from_data(const std::vector<uint8_t>& data) {
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
        } catch (const std::exception&) {
            // Failed to decode payload
            return nullptr;
        }
    }

    if (type == PeerMessageType::CHOKE ||
        type == PeerMessageType::UNCHOKE ||
        type == PeerMessageType::INTERESTED ||
        type == PeerMessageType::NOT_INTERESTED ||
        type == PeerMessageType::HAVE ||
        type == PeerMessageType::BITFIELD ||
        type == PeerMessageType::REQUEST ||
        type == PeerMessageType::PIECE ||
        type == PeerMessageType::CANCEL ||
        type == PeerMessageType::PORT) {
        // TODO: Implement other message types
        return nullptr;
    }

    // Unknown message type
    return nullptr;
}

std::shared_ptr<HandshakeMessage> PeerMessageFactory::create_handshake(
    const std::vector<uint8_t>& info_hash,
    const std::vector<uint8_t>& peer_id,
    const std::vector<uint8_t>& reserved) {
    return std::make_shared<HandshakeMessage>(info_hash, peer_id, reserved);
}

std::shared_ptr<KeepAliveMessage> PeerMessageFactory::create_keep_alive() {
    return std::make_shared<KeepAliveMessage>();
}

} // namespace bitscrape::bittorrent
