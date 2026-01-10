#pragma once

#include <string>
#include <vector>

namespace bitscrape::web {

/**
 * @brief Base64 encoding/decoding implementation using only the C++ standard library
 */
class Base64 {
public:
    /**
     * @brief Encode binary data to Base64
     * 
     * @param data Input data
     * @param length Length of input data in bytes
     * @return Base64-encoded string
     */
    static std::string encode(const unsigned char* data, size_t length) {
        static const char base64_chars[] = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        
        std::string result;
        result.reserve(((length + 2) / 3) * 4); // Estimate size
        
        for (size_t i = 0; i < length; i += 3) {
            // Get the next three bytes (or fewer if at the end)
            unsigned char b1 = data[i];
            unsigned char b2 = (i + 1 < length) ? data[i + 1] : 0;
            unsigned char b3 = (i + 2 < length) ? data[i + 2] : 0;
            
            // Encode to four Base64 characters
            result.push_back(base64_chars[b1 >> 2]);
            result.push_back(base64_chars[((b1 & 0x03) << 4) | ((b2 & 0xf0) >> 4)]);
            result.push_back((i + 1 < length) ? base64_chars[((b2 & 0x0f) << 2) | ((b3 & 0xc0) >> 6)] : '=');
            result.push_back((i + 2 < length) ? base64_chars[b3 & 0x3f] : '=');
        }
        
        return result;
    }

    /**
     * @brief Encode a string to Base64
     * 
     * @param input Input string
     * @return Base64-encoded string
     */
    static std::string encode(const std::string& input) {
        return encode(reinterpret_cast<const unsigned char*>(input.data()), input.size());
    }

    /**
     * @brief Decode Base64 to binary data
     * 
     * @param input Base64-encoded string
     * @return Decoded binary data
     */
    static std::vector<unsigned char> decode(const std::string& input) {
        static const unsigned char base64_index[256] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0,
            0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0,
            0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };
        
        // Remove padding characters
        size_t padding = 0;
        if (!input.empty() && input.back() == '=') {
            padding++;
            if (input.size() > 1 && input[input.size() - 2] == '=') {
                padding++;
            }
        }
        
        // Calculate output size
        size_t output_length = (input.size() * 3) / 4 - padding;
        std::vector<unsigned char> result(output_length);
        
        // Decode
        size_t i = 0, j = 0;
        while (i < input.size()) {
            // Skip non-Base64 characters
            if (input[i] == '=') {
                break;
            }
            
            // Get four Base64 characters
            unsigned char b1 = base64_index[static_cast<unsigned char>(input[i++])];
            unsigned char b2 = (i < input.size()) ? base64_index[static_cast<unsigned char>(input[i++])] : 0;
            unsigned char b3 = (i < input.size()) ? base64_index[static_cast<unsigned char>(input[i++])] : 0;
            unsigned char b4 = (i < input.size()) ? base64_index[static_cast<unsigned char>(input[i++])] : 0;
            
            // Decode to three bytes
            if (j < output_length) {
                result[j++] = (b1 << 2) | (b2 >> 4);
            }
            if (j < output_length) {
                result[j++] = ((b2 & 0x0f) << 4) | (b3 >> 2);
            }
            if (j < output_length) {
                result[j++] = ((b3 & 0x03) << 6) | b4;
            }
        }
        
        return result;
    }
};

} // namespace bitscrape::web
