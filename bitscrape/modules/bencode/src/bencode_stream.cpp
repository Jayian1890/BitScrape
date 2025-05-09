#include "bitscrape/bencode/bencode_stream.hpp"
#include "bitscrape/bencode/bencode_decoder.hpp"
#include "bitscrape/bencode/bencode_encoder.hpp"

#include <algorithm>
#include <future>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace bitscrape::bencode {

/**
 * @brief Implementation of the bencode stream
 */
class BencodeStreamImpl : public BencodeStream {
public:
  /**
   * @brief Constructor
   */
  BencodeStreamImpl()
      : encoder_(create_bencode_encoder()), decoder_(create_bencode_decoder()) {
  }

  /**
   * @brief Destructor
   */
  ~BencodeStreamImpl() override = default;

  /**
   * @brief Read a BencodeValue from a stream
   *
   * @param stream Input stream to read from
   * @return Decoded BencodeValue
   * @throws std::runtime_error if the data is not valid bencode
   */
  BencodeValue read(std::istream &stream) override {
    // Peek at the first character to determine the type
    char c = stream.peek();

    if (std::isdigit(c)) {
      // String
      std::string value = read_string(stream);
      return BencodeValue(value);
    } else if (c == 'i') {
      // Integer
      int64_t value = read_integer(stream);
      return BencodeValue(value);
    } else if (c == 'l') {
      // List
      std::vector<BencodeValue> value = read_list(stream);
      return BencodeValue(value);
    } else if (c == 'd') {
      // Dictionary
      std::map<std::string, BencodeValue> value = read_dict(stream);
      return BencodeValue(value);
    } else {
      throw std::runtime_error("BencodeStream: Invalid bencode data");
    }
  }

  /**
   * @brief Read a BencodeValue from a stream asynchronously
   *
   * @param stream Input stream to read from
   * @return Future containing the decoded BencodeValue
   */
  std::future<BencodeValue> read_async(std::istream &stream) override {
    return std::async(std::launch::async,
                      [this, &stream]() { return this->read(stream); });
  }

  /**
   * @brief Write a BencodeValue to a stream
   *
   * @param value BencodeValue to write
   * @param stream Output stream to write to
   * @return Number of bytes written
   */
  size_t write(const BencodeValue &value, std::ostream &stream) override {
    // Encode the value
    auto encoded = encoder_->encode(value);

    // Write the encoded data to the stream
    stream.write(reinterpret_cast<const char *>(encoded.data()),
                 encoded.size());

    return encoded.size();
  }

  /**
   * @brief Write a BencodeValue to a stream asynchronously
   *
   * @param value BencodeValue to write
   * @param stream Output stream to write to
   * @return Future containing the number of bytes written
   */
  std::future<size_t> write_async(const BencodeValue &value,
                                  std::ostream &stream) override {
    return std::async(std::launch::async, [this, value, &stream]() {
      return this->write(value, stream);
    });
  }

  /**
   * @brief Read a string from a stream
   *
   * @param stream Input stream to read from
   * @return Decoded string
   * @throws std::runtime_error if the data is not a valid bencode string
   */
  std::string read_string(std::istream &stream) override {
    // Read the length
    std::string length_str;
    char c;
    while (stream.get(c) && c != ':') {
      if (!std::isdigit(c)) {
        throw std::runtime_error("BencodeStream: Invalid string length");
      }
      length_str.push_back(c);
    }

    if (!stream) {
      throw std::runtime_error("BencodeStream: Unexpected end of stream");
    }

    // Parse the length
    size_t length = std::stoull(length_str);

    // Read the string
    std::string value;
    value.resize(length);
    stream.read(&value[0], length);

    if (!stream && !stream.eof()) {
      throw std::runtime_error("BencodeStream: Unexpected end of stream");
    }

    return value;
  }

  /**
   * @brief Read a string from a stream asynchronously
   *
   * @param stream Input stream to read from
   * @return Future containing the decoded string
   */
  std::future<std::string> read_string_async(std::istream &stream) override {
    return std::async(std::launch::async,
                      [this, &stream]() { return this->read_string(stream); });
  }

  /**
   * @brief Read an integer from a stream
   *
   * @param stream Input stream to read from
   * @return Decoded integer
   * @throws std::runtime_error if the data is not a valid bencode integer
   */
  int64_t read_integer(std::istream &stream) override {
    // Check for 'i'
    char c;
    if (!stream.get(c) || c != 'i') {
      throw std::runtime_error("BencodeStream: Invalid integer format");
    }

    // Read the integer
    std::string int_str;
    while (stream.get(c) && c != 'e') {
      int_str.push_back(c);
    }

    if (!stream) {
      throw std::runtime_error("BencodeStream: Unexpected end of stream");
    }

    // Parse the integer
    return std::stoll(int_str);
  }

