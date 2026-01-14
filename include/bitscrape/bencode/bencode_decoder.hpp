#pragma once

#include "bitscrape/bencode/bencode_value.hpp"

#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace bitscrape::bencode {

/**
 * @brief Interface for bencode decoders
 *
 * BencodeDecoder is responsible for decoding bencode data to BencodeValue
 * objects.
 */
class BencodeDecoder {
public:
  /**
   * @brief Virtual destructor
   */
  virtual ~BencodeDecoder() = default;

  /**
   * @brief Decode bencode data to a BencodeValue
   *
   * @param data Bencode data as a byte vector
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  virtual BencodeValue decode(const std::vector<uint8_t> &data) = 0;

  /**
   * @brief Decode bencode data to a BencodeValue asynchronously
   *
   * @param data Bencode data as a byte vector
   * @return Future containing the decoded BencodeValue
   */
  virtual std::future<BencodeValue>
  decode_async(const std::vector<uint8_t> &data) = 0;

  /**
   * @brief Decode bencode data to a BencodeValue
   *
   * @param data Bencode data as a string
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  virtual BencodeValue decode(const std::string &data) = 0;

  /**
   * @brief Decode bencode data to a BencodeValue asynchronously
   *
   * @param data Bencode data as a string
   * @return Future containing the decoded BencodeValue
   */
  virtual std::future<BencodeValue> decode_async(const std::string &data) = 0;

  /**
   * @brief Decode bencode data to a BencodeValue
   *
   * @param data Bencode data as a string view
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  virtual BencodeValue decode(std::string_view data) = 0;

  /**
   * @brief Decode bencode data to a BencodeValue asynchronously
   *
   * @param data Bencode data as a string view
   * @return Future containing the decoded BencodeValue
   */
  virtual std::future<BencodeValue> decode_async(std::string_view data) = 0;

  /**
   * @brief Decode bencode data to a BencodeValue
   *
   * @param data Bencode data as a byte pointer
   * @param size Size of the data in bytes
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  virtual BencodeValue decode(const uint8_t *data, size_t size) = 0;

  /**
   * @brief Decode bencode data to a BencodeValue asynchronously
   *
   * @param data Bencode data as a byte pointer
   * @param size Size of the data in bytes
   * @return Future containing the decoded BencodeValue
   */
  virtual std::future<BencodeValue> decode_async(const uint8_t *data,
                                                 size_t size) = 0;
};

/**
 * @brief Create a new bencode decoder
 *
 * @return Unique pointer to a new bencode decoder
 */
std::unique_ptr<BencodeDecoder> create_bencode_decoder();

} // namespace bitscrape::bencode
