#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace bitscrape::bittorrent {

/**
 * @brief Enumeration of BitTorrent peer message types
 */
enum class PeerMessageType : uint8_t {
    CHOKE = 0,
    UNCHOKE = 1,
    INTERESTED = 2,
    NOT_INTERESTED = 3,
    HAVE = 4,
    BITFIELD = 5,
    REQUEST = 6,
    PIECE = 7,
    CANCEL = 8,
    PORT = 9,
    EXTENDED = 20,  // BEP 10: Extension Protocol
    HANDSHAKE = 255, // Special type for handshake (not a standard message type)
    KEEP_ALIVE = 254 // Special type for keep-alive (not a standard message type)
};

/**
 * @brief Base class for all BitTorrent peer messages
 */
class PeerMessage {
public:
    /**
     * @brief Constructor
     *
     * @param type Message type
     */
    explicit PeerMessage(PeerMessageType type);

    /**
     * @brief Virtual destructor
     */
    virtual ~PeerMessage() = default;

    /**
     * @brief Get the message type
     *
     * @return Message type
     */
    [[nodiscard]] PeerMessageType type() const;

    /**
     * @brief Serialize the message to a byte vector
     *
     * @return Serialized message
     */
    [[nodiscard]] virtual std::vector<uint8_t> serialize() const = 0;

    /**
     * @brief Get a string representation of the message
     *
     * @return String representation
     */
    [[nodiscard]] virtual std::string to_string() const = 0;

private:
    PeerMessageType type_; ///< Message type
};

/**
 * @brief Handshake message
 *
 * The handshake is the first message sent by either side.
 * Format: <pstrlen><pstr><reserved><info_hash><peer_id>
 */
class HandshakeMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     *
     * @param info_hash 20-byte SHA1 hash of the info dictionary
     * @param peer_id 20-byte peer ID
     * @param reserved 8-byte reserved field for extensions
     */
    HandshakeMessage(const std::vector<uint8_t>& info_hash,
                    const std::vector<uint8_t>& peer_id,
                    const std::vector<uint8_t>& reserved = std::vector<uint8_t>(8, 0));

    /**
     * @brief Get the info hash
     *
     * @return Info hash
     */
    [[nodiscard]] const std::vector<uint8_t>& info_hash() const;

    /**
     * @brief Get the peer ID
     *
     * @return Peer ID
     */
    [[nodiscard]] const std::vector<uint8_t>& peer_id() const;

    /**
     * @brief Get the reserved field
     *
     * @return Reserved field
     */
    [[nodiscard]] const std::vector<uint8_t>& reserved() const;

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
    std::vector<uint8_t> info_hash_; ///< 20-byte SHA1 hash of the info dictionary
    std::vector<uint8_t> peer_id_;   ///< 20-byte peer ID
    std::vector<uint8_t> reserved_;  ///< 8-byte reserved field for extensions
};

/**
 * @brief Keep-alive message
 *
 * The keep-alive message is sent to maintain the connection when no other messages
 * are being sent. It has no payload and is simply a length prefix with a value of zero.
 */
class KeepAliveMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     */
    KeepAliveMessage();

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
};

// Forward declarations of other message types
class ChokeMessage;
class UnchokeMessage;
class InterestedMessage;
class NotInterestedMessage;
class HaveMessage;
class BitfieldMessage;
class RequestMessage;
class PieceMessage;
class CancelMessage;
class PortMessage;
class ExtendedMessage;

/**
 * @brief Factory for creating peer messages from serialized data
 */
class PeerMessageFactory {
public:
    /**
     * @brief Create a message from serialized data
     *
     * @param data Serialized message data
     * @return Shared pointer to the created message, or nullptr if parsing failed
     */
    [[nodiscard]] static std::shared_ptr<PeerMessage> create_from_data(const std::vector<uint8_t>& data);

    /**
     * @brief Create a handshake message
     *
     * @param info_hash 20-byte SHA1 hash of the info dictionary
     * @param peer_id 20-byte peer ID
     * @param reserved 8-byte reserved field for extensions
     * @return Shared pointer to the created handshake message
     */
    [[nodiscard]] static std::shared_ptr<HandshakeMessage> create_handshake(
        const std::vector<uint8_t>& info_hash,
        const std::vector<uint8_t>& peer_id,
        const std::vector<uint8_t>& reserved = std::vector<uint8_t>(8, 0));

    /**
     * @brief Create a keep-alive message
     *
     * @return Shared pointer to the created keep-alive message
     */
    [[nodiscard]] static std::shared_ptr<KeepAliveMessage> create_keep_alive();
};

} // namespace bitscrape::bittorrent
