#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace bitscrape::web {

/**
 * @brief SHA-1 hash implementation using only the C++ standard library
 */
class SHA1 {
public:
    /**
     * @brief Size of a SHA-1 hash in bytes (160 bits = 20 bytes)
     */
    static constexpr size_t DIGEST_LENGTH = 20;

    /**
     * @brief Compute the SHA-1 hash of a string
     * 
     * @param input Input string
     * @param output Output buffer (must be at least DIGEST_LENGTH bytes)
     */
    static void compute(const std::string& input, unsigned char* output) {
        compute(reinterpret_cast<const unsigned char*>(input.data()), input.size(), output);
    }

    /**
     * @brief Compute the SHA-1 hash of a byte array
     * 
     * @param input Input data
     * @param length Length of input data in bytes
     * @param output Output buffer (must be at least DIGEST_LENGTH bytes)
     */
    static void compute(const unsigned char* input, size_t length, unsigned char* output) {
        SHA1 sha1;
        sha1.update(input, length);
        sha1.finalize();
        
        for (size_t i = 0; i < DIGEST_LENGTH; ++i) {
            output[i] = static_cast<unsigned char>((sha1.state_[i / 4] >> ((3 - (i % 4)) * 8)) & 0xFF);
        }
    }

    /**
     * @brief Compute the SHA-1 hash of a string and return it as a byte array
     * 
     * @param input Input string
     * @return SHA-1 hash as a byte array
     */
    static std::array<unsigned char, DIGEST_LENGTH> compute(const std::string& input) {
        std::array<unsigned char, DIGEST_LENGTH> output;
        compute(input, output.data());
        return output;
    }

    /**
     * @brief Compute the SHA-1 hash of a byte array and return it as a byte array
     * 
     * @param input Input data
     * @param length Length of input data in bytes
     * @return SHA-1 hash as a byte array
     */
    static std::array<unsigned char, DIGEST_LENGTH> compute(const unsigned char* input, size_t length) {
        std::array<unsigned char, DIGEST_LENGTH> output;
        compute(input, length, output.data());
        return output;
    }

private:
    /**
     * @brief Default constructor
     */
    SHA1() {
        reset();
    }

    /**
     * @brief Reset the hash state
     */
    void reset() {
        // Initialize hash values (FIPS 180-1)
        state_[0] = 0x67452301;
        state_[1] = 0xEFCDAB89;
        state_[2] = 0x98BADCFE;
        state_[3] = 0x10325476;
        state_[4] = 0xC3D2E1F0;
        
        count_ = 0;
        buffer_index_ = 0;
    }

    /**
     * @brief Update the hash with more data
     * 
     * @param data Input data
     * @param length Length of input data in bytes
     */
    void update(const unsigned char* data, size_t length) {
        // Update count
        size_t old_count = count_;
        count_ += length * 8; // Count in bits
        
        // Handle overflow
        if (count_ < old_count) {
            // This would only happen after 2^64 bits, which is unlikely
        }
        
        // Process data in chunks
        while (length > 0) {
            size_t copy_length = std::min(length, 64 - buffer_index_);
            std::copy(data, data + copy_length, buffer_.begin() + buffer_index_);
            
            buffer_index_ += copy_length;
            data += copy_length;
            length -= copy_length;
            
            if (buffer_index_ == 64) {
                transform();
                buffer_index_ = 0;
            }
        }
    }

    /**
     * @brief Finalize the hash computation
     */
    void finalize() {
        // Add padding
        unsigned char padding[64];
        std::fill(padding, padding + 64, 0);
        padding[0] = 0x80; // First bit is 1, rest are 0
        
        // Append length
        uint64_t bits = count_;
        for (int i = 0; i < 8; ++i) {
            padding[56 + i] = static_cast<unsigned char>((bits >> ((7 - i) * 8)) & 0xFF);
        }
        
        // Process padding
        if (buffer_index_ < 56) {
            // One block is enough
            update(padding, 56 - buffer_index_);
        } else {
            // Need two blocks
            update(padding, 64 - buffer_index_);
            update(padding + 64 - buffer_index_, 56);
        }
        
        // Append length
        update(padding + 56, 8);
    }

    /**
     * @brief Transform a 64-byte block
     */
    void transform() {
        // Convert bytes to words
        std::array<uint32_t, 80> words;
        for (int i = 0; i < 16; ++i) {
            words[i] = (static_cast<uint32_t>(buffer_[i * 4]) << 24) |
                       (static_cast<uint32_t>(buffer_[i * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(buffer_[i * 4 + 2]) << 8) |
                       static_cast<uint32_t>(buffer_[i * 4 + 3]);
        }
        
        // Extend the sixteen 32-bit words into eighty 32-bit words
        for (int i = 16; i < 80; ++i) {
            words[i] = rotate_left(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
        }
        
        // Initialize hash value for this chunk
        uint32_t a = state_[0];
        uint32_t b = state_[1];
        uint32_t c = state_[2];
        uint32_t d = state_[3];
        uint32_t e = state_[4];
        
        // Main loop
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            
            uint32_t temp = rotate_left(a, 5) + f + e + k + words[i];
            e = d;
            d = c;
            c = rotate_left(b, 30);
            b = a;
            a = temp;
        }
        
        // Add this chunk's hash to result so far
        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
    }

    /**
     * @brief Rotate a 32-bit value left by n bits
     */
    static uint32_t rotate_left(uint32_t value, int bits) {
        return (value << bits) | (value >> (32 - bits));
    }

    std::array<uint32_t, 5> state_; ///< Hash state
    std::array<unsigned char, 64> buffer_; ///< Input buffer
    size_t buffer_index_; ///< Current position in buffer
    uint64_t count_; ///< Number of bits processed
};

} // namespace bitscrape::web
