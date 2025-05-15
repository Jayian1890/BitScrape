#include "bitscrape/dht/token_manager.hpp"

#include <random>
#include <chrono>
#include <future>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "bitscrape/lock/lock_manager_singleton.hpp"

namespace bitscrape::dht {

TokenManager::TokenManager(lock::LockManager& lock_manager)
    : last_rotation_(std::chrono::system_clock::now()),
      lock_manager_(lock_manager),
      resource_id_(lock_manager.register_resource(get_resource_name(), lock::LockManager::LockPriority::NORMAL)) {
    // Generate initial secrets
    rotate_secret();
}

TokenManager::TokenManager(std::shared_ptr<lock::LockManager> lock_manager)
    : last_rotation_(std::chrono::system_clock::now()),
      lock_manager_(*lock_manager),
      resource_id_(lock_manager->register_resource(get_resource_name(), lock::LockManager::LockPriority::NORMAL)) {
    // Generate initial secrets
    rotate_secret();
}

std::string TokenManager::get_resource_name() const {
    return "dht.token_manager";
}

types::DHTToken TokenManager::generate_token(const types::Endpoint& endpoint) {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::EXCLUSIVE);

    // Check if we need to rotate the secret
    auto now = std::chrono::system_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_rotation_).count() >= TOKEN_ROTATION_INTERVAL) {
        rotate_secret();
        last_rotation_ = now;
    }

    // Generate a token with the current secret
    return generate_token_with_secret(endpoint, current_secret_);
}

std::future<types::DHTToken> TokenManager::generate_token_async(const types::Endpoint& endpoint) {
    return std::async(std::launch::async, [this, endpoint]() {
        return this->generate_token(endpoint);
    });
}

bool TokenManager::verify_token(const types::DHTToken& token, const types::Endpoint& endpoint) {
    auto guard = lock_manager_.get_lock_guard(resource_id_, lock::LockManager::LockType::SHARED);

    // Generate tokens with both secrets and compare
    auto current_token = generate_token_with_secret(endpoint, current_secret_);
    auto previous_token = generate_token_with_secret(endpoint, previous_secret_);

    return token == current_token || token == previous_token;
}

std::future<bool> TokenManager::verify_token_async(const types::DHTToken& token, const types::Endpoint& endpoint) {
    return std::async(std::launch::async, [this, token, endpoint]() {
        return this->verify_token(token, endpoint);
    });
}

void TokenManager::rotate_secret() {
    // Move the current secret to the previous secret
    previous_secret_ = current_secret_;

    // Generate a new random secret
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);

    current_secret_.clear();
    for (int i = 0; i < 16; ++i) {
        current_secret_.push_back(dis(gen));
    }
}

types::DHTToken TokenManager::generate_token_with_secret(const types::Endpoint& endpoint, const std::vector<uint8_t>& secret) {
    // If the secret is empty, return an empty token
    if (secret.empty()) {
        return types::DHTToken();
    }

    // Combine the endpoint and secret
    std::string data = endpoint.to_string();
    data.append(reinterpret_cast<const char*>(secret.data()), secret.size());

    // Hash the data to create a token
    // In a real implementation, we would use a cryptographic hash function
    // For simplicity, we'll just use a simple hash algorithm here
    std::vector<uint8_t> token_data;
    const size_t TOKEN_SIZE = 20; // Use a fixed size of 20 bytes
    token_data.reserve(TOKEN_SIZE);

    for (size_t i = 0; i < TOKEN_SIZE; ++i) {
        uint8_t byte = 0;
        for (size_t j = 0; j < data.size(); ++j) {
            byte ^= data[j] ^ secret[j % secret.size()];
            byte = (byte << 1) | (byte >> 7);  // Rotate left
        }
        token_data.push_back(byte);
    }

    return types::DHTToken(token_data);
}

} // namespace bitscrape::dht
