#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <future>
#include <functional>

namespace bitscrape::types {

/**
 * @brief Represents a 160-bit BitTorrent InfoHash
 * 
 * InfoHash is a 160-bit identifier used in the BitTorrent network to identify torrents.
 * It provides methods for creating, comparing, and manipulating info hashes.
 */
class InfoHash {
public:
    /// Size of an InfoHash in bytes (160 bits = 20 bytes)
    static constexpr size_t SIZE = 20;
    
    /// Type for the internal storage of the InfoHash
    using HashStorage = std::array<uint8_t, SIZE>;

    /**
     * @brief Default constructor creates a zero InfoHash
     */
    InfoHash();
    
    /**
     * @brief Create an InfoHash from a byte array
     * 
     * @param bytes Array of bytes representing the InfoHash
     */
    explicit InfoHash(const HashStorage& bytes);
    
    /**
     * @brief Create an InfoHash from a byte vector
     * 
     * @param bytes Vector of bytes representing the InfoHash
     * @throws std::invalid_argument if the vector size is not SIZE
     */
    explicit InfoHash(const std::vector<uint8_t>& bytes);
    
    /**
     * @brief Create an InfoHash from a hexadecimal string
     * 
     * @param hex Hexadecimal string representing the InfoHash
     * @throws std::invalid_argument if the string is not a valid hex string or wrong length
     */
    explicit InfoHash(const std::string& hex);
    
    /**
     * @brief Create an InfoHash from a string_view
     * 
     * @param hex String view representing the InfoHash in hexadecimal
     * @throws std::invalid_argument if the string is not a valid hex string or wrong length
     */
    explicit InfoHash(std::string_view hex);
    
    /**
     * @brief Copy constructor
     */
    InfoHash(const InfoHash& other) = default;
    
    /**
     * @brief Move constructor
     */
    InfoHash(InfoHash&& other) noexcept = default;
    
    /**
     * @brief Copy assignment operator
     */
    InfoHash& operator=(const InfoHash& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    InfoHash& operator=(InfoHash&& other) noexcept = default;
    
    /**
     * @brief Destructor
     */
    ~InfoHash() = default;
    
    /**
     * @brief Get the raw bytes of the InfoHash
     * 
     * @return Const reference to the internal byte array
     */
    const HashStorage& bytes() const { return hash_; }
    
    /**
     * @brief Get the InfoHash as a hexadecimal string
     * 
     * @return Hexadecimal string representation of the InfoHash
     */
    std::string to_hex() const;
    
    /**
     * @brief Calculate the InfoHash from a bencode dictionary asynchronously
     * 
     * @param data The bencode data
     * @return A future that will contain the calculated InfoHash
     */
    static std::future<InfoHash> from_bencode_async(const std::vector<uint8_t>& data);
    
    /**
     * @brief Calculate the InfoHash from a bencode dictionary
     * 
     * @param data The bencode data
     * @return The calculated InfoHash
     */
    static InfoHash from_bencode(const std::vector<uint8_t>& data);
    
    /**
     * @brief Generate a random InfoHash
     * 
     * @return A new random InfoHash
     */
    static InfoHash random();
    
    /**
     * @brief Generate a random InfoHash asynchronously
     * 
     * @return A future that will contain a random InfoHash
     */
    static std::future<InfoHash> random_async();
    
    /**
     * @brief Equality operator
     */
    bool operator==(const InfoHash& other) const;
    
    /**
     * @brief Inequality operator
     */
    bool operator!=(const InfoHash& other) const;
    
    /**
     * @brief Less than operator for ordering
     */
    bool operator<(const InfoHash& other) const;
    
    /**
     * @brief Greater than operator for ordering
     */
    bool operator>(const InfoHash& other) const;
    
    /**
     * @brief Less than or equal operator for ordering
     */
    bool operator<=(const InfoHash& other) const;
    
    /**
     * @brief Greater than or equal operator for ordering
     */
    bool operator>=(const InfoHash& other) const;

private:
    HashStorage hash_; ///< The internal storage for the InfoHash
};

} // namespace bitscrape::types
