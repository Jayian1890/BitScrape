#pragma once

#include "bitscrape/bencode/bencode_value.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <future>
#include <functional>
#include <istream>
#include <ostream>

namespace bitscrape::bencode {

/**
 * @brief Interface for bencode streams
 * 
 * BencodeStream is responsible for streaming bencode data to and from streams.
 */
class BencodeStream {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~BencodeStream() = default;
    
    /**
     * @brief Read a BencodeValue from a stream
     * 
     * @param stream Input stream to read from
     * @return Decoded BencodeValue
     * @throws std::runtime_error if the data is not valid bencode
     */
    virtual BencodeValue read(std::istream& stream) = 0;
    
    /**
     * @brief Read a BencodeValue from a stream asynchronously
     * 
     * @param stream Input stream to read from
     * @return Future containing the decoded BencodeValue
     */
    virtual std::future<BencodeValue> read_async(std::istream& stream) = 0;
    
    /**
     * @brief Write a BencodeValue to a stream
     * 
     * @param value BencodeValue to write
     * @param stream Output stream to write to
     * @return Number of bytes written
     */
    virtual size_t write(const BencodeValue& value, std::ostream& stream) = 0;
    
    /**
     * @brief Write a BencodeValue to a stream asynchronously
     * 
     * @param value BencodeValue to write
     * @param stream Output stream to write to
     * @return Future containing the number of bytes written
     */
    virtual std::future<size_t> write_async(const BencodeValue& value, std::ostream& stream) = 0;
    
    /**
     * @brief Read a string from a stream
     * 
     * @param stream Input stream to read from
     * @return Decoded string
     * @throws std::runtime_error if the data is not a valid bencode string
     */
    virtual std::string read_string(std::istream& stream) = 0;
    
    /**
     * @brief Read a string from a stream asynchronously
     * 
     * @param stream Input stream to read from
     * @return Future containing the decoded string
     */
    virtual std::future<std::string> read_string_async(std::istream& stream) = 0;
    
    /**
     * @brief Read an integer from a stream
     * 
     * @param stream Input stream to read from
     * @return Decoded integer
     * @throws std::runtime_error if the data is not a valid bencode integer
     */
    virtual int64_t read_integer(std::istream& stream) = 0;
    
    /**
     * @brief Read an integer from a stream asynchronously
     * 
     * @param stream Input stream to read from
     * @return Future containing the decoded integer
     */
    virtual std::future<int64_t> read_integer_async(std::istream& stream) = 0;
    
    /**
     * @brief Read a list from a stream
     * 
     * @param stream Input stream to read from
     * @return Decoded list
     * @throws std::runtime_error if the data is not a valid bencode list
     */
    virtual std::vector<BencodeValue> read_list(std::istream& stream) = 0;
    
    /**
     * @brief Read a list from a stream asynchronously
     * 
     * @param stream Input stream to read from
     * @return Future containing the decoded list
     */
    virtual std::future<std::vector<BencodeValue>> read_list_async(std::istream& stream) = 0;
    
    /**
     * @brief Read a dictionary from a stream
     * 
     * @param stream Input stream to read from
     * @return Decoded dictionary
     * @throws std::runtime_error if the data is not a valid bencode dictionary
     */
    virtual std::map<std::string, BencodeValue> read_dict(std::istream& stream) = 0;
    
    /**
     * @brief Read a dictionary from a stream asynchronously
     * 
     * @param stream Input stream to read from
     * @return Future containing the decoded dictionary
     */
    virtual std::future<std::map<std::string, BencodeValue>> read_dict_async(std::istream& stream) = 0;
};

/**
 * @brief Create a new bencode stream
 * 
 * @return Unique pointer to a new bencode stream
 */
std::unique_ptr<BencodeStream> create_bencode_stream();

} // namespace bitscrape::bencode
