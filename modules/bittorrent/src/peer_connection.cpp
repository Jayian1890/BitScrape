#include "bitscrape/bittorrent/peer_connection.hpp"

#include <future>

namespace bitscrape::bittorrent {

PeerConnection::PeerConnection(network::Address address,
                             types::InfoHash info_hash,
                             std::vector<uint8_t> peer_id)
    : address_(std::move(address)),
      info_hash_(info_hash),
      peer_id_(std::move(peer_id)),
      socket_(std::make_unique<network::TCPSocket>()),
      state_(State::DISCONNECTED),
      peer_choked_(true),
      peer_interested_(false),
      am_choked_(true),
      am_interested_(false) {
    // Ensure peer_id is 20 bytes
    if (peer_id_.size() != 20) {
        throw std::invalid_argument("Peer ID must be 20 bytes");
    }
}

PeerConnection::~PeerConnection() {
    disconnect();
}

bool PeerConnection::connect() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already connected
    if (state_ != State::DISCONNECTED) {
        return state_ == State::CONNECTED;
    }

    // Update state
    state_ = State::CONNECTING;

    // Connect to peer
    if (!socket_->connect(address_)) {
        state_ = State::DISCONNECTED;
        return false;
    }

    // Perform handshake
    state_ = State::HANDSHAKING;
    if (!handshake()) {
        socket_->close();
        state_ = State::DISCONNECTED;
        return false;
    }

    // Update state
    state_ = State::CONNECTED;

    return true;
}

std::future<bool> PeerConnection::connect_async() {
    return std::async(std::launch::async, [this]() {
        return this->connect();
    });
}

void PeerConnection::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already disconnected
    if (state_ == State::DISCONNECTED) {
        return;
    }

    // Update state
    state_ = State::DISCONNECTING;

    // Close socket
    socket_->close();

    // Update state
    state_ = State::DISCONNECTED;

    // Reset state
    peer_choked_ = true;
    peer_interested_ = false;
    am_choked_ = true;
    am_interested_ = false;
    remote_peer_id_.clear();
}

bool PeerConnection::send_message(const PeerMessage& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if connected
    if (state_ != State::CONNECTED) {
        return false;
    }

    // Serialize message
    auto data = message.serialize();

    // Send message
    return socket_->send(data.data(), data.size()) == static_cast<int>(data.size());
}

std::future<bool> PeerConnection::send_message_async(const PeerMessage& message) {
    // Create a copy of the message data
    auto data = message.serialize();

    return std::async(std::launch::async, [this, data = std::move(data)]() {
        // Create a new message from the serialized data
        // TODO: Implement message deserialization
        // For now, just send the original message
        return socket_->send(data.data(), data.size()) == static_cast<int>(data.size());
    });
}

std::shared_ptr<PeerMessage> PeerConnection::receive_message() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if connected
    if (state_ != State::CONNECTED) {
        return nullptr;
    }

    // First read the message length (4 bytes)
    uint32_t length = 0;
    if (socket_->receive(reinterpret_cast<uint8_t*>(&length), sizeof(length)) != sizeof(length)) {
        return nullptr;
    }

    // Convert from network byte order to host byte order
    length = ntohl(length);

    // If length is 0, it's a keep-alive message
    if (length == 0) {
        return PeerMessageFactory::create_keep_alive();
    }

    // Read the message type (1 byte)
    uint8_t type = 0;
    if (socket_->receive(&type, sizeof(type)) != sizeof(type)) {
        return nullptr;
    }

    // Read the message payload
    std::vector<uint8_t> payload(length - 1);  // -1 for the type byte
    if (!payload.empty()) {
        if (socket_->receive(payload.data(), payload.size()) != static_cast<int>(payload.size())) {
            return nullptr;
        }
    }

    // Create the message based on the type
    std::vector<uint8_t> message_data;
    message_data.push_back(type);
    message_data.insert(message_data.end(), payload.begin(), payload.end());

    return PeerMessageFactory::create_from_data(message_data);
}

std::future<std::shared_ptr<PeerMessage>> PeerConnection::receive_message_async() {
    return std::async(std::launch::async, [this]() {
        return this->receive_message();
    });
}

PeerConnection::State PeerConnection::state() const {
    return state_;
}

const network::Address& PeerConnection::address() const {
    return address_;
}

const types::InfoHash& PeerConnection::info_hash() const {
    return info_hash_;
}

const std::vector<uint8_t>& PeerConnection::peer_id() const {
    return peer_id_;
}

const std::vector<uint8_t>& PeerConnection::remote_peer_id() const {
    return remote_peer_id_;
}

bool PeerConnection::is_choked() const {
    return peer_choked_;
}

bool PeerConnection::is_interested() const {
    return peer_interested_;
}

bool PeerConnection::am_choked() const {
    return am_choked_;
}

bool PeerConnection::am_interested() const {
    return am_interested_;
}

int PeerConnection::send_raw_data(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if connected
    if (state_ != State::CONNECTED) {
        return -1;
    }

    // Send data
    return socket_->send(data, size);
}

bool PeerConnection::handshake() {
    // Create handshake message
    // Convert InfoHash bytes to vector
    std::vector<uint8_t> info_hash_bytes(info_hash_.bytes().begin(), info_hash_.bytes().end());
    HandshakeMessage handshake(info_hash_bytes, peer_id_);

    // Serialize handshake
    auto data = handshake.serialize();

    // Send handshake
    if (socket_->send(data.data(), data.size()) != static_cast<int>(data.size())) {
        return false;
    }

    // Receive handshake response
    std::vector<uint8_t> response(68);
    if (socket_->receive(response.data(), response.size()) != 68) {
        return false;
    }

    // Parse handshake response
    // Check protocol string length
    if (response[0] != 19) {
        return false;
    }

    // Check protocol string
    std::string protocol(response.begin() + 1, response.begin() + 20);
    if (protocol != "BitTorrent protocol") {
        return false;
    }

    // Extract reserved bytes (for extensions)
    std::vector<uint8_t> reserved(response.begin() + 20, response.begin() + 28);

    // Extract info hash
    std::vector<uint8_t> received_info_hash(response.begin() + 28, response.begin() + 48);
    std::vector<uint8_t> expected_info_hash(info_hash_.bytes().begin(), info_hash_.bytes().end());
    if (received_info_hash != expected_info_hash) {
        return false;
    }

    // Extract peer ID
    remote_peer_id_.assign(response.begin() + 48, response.begin() + 68);

    return true;
}

void PeerConnection::process_message(const PeerMessage& message) {
    switch (message.type()) {
        case PeerMessageType::CHOKE:
            am_choked_ = true;
            break;

        case PeerMessageType::UNCHOKE:
            am_choked_ = false;
            break;

        case PeerMessageType::INTERESTED:
            peer_interested_ = true;
            break;

        case PeerMessageType::NOT_INTERESTED:
            peer_interested_ = false;
            break;

        case PeerMessageType::HAVE:
            // Handle have message (update bitfield)
            break;

        case PeerMessageType::BITFIELD:
            // Handle bitfield message (store peer's bitfield)
            break;

        case PeerMessageType::REQUEST:
            // Handle request message (for piece data)
            break;

        case PeerMessageType::PIECE:
            // Handle piece message (received piece data)
            break;

        case PeerMessageType::CANCEL:
            // Handle cancel message (cancel pending request)
            break;

        case PeerMessageType::PORT:
            // Handle port message (DHT port)
            break;

        case PeerMessageType::EXTENDED:
            // Handle extended message (BEP 10)
            break;

        default:
            // Unknown message type
            break;
    }
}

} // namespace bitscrape::bittorrent
