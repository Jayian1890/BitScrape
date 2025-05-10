#include "bitscrape/bittorrent/metadata_exchange.hpp"

#include <future>
#include <thread>
#include <chrono>
#include <algorithm>

namespace bitscrape::bittorrent {

MetadataExchange::MetadataExchange(PeerWireProtocol& protocol)
    : protocol_(protocol),
      ut_metadata_id_(0) {
}

MetadataExchange::~MetadataExchange() {
}

void MetadataExchange::initialize() {
    // TODO: Register message handlers for extended messages
}

bool MetadataExchange::request_metadata(const network::Address& address) {
    // Check if peer is connected
    if (!protocol_.is_peer_connected(address)) {
        return false;
    }
    
    // Send extended handshake
    return send_extended_handshake(address);
}

std::future<bool> MetadataExchange::request_metadata_async(const network::Address& address) {
    return std::async(std::launch::async, [this, address]() {
        return this->request_metadata(address);
    });
}

std::shared_ptr<types::MetadataInfo> MetadataExchange::metadata() const {
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    return metadata_;
}

void MetadataExchange::set_metadata_received_callback(std::function<void(const types::MetadataInfo&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    metadata_received_callback_ = callback;
}

void MetadataExchange::handle_extended_handshake(const network::Address& address, const bencode::BencodeValue& message) {
    // TODO: Implement extended handshake handling
}

void MetadataExchange::handle_metadata_message(const network::Address& address, const bencode::BencodeValue& message) {
    // TODO: Implement metadata message handling
}

bool MetadataExchange::send_extended_handshake(const network::Address& address) {
    // TODO: Implement extended handshake sending
    return false;
}

bool MetadataExchange::send_metadata_request(const network::Address& address, int piece) {
    // TODO: Implement metadata request sending
    return false;
}

bool MetadataExchange::send_metadata_data(const network::Address& address, int piece, int total_size, const std::vector<uint8_t>& data) {
    // TODO: Implement metadata data sending
    return false;
}

bool MetadataExchange::send_metadata_reject(const network::Address& address, int piece) {
    // TODO: Implement metadata reject sending
    return false;
}

bool MetadataExchange::process_metadata_pieces() {
    std::lock_guard<std::mutex> lock(pieces_mutex_);
    
    // Check if we have any pieces
    if (metadata_pieces_.empty()) {
        return false;
    }
    
    // Get metadata size
    int metadata_size = 0;
    for (const auto& pair : peer_metadata_size_) {
        if (metadata_size == 0) {
            metadata_size = pair.second;
        } else if (metadata_size != pair.second) {
            // Inconsistent metadata size reported by peers
            // TODO: Handle this case
        }
    }
    
    // Check if we have all pieces
    int num_pieces = (metadata_size + 16383) / 16384; // 16 KiB pieces
    for (int i = 0; i < num_pieces; ++i) {
        if (metadata_pieces_.find(i) == metadata_pieces_.end()) {
            return false;
        }
    }
    
    // Combine pieces
    std::vector<uint8_t> metadata_data;
    metadata_data.reserve(metadata_size);
    
    for (int i = 0; i < num_pieces; ++i) {
        const auto& piece = metadata_pieces_[i];
        metadata_data.insert(metadata_data.end(), piece.begin(), piece.end());
    }
    
    // Truncate to metadata size
    if (metadata_data.size() > static_cast<size_t>(metadata_size)) {
        metadata_data.resize(metadata_size);
    }
    
    // Create metadata info
    auto metadata = std::make_shared<types::MetadataInfo>();
    if (!metadata->parse(metadata_data)) {
        return false;
    }
    
    // Store metadata
    {
        std::lock_guard<std::mutex> metadata_lock(metadata_mutex_);
        metadata_ = metadata;
    }
    
    // Call callback
    {
        std::lock_guard<std::mutex> callback_lock(callback_mutex_);
        if (metadata_received_callback_) {
            metadata_received_callback_(*metadata);
        }
    }
    
    return true;
}

} // namespace bitscrape::bittorrent
