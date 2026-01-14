#include "bitscrape/bencode/bencode_decoder.hpp"

#include <algorithm>
#include <cctype>
#include <future>
#include <sstream>
#include <stdexcept>

namespace bitscrape::bencode {

/**
 * @brief Implementation of the bencode decoder
 */
class BencodeDecoderImpl : public BencodeDecoder {
public:
  /**
   * @brief Constructor
   */
  BencodeDecoderImpl() = default;

  /**
   * @brief Destructor
   */
  ~BencodeDecoderImpl() override = default;

  /**
   * @brief Decode bencode data to a BencodeValue
   *
   * @param data Bencode data as a byte vector
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  BencodeValue decode(const std::vector<uint8_t> &data) override {
    if (data.empty()) {
      throw std::runtime_error("BencodeDecoder: Empty data");
    }

    return decode(data.data(), data.size());
  }

  /**
   * @brief Decode bencode data to a BencodeValue asynchronously
   *
   * @param data Bencode data as a byte vector
   * @return Future containing the decoded BencodeValue
   */
  std::future<BencodeValue>
  decode_async(const std::vector<uint8_t> &data) override {
    return std::async(std::launch::async,
                      [this, data]() { return this->decode(data); });
  }

  /**
   * @brief Decode bencode data to a BencodeValue
   *
   * @param data Bencode data as a string
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  BencodeValue decode(const std::string &data) override {
    if (data.empty()) {
      throw std::runtime_error("BencodeDecoder: Empty data");
    }

    return decode(reinterpret_cast<const uint8_t *>(data.data()), data.size());
  }

  /**
   * @brief Decode bencode data to a BencodeValue asynchronously
   *
   * @param data Bencode data as a string
   * @return Future containing the decoded BencodeValue
   */
  std::future<BencodeValue> decode_async(const std::string &data) override {
    return std::async(std::launch::async,
                      [this, data]() { return this->decode(data); });
  }

  /**
   * @brief Decode bencode data to a BencodeValue
   *
   * @param data Bencode data as a string view
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  BencodeValue decode(std::string_view data) override {
    if (data.empty()) {
      throw std::runtime_error("BencodeDecoder: Empty data");
    }

    return decode(reinterpret_cast<const uint8_t *>(data.data()), data.size());
  }

  /**
   * @brief Decode bencode data to a BencodeValue asynchronously
   *
   * @param data Bencode data as a string view
   * @return Future containing the decoded BencodeValue
   */
  std::future<BencodeValue> decode_async(std::string_view data) override {
    return std::async(std::launch::async,
                      [this, data]() { return this->decode(data); });
  }

  /**
   * @brief Decode bencode data to a BencodeValue
   *
   * @param data Bencode data as a byte pointer
   * @param size Size of the data in bytes
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  BencodeValue decode(const uint8_t *data, size_t size) override {
    if (data == nullptr || size == 0) {
      throw std::runtime_error("BencodeDecoder: Empty data");
    }

    size_t pos = 0;
    return decode_value(data, size, pos);
  }

  /**
   * @brief Decode bencode data to a BencodeValue asynchronously
   *
   * @param data Bencode data as a byte pointer
   * @param size Size of the data in bytes
   * @return Future containing the decoded BencodeValue
   */
  std::future<BencodeValue> decode_async(const uint8_t *data,
                                         size_t size) override {
    return std::async(std::launch::async, [this, data, size]() {
      return this->decode(data, size);
    });
  }

private:
  /**
   * @brief Decode a bencode value
   *
   * @param data Bencode data
   * @param size Size of the data
   * @param pos Current position in the data (updated)
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  BencodeValue decode_value(const uint8_t *data, size_t size, size_t &pos) {
    if (pos >= size) {
      throw std::runtime_error("BencodeDecoder: Unexpected end of data");
    }

    char c = static_cast<char>(data[pos]);

    if (std::isdigit(c)) {
      return decode_string(data, size, pos);
    } else if (c == 'i') {
      return decode_integer(data, size, pos);
    } else if (c == 'l') {
      return decode_list(data, size, pos);
    } else if (c == 'd') {
      return decode_dict(data, size, pos);
    } else {
      throw std::runtime_error("BencodeDecoder: Invalid bencode data");
    }
  }

  /**
   * @brief Decode a bencode string
   *
   * @param data Bencode data
   * @param size Size of the data
   * @param pos Current position in the data (updated)
   * @return Decoded string as a BencodeValue
   * @throws std::runtime_error if the data is not a valid bencode string
   */
  BencodeValue decode_string(const uint8_t *data, size_t size, size_t &pos) {
    // Format: <length>:<string>
    size_t length_start = pos;

    // Find the colon
    while (pos < size && data[pos] != ':') {
      if (!std::isdigit(data[pos])) {
        throw std::runtime_error("BencodeDecoder: Invalid string length");
      }
      ++pos;
    }

    if (pos >= size) {
      throw std::runtime_error("BencodeDecoder: Unexpected end of data");
    }

    // Parse the length
    std::string length_str(reinterpret_cast<const char *>(data + length_start),
                           pos - length_start);
    size_t length = std::stoull(length_str);

    // Skip the colon
    ++pos;

    // Check if there's enough data
    if (pos + length > size) {
      throw std::runtime_error(
          "BencodeDecoder: String length exceeds data size");
    }

    // Extract the string
    std::string value(reinterpret_cast<const char *>(data + pos), length);
    pos += length;

    return BencodeValue(value);
  }

