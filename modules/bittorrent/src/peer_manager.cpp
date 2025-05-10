#include "bitscrape/bittorrent/peer_manager.hpp"

#include <future>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>

namespace bitscrape::bittorrent {

PeerManager::PeerManager(const types::InfoHash& info_hash,
                       const std::vector<uint8_t>& peer_id,
                       int max_connections)
    : info_hash_(info_hash),
      peer_id_(peer_id),
      protocol_(std::make_unique<PeerWireProtocol>(info_hash, peer_id)),
      max_connections_(max_connections),
      running_(false) {
    // Ensure peer_id is 20 bytes
    if (peer_id_.size() != 20) {
        throw std::invalid_argument("Peer ID must be 20 bytes");
    }
}

PeerManager::~PeerManager() {
    stop();
}

bool PeerManager::start() {
    // Check if already running
    if (running_) {
        return true;
    }

    // Set running flag
    running_ = true;

    // Start connection thread
    connection_thread_ = std::thread([this]() {
        while (running_) {
            // Connect to peers
            connect_to_peers();

            // Manage connections
            manage_connections();

            // Sleep to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    return true;
}

std::future<bool> PeerManager::start_async() {
    return std::async(std::launch::async, [this]() {
        return this->start();
    });
}

void PeerManager::stop() {
    // Check if already stopped
    if (!running_) {
        return;
    }

    // Set running flag
    running_ = false;

    // Wait for connection thread to finish
    if (connection_thread_.joinable()) {
        connection_thread_.join();
    }

    // Disconnect all peers
    protocol_->disconnect_all_peers();
}

void PeerManager::add_peer(const network::Address& address) {
    std::lock_guard<std::mutex> lock(peers_mutex_);

    // Add peer to known peers
    std::string address_str = address.to_string() + ":" + std::to_string(address.port());
    known_peers_.insert(address_str);
}

void PeerManager::add_peers(const std::vector<network::Address>& addresses) {
    std::lock_guard<std::mutex> lock(peers_mutex_);

    // Add peers to known peers
    for (const auto& address : addresses) {
        std::string address_str = address.to_string() + ":" + std::to_string(address.port());
        known_peers_.insert(address_str);
    }
}

void PeerManager::remove_peer(const network::Address& address) {
    std::lock_guard<std::mutex> lock(peers_mutex_);

    // Remove peer from known peers
    std::string address_str = address.to_string() + ":" + std::to_string(address.port());
    known_peers_.erase(address_str);

    // Disconnect from peer
    protocol_->disconnect_from_peer(address);
}

std::vector<network::Address> PeerManager::known_peers() const {
    // Create a copy of the known peers set to avoid holding the lock while creating Address objects
    std::unordered_set<std::string> peers_copy;
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        peers_copy = known_peers_;
    }

    // Collect known peers
    std::vector<network::Address> result;
    for (const auto& address_str : peers_copy) {
        // Parse the address string (format: "ip:port")
        size_t colon_pos = address_str.find_last_of(':');
        if (colon_pos != std::string::npos) {
            std::string ip = address_str.substr(0, colon_pos);
            uint16_t port = static_cast<uint16_t>(std::stoi(address_str.substr(colon_pos + 1)));
            result.emplace_back(ip, port);
        }
    }

    return result;
}

std::vector<network::Address> PeerManager::connected_peers() const {
    return protocol_->connected_peers();
}

PeerWireProtocol& PeerManager::protocol() {
    return *protocol_;
}

const types::InfoHash& PeerManager::info_hash() const {
    return info_hash_;
}

const std::vector<uint8_t>& PeerManager::peer_id() const {
    return peer_id_;
}

int PeerManager::max_connections() const {
    return max_connections_;
}

void PeerManager::set_max_connections(int max_connections) {
    max_connections_ = max_connections;
}

void PeerManager::connect_to_peers() {
    // Get connected peers
    auto connected = protocol_->connected_peers();

    // Check if we need more connections
    if (connected.size() >= static_cast<size_t>(max_connections_)) {
        return;
    }

    // Get known peers
    auto known = known_peers();

    // Shuffle known peers
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::shuffle(known.begin(), known.end(), gen);

    // Connect to peers until we reach max_connections
    for (const auto& address : known) {
        // Check if already connected
        bool already_connected = false;
        for (const auto& connected_address : connected) {
            if (address.to_string() == connected_address.to_string()) {
                already_connected = true;
                break;
            }
        }

        if (!already_connected) {
            // Connect to peer
            protocol_->connect_to_peer_async(address);

            // Update connected peers
            connected = protocol_->connected_peers();

            // Check if we've reached max_connections
            if (connected.size() >= static_cast<size_t>(max_connections_)) {
                break;
            }
        }
    }
}

void PeerManager::manage_connections() {
    // Get connected peers
    auto connected = protocol_->connected_peers();

    // Check if we have too many connections
    if (connected.size() <= static_cast<size_t>(max_connections_)) {
        return;
    }

    // Sort peers by quality (for now, just use a random order)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::shuffle(connected.begin(), connected.end(), gen);

    // Disconnect excess peers
    for (auto i = static_cast<size_t>(max_connections_); i < connected.size(); ++i) {
        protocol_->disconnect_from_peer(connected[i]);
    }
}

} // namespace bitscrape::bittorrent
