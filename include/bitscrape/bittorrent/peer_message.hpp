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

/**
 * @brief Choke message
 *
 * The choke message is sent to inform the peer that they are being choked.
 * When a peer is choked, they will not receive any requested pieces.
 */
class ChokeMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     */
    ChokeMessage();

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

/**
 * @brief Unchoke message
 *
 * The unchoke message is sent to inform the peer that they are no longer being choked.
 * When a peer is unchoked, they can request pieces.
 */
class UnchokeMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     */
    UnchokeMessage();

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

/**
 * @brief Interested message
 *
 * The interested message is sent to inform the peer that we are interested in their pieces.
 */
class InterestedMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     */
    InterestedMessage();

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

/**
 * @brief Not interested message
 *
 * The not interested message is sent to inform the peer that we are not interested in their pieces.
 */
class NotInterestedMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     */
    NotInterestedMessage();

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

/**
 * @brief Have message
 *
 * The have message is sent to inform the peer that we have a piece.
 */
class HaveMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     *
     * @param piece_index Index of the piece we have
     */
    explicit HaveMessage(uint32_t piece_index);

    /**
     * @brief Get the piece index
     *
     * @return Piece index
     */
    [[nodiscard]] uint32_t piece_index() const;

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
    uint32_t piece_index_; ///< Index of the piece we have
};

/**
 * @brief Bitfield message
 *
 * The bitfield message is sent to inform the peer which pieces we have.
 */
class BitfieldMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     *
     * @param bitfield Bitfield of pieces we have
     */
    explicit BitfieldMessage(const std::vector<uint8_t>& bitfield);

    /**
     * @brief Get the bitfield
     *
     * @return Bitfield
     */
    [[nodiscard]] const std::vector<uint8_t>& bitfield() const;

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
    std::vector<uint8_t> bitfield_; ///< Bitfield of pieces we have
};

/**
 * @brief Request message
 *
 * The request message is sent to request a block of a piece.
 */
class RequestMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     *
     * @param index Index of the piece
     * @param begin Offset within the piece
     * @param length Length of the block
     */
    RequestMessage(uint32_t index, uint32_t begin, uint32_t length);

    /**
     * @brief Get the piece index
     *
     * @return Piece index
     */
    [[nodiscard]] uint32_t index() const;

    /**
     * @brief Get the offset within the piece
     *
     * @return Offset
     */
    [[nodiscard]] uint32_t begin() const;

    /**
     * @brief Get the length of the block
     *
     * @return Length
     */
    [[nodiscard]] uint32_t length() const;

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
    uint32_t index_;  ///< Index of the piece
    uint32_t begin_;  ///< Offset within the piece
    uint32_t length_; ///< Length of the block
};

/**
 * @brief Piece message
 *
 * The piece message is sent to deliver a block of a piece.
 */
class PieceMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     *
     * @param index Index of the piece
     * @param begin Offset within the piece
     * @param block Block data
     */
    PieceMessage(uint32_t index, uint32_t begin, const std::vector<uint8_t>& block);

    /**
     * @brief Get the piece index
     *
     * @return Piece index
     */
    [[nodiscard]] uint32_t index() const;

    /**
     * @brief Get the offset within the piece
     *
     * @return Offset
     */
    [[nodiscard]] uint32_t begin() const;

    /**
     * @brief Get the block data
     *
     * @return Block data
     */
    [[nodiscard]] const std::vector<uint8_t>& block() const;

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
    uint32_t index_;                ///< Index of the piece
    uint32_t begin_;                ///< Offset within the piece
    std::vector<uint8_t> block_;    ///< Block data
};

/**
 * @brief Cancel message
 *
 * The cancel message is sent to cancel a previously requested block.
 */
class CancelMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     *
     * @param index Index of the piece
     * @param begin Offset within the piece
     * @param length Length of the block
     */
    CancelMessage(uint32_t index, uint32_t begin, uint32_t length);

    /**
     * @brief Get the piece index
     *
     * @return Piece index
     */
    [[nodiscard]] uint32_t index() const;

    /**
     * @brief Get the offset within the piece
     *
     * @return Offset
     */
    [[nodiscard]] uint32_t begin() const;

    /**
     * @brief Get the length of the block
     *
     * @return Length
     */
    [[nodiscard]] uint32_t length() const;

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
    uint32_t index_;  ///< Index of the piece
    uint32_t begin_;  ///< Offset within the piece
    uint32_t length_; ///< Length of the block
};

/**
 * @brief Port message
 *
 * The port message is sent to inform the peer of the port we are listening on for DHT.
 */
class PortMessage : public PeerMessage {
public:
    /**
     * @brief Constructor
     *
     * @param port Port number
     */
    explicit PortMessage(uint16_t port);

    /**
     * @brief Get the port number
     *
     * @return Port number
     */
    [[nodiscard]] uint16_t port() const;

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
    uint16_t port_; ///< Port number
};

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

    /**
     * @brief Create a choke message
     *
     * @return Shared pointer to the created choke message
     */
    [[nodiscard]] static std::shared_ptr<ChokeMessage> create_choke();

    /**
     * @brief Create an unchoke message
     *
     * @return Shared pointer to the created unchoke message
     */
    [[nodiscard]] static std::shared_ptr<UnchokeMessage> create_unchoke();

    /**
     * @brief Create an interested message
     *
     * @return Shared pointer to the created interested message
     */
    [[nodiscard]] static std::shared_ptr<InterestedMessage> create_interested();

    /**
     * @brief Create a not interested message
     *
     * @return Shared pointer to the created not interested message
     */
    [[nodiscard]] static std::shared_ptr<NotInterestedMessage> create_not_interested();

    /**
     * @brief Create a have message
     *
     * @param piece_index Index of the piece we have
     * @return Shared pointer to the created have message
     */
    [[nodiscard]] static std::shared_ptr<HaveMessage> create_have(uint32_t piece_index);

    /**
     * @brief Create a bitfield message
     *
     * @param bitfield Bitfield of pieces we have
     * @return Shared pointer to the created bitfield message
     */
    [[nodiscard]] static std::shared_ptr<BitfieldMessage> create_bitfield(const std::vector<uint8_t>& bitfield);

    /**
     * @brief Create a request message
     *
     * @param index Index of the piece
     * @param begin Offset within the piece
     * @param length Length of the block
     * @return Shared pointer to the created request message
     */
    [[nodiscard]] static std::shared_ptr<RequestMessage> create_request(uint32_t index, uint32_t begin, uint32_t length);

    /**
     * @brief Create a piece message
     *
     * @param index Index of the piece
     * @param begin Offset within the piece
     * @param block Block data
     * @return Shared pointer to the created piece message
     */
    [[nodiscard]] static std::shared_ptr<PieceMessage> create_piece(uint32_t index, uint32_t begin, const std::vector<uint8_t>& block);

    /**
     * @brief Create a cancel message
     *
     * @param index Index of the piece
     * @param begin Offset within the piece
     * @param length Length of the block
     * @return Shared pointer to the created cancel message
     */
    [[nodiscard]] static std::shared_ptr<CancelMessage> create_cancel(uint32_t index, uint32_t begin, uint32_t length);

    /**
     * @brief Create a port message
     *
     * @param port Port number
     * @return Shared pointer to the created port message
     */
    [[nodiscard]] static std::shared_ptr<PortMessage> create_port(uint16_t port);
};

} // namespace bitscrape::bittorrent
