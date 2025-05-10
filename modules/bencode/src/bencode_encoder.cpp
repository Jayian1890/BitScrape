#include "bitscrape/bencode/bencode_encoder.hpp"

#include <algorithm>
#include <future>
#include <sstream>
#include <stdexcept>

namespace bitscrape::bencode {

/**
 * @brief Implementation of the bencode encoder
 */
class BencodeEncoderImpl : public BencodeEncoder {
public:
    /**
     * @brief Constructor
     */
    BencodeEncoderImpl() = default;
    
    /**
     * @brief Destructor
     */
    ~BencodeEncoderImpl() override = default;
    
    /**
     * @brief Encode a BencodeValue to bencode format
     * 
     * @param value BencodeValue to encode
     * @return Encoded data as a byte vector
     */
    std::vector<uint8_t> encode(const BencodeValue& value) override {
        switch (value.type()) {
            case BencodeValue::Type::STRING:
                return encode_string(value.as_string());
            case BencodeValue::Type::INTEGER:
                return encode_integer(value.as_integer());
            case BencodeValue::Type::LIST:
                return encode_list(value.as_list());
            case BencodeValue::Type::DICT:
                return encode_dict(value.as_dict());
            default:
                throw std::runtime_error("BencodeEncoder: Invalid type");
        }
    }
    
    /**
     * @brief Encode a BencodeValue to bencode format asynchronously
     * 
     * @param value BencodeValue to encode
     * @return Future containing the encoded data as a byte vector
     */
    std::future<std::vector<uint8_t>> encode_async(const BencodeValue& value) override {
        return std::async(std::launch::async, [this, value]() {
            return this->encode(value);
        });
    }
    
    /**
     * @brief Encode a string to bencode format
     * 
     * @param value String to encode
     * @return Encoded data as a byte vector
     */
    std::vector<uint8_t> encode_string(const std::string& value) override {
        // Format: <length>:<string>
        std::string encoded = std::to_string(value.size()) + ":" + value;
        return std::vector<uint8_t>(encoded.begin(), encoded.end());
    }
    
    /**
     * @brief Encode a string to bencode format asynchronously
     * 
     * @param value String to encode
     * @return Future containing the encoded data as a byte vector
     */
    std::future<std::vector<uint8_t>> encode_string_async(const std::string& value) override {
        return std::async(std::launch::async, [this, value]() {
            return this->encode_string(value);
        });
    }
    
    /**
     * @brief Encode an integer to bencode format
     * 
     * @param value Integer to encode
     * @return Encoded data as a byte vector
     */
    std::vector<uint8_t> encode_integer(int64_t value) override {
        // Format: i<integer>e
        std::string encoded = "i" + std::to_string(value) + "e";
        return std::vector<uint8_t>(encoded.begin(), encoded.end());
    }
    
    /**
     * @brief Encode an integer to bencode format asynchronously
     * 
     * @param value Integer to encode
     * @return Future containing the encoded data as a byte vector
     */
    std::future<std::vector<uint8_t>> encode_integer_async(int64_t value) override {
        return std::async(std::launch::async, [this, value]() {
            return this->encode_integer(value);
        });
    }
    
    /**
     * @brief Encode a list to bencode format
     * 
     * @param value List to encode
     * @return Encoded data as a byte vector
     */
    std::vector<uint8_t> encode_list(const std::vector<BencodeValue>& value) override {
        // Format: l<item1><item2>...e
        std::vector<uint8_t> encoded;
        encoded.push_back('l');
        
        for (const auto& item : value) {
            auto item_encoded = encode(item);
            encoded.insert(encoded.end(), item_encoded.begin(), item_encoded.end());
        }
        
        encoded.push_back('e');
        return encoded;
    }
    
    /**
     * @brief Encode a list to bencode format asynchronously
     * 
     * @param value List to encode
     * @return Future containing the encoded data as a byte vector
     */
    std::future<std::vector<uint8_t>> encode_list_async(const std::vector<BencodeValue>& value) override {
        return std::async(std::launch::async, [this, value]() {
            return this->encode_list(value);
        });
    }
    
    /**
     * @brief Encode a dictionary to bencode format
     * 
     * @param value Dictionary to encode
     * @return Encoded data as a byte vector
     */
    std::vector<uint8_t> encode_dict(const std::map<std::string, BencodeValue>& value) override {
        // Format: d<key1><value1><key2><value2>...e
        std::vector<uint8_t> encoded;
        encoded.push_back('d');
        
        for (const auto& [key, val] : value) {
            auto key_encoded = encode_string(key);
            encoded.insert(encoded.end(), key_encoded.begin(), key_encoded.end());
            
            auto val_encoded = encode(val);
            encoded.insert(encoded.end(), val_encoded.begin(), val_encoded.end());
        }
        
        encoded.push_back('e');
        return encoded;
    }
    
    /**
     * @brief Encode a dictionary to bencode format asynchronously
     * 
     * @param value Dictionary to encode
     * @return Future containing the encoded data as a byte vector
     */
    std::future<std::vector<uint8_t>> encode_dict_async(const std::map<std::string, BencodeValue>& value) override {
        return std::async(std::launch::async, [this, value]() {
            return this->encode_dict(value);
        });
    }
};

std::unique_ptr<BencodeEncoder> create_bencode_encoder() {
    return std::make_unique<BencodeEncoderImpl>();
}

} // namespace bitscrape::bencode
