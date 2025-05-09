#include "bitscrape/types/node_id.hpp"

#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace bitscrape::types {

NodeID::NodeID() {
    // Generate a random NodeID
    *this = random();
}

NodeID::NodeID(const IDStorage& bytes) : id_(bytes) {
}

NodeID::NodeID(const std::vector<uint8_t>& bytes) {
    if (bytes.size() != SIZE) {
        throw std::invalid_argument("NodeID: Invalid byte vector size");
    }
    
    std::copy(bytes.begin(), bytes.end(), id_.begin());
}

NodeID::NodeID(const std::string& hex) {
    if (hex.size() != SIZE * 2) {
        throw std::invalid_argument("NodeID: Invalid hex string length");
    }
    
    for (size_t i = 0; i < SIZE; ++i) {
        std::string byte_str = hex.substr(i * 2, 2);
        try {
            id_[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        } catch (const std::exception& e) {
            throw std::invalid_argument("NodeID: Invalid hex string");
        }
    }
}

NodeID::NodeID(std::string_view hex) {
    if (hex.size() != SIZE * 2) {
        throw std::invalid_argument("NodeID: Invalid hex string length");
    }
    
    for (size_t i = 0; i < SIZE; ++i) {
        std::string byte_str(hex.substr(i * 2, 2));
        try {
            id_[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        } catch (const std::exception& e) {
            throw std::invalid_argument("NodeID: Invalid hex string");
        }
    }
}

std::string NodeID::to_hex() const {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    
    for (auto byte : id_) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    
    return oss.str();
}

NodeID NodeID::distance(const NodeID& other) const {
    IDStorage result;
    
    for (size_t i = 0; i < SIZE; ++i) {
        result[i] = id_[i] ^ other.id_[i];
    }
    
    return NodeID(result);
}

std::future<NodeID> NodeID::distance_async(const NodeID& other) const {
    return std::async(std::launch::async, [this, other]() {
        return this->distance(other);
    });
}

NodeID NodeID::random() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    
    IDStorage bytes;
    for (auto& byte : bytes) {
        byte = dist(gen);
    }
    
    return NodeID(bytes);
}

std::future<NodeID> NodeID::random_async() {
    return std::async(std::launch::async, []() {
        return random();
    });
}

bool NodeID::operator==(const NodeID& other) const {
    return id_ == other.id_;
}

bool NodeID::operator!=(const NodeID& other) const {
    return !(*this == other);
}

bool NodeID::operator<(const NodeID& other) const {
    return id_ < other.id_;
}

bool NodeID::operator>(const NodeID& other) const {
    return other < *this;
}

bool NodeID::operator<=(const NodeID& other) const {
    return !(other < *this);
}

bool NodeID::operator>=(const NodeID& other) const {
    return !(*this < other);
}

} // namespace bitscrape::types