  /**
   * @brief Read an integer from a stream asynchronously
   *
   * @param stream Input stream to read from
   * @return Future containing the decoded integer
   */
  std::future<int64_t> read_integer_async(std::istream &stream) override {
    return std::async(std::launch::async,
                      [this, &stream]() { return this->read_integer(stream); });
  }

  /**
   * @brief Read a list from a stream
   *
   * @param stream Input stream to read from
   * @return Decoded list
   * @throws std::runtime_error if the data is not a valid bencode list
   */
  std::vector<BencodeValue> read_list(std::istream &stream) override {
    // Check for 'l'
    char c;
    if (!stream.get(c) || c != 'l') {
      throw std::runtime_error("BencodeStream: Invalid list format");
    }

    std::vector<BencodeValue> list;

    // Peek at the next character
    c = stream.peek();

    // Read items until we reach 'e'
    while (stream && c != 'e') {
      // Determine the type of the item
      if (std::isdigit(c)) {
        // String
        std::string value = read_string(stream);
        list.push_back(BencodeValue(value));
      } else if (c == 'i') {
        // Integer
        int64_t value = read_integer(stream);
        list.push_back(BencodeValue(value));
      } else if (c == 'l') {
        // List
        std::vector<BencodeValue> value = read_list(stream);
        list.push_back(BencodeValue(value));
      } else if (c == 'd') {
        // Dictionary
        std::map<std::string, BencodeValue> value = read_dict(stream);
        list.push_back(BencodeValue(value));
      } else {
        throw std::runtime_error("BencodeStream: Invalid bencode data in list");
      }

      // Peek at the next character
      c = stream.peek();
    }

    // Skip the 'e'
    stream.get(c);

    if (!stream) {
      throw std::runtime_error("BencodeStream: Unexpected end of stream");
    }

    return list;
  }

  /**
   * @brief Read a list from a stream asynchronously
   *
   * @param stream Input stream to read from
   * @return Future containing the decoded list
   */
  std::future<std::vector<BencodeValue>>
  read_list_async(std::istream &stream) override {
    return std::async(std::launch::async,
                      [this, &stream]() { return this->read_list(stream); });
  }

  /**
   * @brief Read a dictionary from a stream
   *
   * @param stream Input stream to read from
   * @return Decoded dictionary
   * @throws std::runtime_error if the data is not a valid bencode dictionary
   */
  std::map<std::string, BencodeValue> read_dict(std::istream &stream) override {
    // Check for 'd'
    char c;
    if (!stream.get(c) || c != 'd') {
      throw std::runtime_error("BencodeStream: Invalid dictionary format");
    }

    std::map<std::string, BencodeValue> dict;

    // Peek at the next character
    c = stream.peek();

    // Read key-value pairs until we reach 'e'
    while (stream && c != 'e') {
      // Read the key (must be a string)
      if (!std::isdigit(c)) {
        throw std::runtime_error(
            "BencodeStream: Dictionary key must be a string");
      }

      std::string key = read_string(stream);

      // Peek at the next character to determine the type of the value
      c = stream.peek();

      // Read the value based on its type
      if (std::isdigit(c)) {
        // String
        std::string value = read_string(stream);
        dict[key] = BencodeValue(value);
      } else if (c == 'i') {
        // Integer
        int64_t value = read_integer(stream);
        dict[key] = BencodeValue(value);
      } else if (c == 'l') {
        // List
        std::vector<BencodeValue> value = read_list(stream);
        dict[key] = BencodeValue(value);
      } else if (c == 'd') {
        // Dictionary
        std::map<std::string, BencodeValue> value = read_dict(stream);
        dict[key] = BencodeValue(value);
      } else {
        throw std::runtime_error(
            "BencodeStream: Invalid bencode data in dictionary");
      }

      // Peek at the next character
      c = stream.peek();
    }

    // Skip the 'e'
    stream.get(c);

    if (!stream) {
      throw std::runtime_error("BencodeStream: Unexpected end of stream");
    }

    return dict;
  }

  /**
   * @brief Read a dictionary from a stream asynchronously
   *
   * @param stream Input stream to read from
   * @return Future containing the decoded dictionary
   */
  std::future<std::map<std::string, BencodeValue>>
  read_dict_async(std::istream &stream) override {
    return std::async(std::launch::async,
                      [this, &stream]() { return this->read_dict(stream); });
  }

private:
  std::unique_ptr<BencodeEncoder> encoder_; ///< Bencode encoder
  std::unique_ptr<BencodeDecoder> decoder_; ///< Bencode decoder
};

std::unique_ptr<BencodeStream> create_bencode_stream() {
  return std::make_unique<BencodeStreamImpl>();
}

} // namespace bitscrape::bencode
