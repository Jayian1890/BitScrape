#include "bitscrape/types/info_hash.hpp"

#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace bitscrape::types {

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
  // This is a simplified implementation that just returns a dummy hash
  // In a real implementation, we would parse the bencode data, extract the info
  // dictionary, and calculate the SHA1 hash

  // Create a dummy hash based on the first 20 bytes of the data (or less if
  // data is smaller)
  HashStorage hash;
  std::fill(hash.begin(), hash.end(), 0);

  if (!data.empty()) {
    // Copy up to SIZE bytes from data to hash
    size_t bytes_to_copy = std::min(data.size(), static_cast<size_t>(SIZE));
    std::copy(data.begin(), data.begin() + bytes_to_copy, hash.begin());
  }

  return InfoHash(hash);
}

std::future<InfoHash>
InfoHash::from_bencode_async(const std::vector<uint8_t> &data) {
  return std::async(std::launch::async,
                    [data]() { return from_bencode(data); });
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
