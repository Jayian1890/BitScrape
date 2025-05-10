#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <future>

#include "bitscrape/types/dht_token.hpp"
#include "bitscrape/types/endpoint.hpp"

namespace bitscrape::dht {

/**
 * @brief Manages tokens for the DHT protocol
 * 
 * TokenManager is responsible for generating and verifying tokens used in the DHT protocol.
 * Tokens are used to prevent unauthorized announce_peer messages.
 */
class TokenManager {
public:
    /// Token rotation interval in seconds
    static constexpr int TOKEN_ROTATION_INTERVAL = 300;
    
    /**
     * @brief Default constructor
     */
    TokenManager();
    
    /**
     * @brief Generate a token for an endpoint
     * 
     * @param endpoint Endpoint to generate a token for
     * @return Generated token
     */
    types::DHTToken generate_token(const types::Endpoint& endpoint);
    
    /**
     * @brief Generate a token for an endpoint asynchronously
     * 
     * @param endpoint Endpoint to generate a token for
     * @return Future containing the generated token
     */
    std::future<types::DHTToken> generate_token_async(const types::Endpoint& endpoint);
    
    /**
     * @brief Verify a token for an endpoint
     * 
     * @param token Token to verify
     * @param endpoint Endpoint to verify the token for
     * @return true if the token is valid, false otherwise
     */
    bool verify_token(const types::DHTToken& token, const types::Endpoint& endpoint);
    
    /**
     * @brief Verify a token for an endpoint asynchronously
     * 
     * @param token Token to verify
     * @param endpoint Endpoint to verify the token for
     * @return Future containing true if the token is valid, false otherwise
     */
    std::future<bool> verify_token_async(const types::DHTToken& token, const types::Endpoint& endpoint);
    
private:
    /**
     * @brief Rotate the secret
     */
    void rotate_secret();
    
    /**
     * @brief Generate a token for an endpoint with a specific secret
     * 
     * @param endpoint Endpoint to generate a token for
     * @param secret Secret to use
     * @return Generated token
     */
    types::DHTToken generate_token_with_secret(const types::Endpoint& endpoint, const std::vector<uint8_t>& secret);
    
    std::vector<uint8_t> current_secret_;                    ///< Current secret for token generation
    std::vector<uint8_t> previous_secret_;                   ///< Previous secret for token verification
    std::chrono::system_clock::time_point last_rotation_;    ///< Time of the last secret rotation
    mutable std::mutex mutex_;                               ///< Mutex for thread safety
};

} // namespace bitscrape::dht
