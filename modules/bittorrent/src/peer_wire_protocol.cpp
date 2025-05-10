#include "bitscrape/bittorrent/peer_wire_protocol.hpp"

#include <future>
#include <thread>
#include <chrono>
#include <algorithm>

namespace bitscrape::bittorrent {

PeerWireProtocol::PeerWireProtocol(const types::InfoHash& info_hash, const std::vector<uint8_t>& peer_id)
    : info_hash_(info_hash),
      peer_id_(peer_id) {
    // Ensure peer_id is 20 bytes
    if (peer_id_.size() != 20) {
        throw std::invalid_argument("Peer ID must be 20 bytes");
    }
}

PeerWireProtocol::~PeerWireProtocol() {
    disconnect_all_peers();
}

bool PeerWireProtocol::connect_to_peer(const network::Address& address) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    // Check if already connected
    std::string address_str = address.to_string();
    auto it = connections_.find(address_str);
    if (it != connections_.end()) {
        return it->second->state() == PeerConnection::State::CONNECTED;
    }

    // Create connection
    auto connection = std::make_shared<PeerConnection>(address, info_hash_, peer_id_);

    // Connect
    bool success = connection->connect();

    // Store connection if successful
    if (success) {
        connections_[address_str] = connection;

        // Start receive loop
        start_receive_loop(connection);
    }

    return success;
}

std::future<bool> PeerWireProtocol::connect_to_peer_async(const network::Address& address) {
    return std::async(std::launch::async, [this, address]() {
        return this->connect_to_peer(address);
    });
}

void PeerWireProtocol::disconnect_from_peer(const network::Address& address) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    // Find connection
    std::string address_str = address.to_string();
    auto it = connections_.find(address_str);
    if (it != connections_.end()) {
        // Disconnect
        it->second->disconnect();

        // Remove connection
        connections_.erase(it);
    }
}

void PeerWireProtocol::disconnect_all_peers() {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    // Disconnect all peers
    for (auto& pair : connections_) {
        pair.second->disconnect();
    }

    // Clear connections
    connections_.clear();
}

bool PeerWireProtocol::send_message(const network::Address& address, const PeerMessage& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    // Find connection
    std::string address_str = address.to_string();
    auto it = connections_.find(address_str);
    if (it == connections_.end()) {
        return false;
    }

    // Send message
    return it->second->send_message(message);
}

std::future<bool> PeerWireProtocol::send_message_async(const network::Address& address, const PeerMessage& message) {
    return std::async(std::launch::async, [this, address, &message]() {
        return this->send_message(address, message);
    });
}

bool PeerWireProtocol::send_raw_data(const network::Address& address, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    // Find connection
    std::string address_str = address.to_string();
    auto it = connections_.find(address_str);
    if (it == connections_.end()) {
        return false;
    }

    // Send data
    return it->second->send_raw_data(data.data(), data.size()) == static_cast<int>(data.size());
}

void PeerWireProtocol::register_message_handler(PeerMessageType type,
                                              std::function<void(const network::Address&, const PeerMessage&)> handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);

    // Register handler
    message_handlers_[type] = handler;
}

const types::InfoHash& PeerWireProtocol::info_hash() const {
    return info_hash_;
}

const std::vector<uint8_t>& PeerWireProtocol::peer_id() const {
    return peer_id_;
}

std::vector<network::Address> PeerWireProtocol::connected_peers() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    // Collect connected peers
    std::vector<network::Address> result;
    for (const auto& pair : connections_) {
        if (pair.second->state() == PeerConnection::State::CONNECTED) {
            result.push_back(pair.second->address());
        }
    }

    return result;
}

bool PeerWireProtocol::is_peer_connected(const network::Address& address) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    // Find connection
    std::string address_str = address.to_string();
    auto it = connections_.find(address_str);
    if (it == connections_.end()) {
        return false;
    }

    // Check if connected
    return it->second->state() == PeerConnection::State::CONNECTED;
}

void PeerWireProtocol::process_message(const network::Address& address, const PeerMessage& message) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);

    // Find handler
    auto it = message_handlers_.find(message.type());
    if (it != message_handlers_.end()) {
        // Call handler
        it->second(address, message);
    }
}

void PeerWireProtocol::start_receive_loop(std::shared_ptr<PeerConnection> connection) {
    // Start receive loop in a new thread
    std::thread([this, connection]() {
        while (connection->state() == PeerConnection::State::CONNECTED) {
            // Receive message
            auto message = connection->receive_message();
            if (message) {
                // Process message
                process_message(connection->address(), *message);
            } else {
                // Sleep to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }).detach();
}

} // namespace bitscrape::bittorrent
