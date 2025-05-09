#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <future>
#include <chrono>

namespace bitscrape::types {

/**
 * @brief Represents a token used in DHT protocol
 * 
 * DHTToken encapsulates a token used in the DHT protocol for verification.
 * Tokens are used to prevent unauthorized announce_peer messages.
 */
class DHTToken {
public:
    /**
     * @brief Default constructor creates a random token
     */
    DHTToken();
    
    /**
     * @brief Create a token from a byte vector
     * 
     * @param bytes Token bytes
     */
    explicit DHTToken(const std::vector<uint8_t>& bytes);
    
    /**
     * @brief Create a token from a string
     * 
     * @param str Token string
     */
    explicit DHTToken(const std::string& str);
    
    /**
     * @brief Copy constructor
     */
    DHTToken(const DHTToken& other) = default;
    
    /**
     * @brief Move constructor
     */
    DHTToken(DHTToken&& other) noexcept = default;
    
    /**
     * @brief Copy assignment operator
     */
    DHTToken& operator=(const DHTToken& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    DHTToken& operator=(DHTToken&& other) noexcept = default;
    
    /**
     * @brief Destructor
     */
    ~DHTToken() = default;
    
    /**
     * @brief Get the token bytes
     * 
     * @return Token bytes
     */
    const std::vector<uint8_t>& bytes() const { return bytes_; }
    
    /**
     * @brief Get the token as a string
     * 
     * @return Token string
     */
    std::string to_string() const;
    
    /**
     * @brief Get the token creation time
     * 
     * @return Token creation time
     */
    std::chrono::system_clock::time_point creation_time() const { return creation_time_; }
    
    /**
     * @brief Check if the token is expired
     * 
     * @param max_age Maximum token age
     * @return true if the token is expired, false otherwise
     */
    bool is_expired(std::chrono::seconds max_age) const;
    
    /**
     * @brief Generate a random token
     * 
     * @return Random token
     */
    static DHTToken random();
    
    /**
     * @brief Generate a random token asynchronously
     * 
     * @return Future containing a random token
     */
    static std::future<DHTToken> random_async();
    
    /**
     * @brief Equality operator
     */
    bool operator==(const DHTToken& other) const;
    
    /**
     * @brief Inequality operator
     */
    bool operator!=(const DHTToken& other) const;

private:
    std::vector<uint8_t> bytes_;                      ///< Token bytes
    std::chrono::system_clock::time_point creation_time_; ///< Token creation time
};

} // namespace bitscrape::types
