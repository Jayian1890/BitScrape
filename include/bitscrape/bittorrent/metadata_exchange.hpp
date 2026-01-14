#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "bitscrape/bencode/bencode_value.hpp"
#include "bitscrape/bittorrent/peer_wire_protocol.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/types/metadata_info.hpp"

namespace bitscrape::bittorrent {

/**
 * @brief Implements the BitTorrent metadata exchange protocol (BEP 9)
 *
 * This class provides functionality for exchanging metadata between peers
 * using the extension protocol defined in BEP 9.
 */
class MetadataExchange {
public:
  /**
   * @brief Constructor
   *
   * @param protocol Peer wire protocol instance
   */
  explicit MetadataExchange(PeerWireProtocol &protocol);

  /**
   * @brief Destructor
   */
  ~MetadataExchange();

  /**
   * @brief Initialize the metadata exchange protocol
   *
   * This method registers the necessary message handlers and initializes
   * the metadata exchange protocol.
   */
  void initialize();

  /**
   * @brief Request metadata from a peer
   *
   * @param address Peer address
   * @return True if the request was sent successfully, false otherwise
   */
  bool request_metadata(const network::Address &address);

  /**
   * @brief Request metadata from a peer asynchronously
   *
   * @param address Peer address
   * @return Future that will be completed when the metadata is received
   */
  std::future<bool> request_metadata_async(const network::Address &address);

  /**
   * @brief Get the metadata for the torrent
   *
   * @return Metadata info, or nullptr if not available
   */
  std::shared_ptr<types::MetadataInfo> metadata() const;

  /**
   * @brief Set a callback to be called when metadata is received
   *
   * @param callback Function to call when metadata is received
   */
  void set_metadata_received_callback(
      std::function<void(const types::MetadataInfo &)> callback);

private:
  /**
   * @brief Handle an extended handshake message
   *
   * @param address Peer address
   * @param message Extended handshake message
   */
  void handle_extended_handshake(const network::Address &address,
                                 const bencode::BencodeValue &message);

  /**
   * @brief Handle a metadata message
   *
   * @param address Peer address
   * @param message Metadata message
   */
  void handle_metadata_message(const network::Address &address,
                               const bencode::BencodeValue &message);

  /**
   * @brief Send an extended handshake message
   *
   * @param address Peer address
   * @return True if the message was sent successfully, false otherwise
   */
  bool send_extended_handshake(const network::Address &address);

  /**
   * @brief Send a metadata request message
   *
   * @param address Peer address
   * @param piece Metadata piece index
   * @return True if the message was sent successfully, false otherwise
   */
  bool send_metadata_request(const network::Address &address, int piece);

  /**
   * @brief Send a metadata data message
   *
   * @param address Peer address
   * @param piece_index Metadata piece index
   * @param total_size Total metadata size
   * @param piece_data Metadata piece data
   * @return True if the message was sent successfully, false otherwise
   */
  bool send_metadata_data(const network::Address &address, int piece_index,
                          int total_size,
                          const std::vector<uint8_t> &piece_data);

  /**
   * @brief Send a metadata reject message
   *
   * @param address Peer address
   * @param piece Metadata piece index
   * @return True if the message was sent successfully, false otherwise
   */
  bool send_metadata_reject(const network::Address &address, int piece);

  /**
   * @brief Process received metadata pieces
   *
   * @return True if all metadata pieces have been received, false otherwise
   */
  bool process_metadata_pieces();

  PeerWireProtocol &protocol_; ///< Peer wire protocol instance
  std::shared_ptr<types::MetadataInfo> metadata_; ///< Metadata info
  mutable std::mutex metadata_mutex_;             ///< Mutex for metadata access

  std::unordered_map<std::string, int>
      peer_metadata_size_; ///< Metadata size reported by peers
  std::unordered_map<int, std::vector<uint8_t>>
      metadata_pieces_;             ///< Received metadata pieces
  mutable std::mutex pieces_mutex_; ///< Mutex for pieces map

  std::function<void(const types::MetadataInfo &)>
      metadata_received_callback_;    ///< Callback for metadata received
  mutable std::mutex callback_mutex_; ///< Mutex for callback

  std::atomic<int> ut_metadata_id_; ///< ut_metadata extension ID
};

} // namespace bitscrape::bittorrent
