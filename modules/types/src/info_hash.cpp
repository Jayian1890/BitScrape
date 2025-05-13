#include "bitscrape/types/info_hash.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace bitscrape::types {

// Simple SHA-1 implementation for calculating info hashes
namespace {
    // SHA-1 constants
    constexpr uint32_t SHA1_BLOCK_SIZE = 64; // 512 bits
    constexpr uint32_t SHA1_DIGEST_SIZE = 20; // 160 bits

    // SHA-1 functions
    inline uint32_t rotate_left(uint32_t value, int bits) {
        return (value << bits) | (value >> (32 - bits));
    }

    inline uint32_t f1(uint32_t b, uint32_t c, uint32_t d) {
        return (b & c) | ((~b) & d);
    }

    inline uint32_t f2(uint32_t b, uint32_t c, uint32_t d) {
        return b ^ c ^ d;
    }

    inline uint32_t f3(uint32_t b, uint32_t c, uint32_t d) {
        return (b & c) | (b & d) | (c & d);
    }

    // Calculate SHA-1 hash of data
    std::array<uint8_t, SHA1_DIGEST_SIZE> calculate_sha1(const uint8_t* data, size_t length) {
        // Initialize hash values (FIPS 180-1)
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        // Pre-processing: padding with zeros
        uint64_t bit_length = length * 8;
        size_t padding_length = (length % SHA1_BLOCK_SIZE < 56) ?
            (56 - length % SHA1_BLOCK_SIZE) :
            (120 - length % SHA1_BLOCK_SIZE);

        std::vector<uint8_t> padded_data(length + padding_length + 8, 0);
        std::copy(data, data + length, padded_data.begin());
        padded_data[length] = 0x80; // Append a single '1' bit

        // Append length as 64-bit big-endian integer
        for (int i = 0; i < 8; ++i) {
            padded_data[padded_data.size() - 8 + i] =
                static_cast<uint8_t>((bit_length >> (56 - i * 8)) & 0xFF);
        }

        // Process the message in 512-bit chunks
        for (size_t chunk = 0; chunk < padded_data.size(); chunk += SHA1_BLOCK_SIZE) {
            // Break chunk into sixteen 32-bit big-endian words
            std::array<uint32_t, 80> w;
            for (int i = 0; i < 16; ++i) {
                w[i] = (static_cast<uint32_t>(padded_data[chunk + i * 4]) << 24) |
                       (static_cast<uint32_t>(padded_data[chunk + i * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(padded_data[chunk + i * 4 + 2]) << 8) |
                       static_cast<uint32_t>(padded_data[chunk + i * 4 + 3]);
            }

            // Extend the sixteen 32-bit words into eighty 32-bit words
            for (int i = 16; i < 80; ++i) {
                w[i] = rotate_left(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
            }

            // Initialize hash value for this chunk
            uint32_t a = h0;
            uint32_t b = h1;
            uint32_t c = h2;
            uint32_t d = h3;
            uint32_t e = h4;

            // Main loop
            for (int i = 0; i < 80; ++i) {
                uint32_t f, k;

                if (i < 20) {
                    f = f1(b, c, d);
                    k = 0x5A827999;
                } else if (i < 40) {
                    f = f2(b, c, d);
                    k = 0x6ED9EBA1;
                } else if (i < 60) {
                    f = f3(b, c, d);
                    k = 0x8F1BBCDC;
                } else {
                    f = f2(b, c, d);
                    k = 0xCA62C1D6;
                }

                uint32_t temp = rotate_left(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = rotate_left(b, 30);
                b = a;
                a = temp;
            }

            // Add this chunk's hash to result so far
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        // Produce the final hash value as a 160-bit number
        std::array<uint8_t, SHA1_DIGEST_SIZE> hash;
        for (int i = 0; i < 4; ++i) {
            hash[i] = static_cast<uint8_t>((h0 >> (24 - i * 8)) & 0xFF);
            hash[i + 4] = static_cast<uint8_t>((h1 >> (24 - i * 8)) & 0xFF);
            hash[i + 8] = static_cast<uint8_t>((h2 >> (24 - i * 8)) & 0xFF);
            hash[i + 12] = static_cast<uint8_t>((h3 >> (24 - i * 8)) & 0xFF);
            hash[i + 16] = static_cast<uint8_t>((h4 >> (24 - i * 8)) & 0xFF);
        }

        return hash;
    }
}

InfoHash::InfoHash() {
  // Create a zero-filled InfoHash
  std::fill(hash_.begin(), hash_.end(), 0);
}

InfoHash::InfoHash(const HashStorage &bytes) : hash_(bytes) {}

InfoHash::InfoHash(const std::vector<uint8_t> &bytes) {
  if (bytes.size() != SIZE) {
    throw std::invalid_argument("InfoHash: Invalid byte vector size");
  }

  std::copy(bytes.begin(), bytes.end(), hash_.begin());
}

InfoHash::InfoHash(const std::string &hex) {
  if (hex.size() != SIZE * 2) {
    throw std::invalid_argument("InfoHash: Invalid hex string length");
  }

  for (size_t i = 0; i < SIZE; ++i) {
    std::string byte_str = hex.substr(i * 2, 2);
    try {
      hash_[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
    } catch (const std::exception &e) {
      throw std::invalid_argument("InfoHash: Invalid hex string");
    }
  }
}

InfoHash::InfoHash(std::string_view hex) {
  if (hex.size() != SIZE * 2) {
    throw std::invalid_argument("InfoHash: Invalid hex string length");
  }

  for (size_t i = 0; i < SIZE; ++i) {
    std::string byte_str(hex.substr(i * 2, 2));
    try {
      hash_[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
    } catch (const std::exception &e) {
      throw std::invalid_argument("InfoHash: Invalid hex string");
    }
  }
}

std::string InfoHash::to_hex() const {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');

  for (auto byte : hash_) {
    oss << std::setw(2) << static_cast<int>(byte);
  }

  return oss.str();
}

InfoHash InfoHash::from_bencode(const std::vector<uint8_t> &data) {
  try {
    // Calculate SHA-1 hash of the data
    auto hash_array = calculate_sha1(data.data(), data.size());

    // Convert to HashStorage
    HashStorage hash;
    std::copy(hash_array.begin(), hash_array.end(), hash.begin());

    return InfoHash(hash);
  } catch (const std::exception &e) {
    // If there's an error, return a hash based on the raw data
    // This is a fallback mechanism
    HashStorage hash;
    std::fill(hash.begin(), hash.end(), 0);

    if (!data.empty()) {
      // Copy up to SIZE bytes from data to hash
      size_t bytes_to_copy = std::min(data.size(), static_cast<size_t>(SIZE));
      std::copy(data.begin(), data.begin() + bytes_to_copy, hash.begin());
    }

    return InfoHash(hash);
  }
}

std::future<InfoHash>
InfoHash::from_bencode_async(const std::vector<uint8_t> &data) {
  return std::async(std::launch::async,
                    [data]() { return from_bencode(data); });
}

InfoHash InfoHash::from_hex(const std::string& hex) {
  return InfoHash(hex);
}

InfoHash InfoHash::random() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dist(0, 255);

  HashStorage bytes;
  for (auto &byte : bytes) {
    byte = dist(gen);
  }

  return InfoHash(bytes);
}

std::future<InfoHash> InfoHash::random_async() {
  return std::async(std::launch::async, []() { return random(); });
}

bool InfoHash::operator==(const InfoHash &other) const {
  return hash_ == other.hash_;
}

bool InfoHash::operator!=(const InfoHash &other) const {
  return !(*this == other);
}

bool InfoHash::operator<(const InfoHash &other) const {
  return hash_ < other.hash_;
}

bool InfoHash::operator>(const InfoHash &other) const { return other < *this; }

bool InfoHash::operator<=(const InfoHash &other) const {
  return !(other < *this);
}

bool InfoHash::operator>=(const InfoHash &other) const {
  return !(*this < other);
}

} // namespace bitscrape::types
