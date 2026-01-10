#pragma once

#include "bitscrape/bencode/bencode_value.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <future>
#include <functional>

namespace bitscrape::bencode {

/**
 * @brief Interface for bencode encoders
 * 
 * BencodeEncoder is responsible for encoding BencodeValue objects to bencode format.
 */
class BencodeEncoder {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~BencodeEncoder() = default;
    
    /**
     * @brief Encode a BencodeValue to bencode format
     * 
     * @param value BencodeValue to encode
     * @return Encoded data as a byte vector
     */
    virtual std::vector<uint8_t> encode(const BencodeValue& value) = 0;
    
    /**
     * @brief Encode a BencodeValue to bencode format asynchronously
     * 
     * @param value BencodeValue to encode
     * @return Future containing the encoded data as a byte vector
     */
    virtual std::future<std::vector<uint8_t>> encode_async(const BencodeValue& value) = 0;
    
    /**
     * @brief Encode a string to bencode format
     * 
     * @param value String to encode
     * @return Encoded data as a byte vector
     */
    virtual std::vector<uint8_t> encode_string(const std::string& value) = 0;
    
    /**
     * @brief Encode a string to bencode format asynchronously
     * 
     * @param value String to encode
     * @return Future containing the encoded data as a byte vector
     */
    virtual std::future<std::vector<uint8_t>> encode_string_async(const std::string& value) = 0;
    
    /**
     * @brief Encode an integer to bencode format
     * 
     * @param value Integer to encode
     * @return Encoded data as a byte vector
     */
    virtual std::vector<uint8_t> encode_integer(int64_t value) = 0;
    
    /**
     * @brief Encode an integer to bencode format asynchronously
     * 
     * @param value Integer to encode
     * @return Future containing the encoded data as a byte vector
     */
    virtual std::future<std::vector<uint8_t>> encode_integer_async(int64_t value) = 0;
    
    /**
     * @brief Encode a list to bencode format
     * 
     * @param value List to encode
     * @return Encoded data as a byte vector
     */
    virtual std::vector<uint8_t> encode_list(const std::vector<BencodeValue>& value) = 0;
    
    /**
     * @brief Encode a list to bencode format asynchronously
     * 
     * @param value List to encode
     * @return Future containing the encoded data as a byte vector
     */
    virtual std::future<std::vector<uint8_t>> encode_list_async(const std::vector<BencodeValue>& value) = 0;
    
    /**
     * @brief Encode a dictionary to bencode format
     * 
     * @param value Dictionary to encode
     * @return Encoded data as a byte vector
     */
    virtual std::vector<uint8_t> encode_dict(const std::map<std::string, BencodeValue>& value) = 0;
    
    /**
     * @brief Encode a dictionary to bencode format asynchronously
     * 
     * @param value Dictionary to encode
     * @return Future containing the encoded data as a byte vector
     */
    virtual std::future<std::vector<uint8_t>> encode_dict_async(const std::map<std::string, BencodeValue>& value) = 0;
};

/**
 * @brief Create a new bencode encoder
 * 
 * @return Unique pointer to a new bencode encoder
 */
std::unique_ptr<BencodeEncoder> create_bencode_encoder();

} // namespace bitscrape::bencode