  /**
   * @brief Decode a bencode integer
   *
   * @param data Bencode data
   * @param size Size of the data
   * @param pos Current position in the data (updated)
   * @return Decoded integer as a BencodeValue
   * @throws std::runtime_error if the data is not a valid bencode integer
   */
  BencodeValue decode_integer(const uint8_t *data, size_t size, size_t &pos) {
    // Format: i<integer>e

    // Skip the 'i'
    ++pos;

    if (pos >= size) {
      throw std::runtime_error("BencodeDecoder: Unexpected end of data");
    }

    // Find the 'e'
    size_t int_start = pos;
    while (pos < size && data[pos] != 'e') {
      ++pos;
    }

    if (pos >= size) {
      throw std::runtime_error("BencodeDecoder: Unexpected end of data");
    }

    // Parse the integer
    std::string int_str(reinterpret_cast<const char *>(data + int_start),
                        pos - int_start);
    int64_t value = std::stoll(int_str);

    // Skip the 'e'
    ++pos;

    return BencodeValue(value);
  }

  /**
   * @brief Decode a bencode list
   *
   * @param data Bencode data
   * @param size Size of the data
   * @param pos Current position in the data (updated)
   * @return Decoded list as a BencodeValue
   * @throws std::runtime_error if the data is not a valid bencode list
   */
  BencodeValue decode_list(const uint8_t *data, size_t size, size_t &pos) {
    // Format: l<item1><item2>...e

    // Skip the 'l'
    ++pos;

    std::vector<BencodeValue> list;

    // Parse items until we reach 'e'
    while (pos < size && data[pos] != 'e') {
      list.push_back(decode_value(data, size, pos));
    }

    if (pos >= size) {
      throw std::runtime_error("BencodeDecoder: Unexpected end of data");
    }

    // Skip the 'e'
    ++pos;

    return BencodeValue(list);
  }

  /**
   * @brief Decode a bencode dictionary
   *
   * @param data Bencode data
   * @param size Size of the data
   * @param pos Current position in the data (updated)
   * @return Decoded dictionary as a BencodeValue
   * @throws std::runtime_error if the data is not a valid bencode dictionary
   */
  BencodeValue decode_dict(const uint8_t *data, size_t size, size_t &pos) {
    // Format: d<key1><value1><key2><value2>...e

    // Skip the 'd'
    ++pos;

    std::map<std::string, BencodeValue> dict;

    // Parse key-value pairs until we reach 'e'
    while (pos < size && data[pos] != 'e') {
      // Parse the key (must be a string)
      if (!std::isdigit(data[pos])) {
        throw std::runtime_error(
            "BencodeDecoder: Dictionary key must be a string");
      }

      BencodeValue key_value = decode_string(data, size, pos);
      std::string key = key_value.as_string();

      // Parse the value
      BencodeValue value = decode_value(data, size, pos);

      // Add the key-value pair to the dictionary
      dict[key] = value;
    }

    if (pos >= size) {
      throw std::runtime_error("BencodeDecoder: Unexpected end of data");
    }

    // Skip the 'e'
    ++pos;

    return BencodeValue(dict);
  }
};

std::unique_ptr<BencodeDecoder> create_bencode_decoder() {
  return std::make_unique<BencodeDecoderImpl>();
}

} // namespace bitscrape::bencode
