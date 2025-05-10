#include "bitscrape/bittorrent/metadata_exchange.hpp"
#include "bitscrape/bittorrent/extended_message.hpp"
#include "bitscrape/bencode/bencode_encoder.hpp"
#include "bitscrape/bencode/bencode_decoder.hpp"

#include <future>

namespace bitscrape::bittorrent {

MetadataExchange::MetadataExchange(PeerWireProtocol& protocol)
    : protocol_(protocol),
      ut_metadata_id_(0) {
}

MetadataExchange::~MetadataExchange() = default;

void MetadataExchange::initialize() {
    // Register message handlers for extended messages
    protocol_.register_message_handler(PeerMessageType::EXTENDED,
        [this](const network::Address& address, const PeerMessage& message) {
            // Cast to ExtendedMessage
            const auto* extended_message = dynamic_cast<const ExtendedMessage*>(&message);
            if (!extended_message) {
                return;
            }

            // Handle based on extended message type
            if (extended_message->extended_type() == 0) {
                // Extended handshake
                handle_extended_handshake(address, extended_message->payload());
            } else if (extended_message->extended_type() == ut_metadata_id_) {
                // Metadata message
                handle_metadata_message(address, extended_message->payload());
            }
        });
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
    metadata_received_callback_ = std::move(callback);
}

void MetadataExchange::handle_extended_handshake(const network::Address& address, const bencode::BencodeValue& message) {
    // Check if the message is a dictionary
    if (!message.is_dict()) {
        return;
    }

    // Get the m dictionary (extension IDs)
    const bencode::BencodeValue* m_dict = message.get("m");
    if (m_dict == nullptr || !m_dict->is_dict()) {
        return;
    }

    // Get the ut_metadata extension ID
    const bencode::BencodeValue* ut_metadata = m_dict->get("ut_metadata");
    if (ut_metadata == nullptr || !ut_metadata->is_integer()) {
        return;
    }

    // Store the ut_metadata extension ID
    ut_metadata_id_ = static_cast<int>(ut_metadata->as_integer());

    // Get the metadata_size
    const bencode::BencodeValue* metadata_size = message.get("metadata_size");
    if (metadata_size != nullptr && metadata_size->is_integer()) {
        // Store the metadata size for this peer
        std::string address_str = address.to_string();
        peer_metadata_size_[address_str] = static_cast<int>(metadata_size->as_integer());

        // Request metadata pieces
        int size = peer_metadata_size_[address_str];
        int num_pieces = (size + 16383) / 16384; // 16 KiB pieces

        for (int i = 0; i < num_pieces; ++i) {
            send_metadata_request(address, i);
        }
    }
}

void MetadataExchange::handle_metadata_message(const network::Address& address, const bencode::BencodeValue& message) {
    // Check if the message is a dictionary
    if (!message.is_dict()) {
        return;
    }

    // Get the msg_type
    const bencode::BencodeValue* msg_type = message.get("msg_type");
    if (msg_type == nullptr || !msg_type->is_integer()) {
        return;
    }

    int type = static_cast<int>(msg_type->as_integer());

    // Get the piece
    const bencode::BencodeValue* piece = message.get("piece");
    if (piece == nullptr || !piece->is_integer()) {
        return;
    }

    int piece_index = static_cast<int>(piece->as_integer());

    // Handle message based on type
    switch (type) {
        case 0: // request
            {
                // Get the total_size from our metadata
                int total_size = 0;
                {
                    std::lock_guard<std::mutex> lock(metadata_mutex_);
                    if (metadata_) {
                        total_size = static_cast<int>(metadata_->raw_data().size());
                    }
                }

                // If we have metadata, send the requested piece
                if (total_size > 0) {
                    // Calculate piece size
                    int piece_size = 16384;
                    if (piece_index == (total_size - 1) / 16384) {
                        piece_size = total_size % 16384;
                        if (piece_size == 0) {
                            piece_size = 16384;
                        }
                    }

                    // Get piece data
                    std::vector<uint8_t> piece_data;
                    {
                        std::lock_guard<std::mutex> lock(metadata_mutex_);
                        if (metadata_) {
                            const auto& data = metadata_->raw_data();
                            int offset = piece_index * 16384;
                            piece_data.assign(data.begin() + offset, data.begin() + offset + piece_size);
                        }
                    }

                    // Send piece data
                    if (!piece_data.empty()) {
                        send_metadata_data(address, piece_index, total_size, piece_data);
                    } else {
                        send_metadata_reject(address, piece_index);
                    }
                } else {
                    // We don't have the metadata
                    send_metadata_reject(address, piece_index);
                }
            }
            break;

        case 1: // data
            {
                // Get the total_size
                const bencode::BencodeValue* total_size = message.get("total_size");
                if (total_size == nullptr || !total_size->is_integer()) {
                    return;
                }

                int size = static_cast<int>(total_size->as_integer());

                // Store the metadata size for this peer
                std::string address_str = address.to_string();
                peer_metadata_size_[address_str] = size;

                // Get the piece data
                std::vector<uint8_t> piece_data;
                const bencode::BencodeValue* data = message.get("data");
                if (data != nullptr && data->is_string()) {
                    // The data is included in the dictionary
                    piece_data = std::vector<uint8_t>(data->as_string().begin(), data->as_string().end());
                } else {
                    // The data is appended to the bencoded dictionary
                    // We need to extract the data from the raw message
                    // This is a bit tricky because we don't have access to the raw message here
                    // For now, we'll just skip this case and rely on the data being included in the dictionary
                }

                // Store the piece
                if (!piece_data.empty()) {
                    std::lock_guard<std::mutex> lock(pieces_mutex_);
                    metadata_pieces_[piece_index] = piece_data;

                    // Try to process the metadata
                    process_metadata_pieces();
                }
            }
            break;

        case 2: // reject
            // The peer rejected our request
            // We could try another peer
            break;
    }
}

bool MetadataExchange::send_extended_handshake(const network::Address& address) {
    // Create the extended handshake message
    std::map<std::string, bencode::BencodeValue> handshake_dict;

    // Add the m dictionary (extension IDs)
    std::map<std::string, bencode::BencodeValue> m_dict;
    m_dict["ut_metadata"] = bencode::BencodeValue(static_cast<int64_t>(1)); // We use ID 1 for ut_metadata
    handshake_dict["m"] = bencode::BencodeValue(m_dict);

    // Add metadata_size if we have it
    {
        std::lock_guard<std::mutex> lock(metadata_mutex_);
        if (metadata_) {
            handshake_dict["metadata_size"] = bencode::BencodeValue(static_cast<int64_t>(metadata_->raw_data().size()));
        }
    }

    // Create the BencodeValue
    bencode::BencodeValue handshake(handshake_dict);

    // Encode the message
    auto encoder = bencode::create_bencode_encoder();
    std::vector<uint8_t> data = encoder->encode(handshake);

    // Create the extended message
    auto extended_message = std::make_shared<ExtendedMessage>(0, handshake); // 0 = handshake

    // Send the message
    return protocol_.send_message(address, *extended_message);
}

bool MetadataExchange::send_metadata_request(const network::Address& address, int piece) {
    // Check if ut_metadata is supported
    if (ut_metadata_id_ == 0) {
        return false;
    }

    // Create the metadata request message
    std::map<std::string, bencode::BencodeValue> request_dict;
    request_dict["msg_type"] = bencode::BencodeValue(static_cast<int64_t>(0)); // 0 = request
    request_dict["piece"] = bencode::BencodeValue(static_cast<int64_t>(piece));

    // Create the BencodeValue
    bencode::BencodeValue request(request_dict);

    // Encode the message
    auto encoder = bencode::create_bencode_encoder();
    std::vector<uint8_t> data = encoder->encode(request);

    // Create the extended message
    auto extended_message = std::make_shared<ExtendedMessage>(ut_metadata_id_, request);

    // Send the message
    return protocol_.send_message(address, *extended_message);
}

bool MetadataExchange::send_metadata_data(const network::Address& address, int piece_index, int total_size, const std::vector<uint8_t>& piece_data) {
    // Check if ut_metadata is supported
    if (ut_metadata_id_ == 0) {
        return false;
    }

    // Create the metadata data message
    std::map<std::string, bencode::BencodeValue> message_dict;
    message_dict["msg_type"] = bencode::BencodeValue(static_cast<int64_t>(1)); // 1 = data
    message_dict["piece"] = bencode::BencodeValue(static_cast<int64_t>(piece_index));
    message_dict["total_size"] = bencode::BencodeValue(static_cast<int64_t>(total_size));

    // Create the BencodeValue
    bencode::BencodeValue message(message_dict);

    // Encode the message
    auto encoder = bencode::create_bencode_encoder();
    std::vector<uint8_t> encoded_message = encoder->encode(message);

    // Append the piece data
    encoded_message.insert(encoded_message.end(), piece_data.begin(), piece_data.end());

    // Create the extended message with the encoded message and piece data
    // We can't use the normal ExtendedMessage constructor because we need to append the piece data
    // to the bencoded dictionary, not include it in the dictionary

    // First, create a message with just the bencoded dictionary
    auto extended_message = std::make_shared<ExtendedMessage>(ut_metadata_id_, message);

    // Then, manually serialize and modify it to include the piece data
    std::vector<uint8_t> serialized = extended_message->serialize();

    // Remove the length prefix (first 4 bytes)
    std::vector<uint8_t> message_data(serialized.begin() + 4, serialized.end());

    // Append the piece data
    message_data.insert(message_data.end(), piece_data.begin(), piece_data.end());

    // Calculate the new length
    auto length = static_cast<uint32_t>(message_data.size());

    // Create the final message with the correct length prefix
    std::vector<uint8_t> final_message;
    final_message.reserve(4 + length);

    // Add length prefix (big-endian)
    final_message.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
    final_message.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
    final_message.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
    final_message.push_back(static_cast<uint8_t>(length & 0xFF));

    // Add message data
    final_message.insert(final_message.end(), message_data.begin(), message_data.end());

    // Send the message
    return protocol_.send_raw_data(address, final_message);
}

bool MetadataExchange::send_metadata_reject(const network::Address& address, int piece) {
    // Check if ut_metadata is supported
    if (ut_metadata_id_ == 0) {
        return false;
    }

    // Create the metadata reject message
    std::map<std::string, bencode::BencodeValue> reject_dict;
    reject_dict["msg_type"] = bencode::BencodeValue(static_cast<int64_t>(2)); // 2 = reject
    reject_dict["piece"] = bencode::BencodeValue(static_cast<int64_t>(piece));

    // Create the BencodeValue
    bencode::BencodeValue reject(reject_dict);

    // Encode the message
    auto encoder = bencode::create_bencode_encoder();
    std::vector<uint8_t> data = encoder->encode(reject);

    // Create the extended message
    auto extended_message = std::make_shared<ExtendedMessage>(ut_metadata_id_, reject);

    // Send the message
    return protocol_.send_message(address, *extended_message);
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
            // Use the most common size or the largest size
            // For now, we'll just keep the first size we found
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
