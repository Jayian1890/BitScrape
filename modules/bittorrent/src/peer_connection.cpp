#include "bitscrape/bittorrent/peer_connection.hpp"

#include <future>

namespace bitscrape::bittorrent {

PeerConnection::PeerConnection(network::Address address,
                             types::InfoHash info_hash,
                             std::vector<uint8_t> peer_id)
    : address_(std::move(address)),
      info_hash_(std::move(info_hash)),
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

    // TODO: Implement message receiving
    return nullptr;
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

    // TODO: Parse handshake response and validate

    return true;
}

void PeerConnection::process_message(const PeerMessage& /* message */) {
    // TODO: Implement message processing
}

} // namespace bitscrape::bittorrent
