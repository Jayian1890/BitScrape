#include "bitscrape/types/dht_token.hpp"

#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace bitscrape::types {

DHTToken::DHTToken() : creation_time_(std::chrono::system_clock::now()) {
    // Create a random token
    *this = random();
}

DHTToken::DHTToken(const std::vector<uint8_t>& bytes)
    : bytes_(bytes), creation_time_(std::chrono::system_clock::now()) {
}

DHTToken::DHTToken(const std::string& str)
    : creation_time_(std::chrono::system_clock::now()) {
    bytes_.resize(str.size());
    std::copy(str.begin(), str.end(), bytes_.begin());
}

std::string DHTToken::to_string() const {
    return std::string(bytes_.begin(), bytes_.end());
}

bool DHTToken::is_expired(std::chrono::seconds max_age) const {
    auto now = std::chrono::system_clock::now();
    auto age = now - creation_time_;
    
    return age > max_age;
}

DHTToken DHTToken::random() {
    // Generate a random token of 20 bytes
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    
    std::vector<uint8_t> bytes(20);
    std::generate(bytes.begin(), bytes.end(), [&]() { return dist(gen); });
    
    return DHTToken(bytes);
}

std::future<DHTToken> DHTToken::random_async() {
    return std::async(std::launch::async, []() {
        return random();
    });
}

bool DHTToken::operator==(const DHTToken& other) const {
    return bytes_ == other.bytes_;
}

bool DHTToken::operator!=(const DHTToken& other) const {
    return !(*this == other);
}

} // namespace bitscrape::types
