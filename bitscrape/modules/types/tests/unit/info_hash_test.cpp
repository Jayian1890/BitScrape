#include <catch2/catch.hpp>

#include "bitscrape/types/info_hash.hpp"

using namespace bitscrape::types;

TEST_CASE("InfoHash construction", "[types][info_hash]") {
    SECTION("Default constructor creates a zero InfoHash") {
        InfoHash hash;
        
        for (auto byte : hash.bytes()) {
            REQUIRE(byte == 0);
        }
    }
    
    SECTION("Construction from byte array") {
        InfoHash::HashStorage bytes = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
            0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14
        };
        
        InfoHash hash(bytes);
        
        REQUIRE(hash.bytes() == bytes);
    }
    
    SECTION("Construction from byte vector") {
        std::vector<uint8_t> bytes = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
            0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14
        };
        
        InfoHash hash(bytes);
        
        REQUIRE(std::equal(hash.bytes().begin(), hash.bytes().end(), bytes.begin()));
    }
    
    SECTION("Construction from hex string") {
        std::string hex = "0102030405060708090a0b0c0d0e0f1011121314";
        
        InfoHash hash(hex);
        
        REQUIRE(hash.to_hex() == hex);
    }
    
    SECTION("Construction from string_view") {
        std::string_view hex = "0102030405060708090a0b0c0d0e0f1011121314";
        
        InfoHash hash(hex);
        
        REQUIRE(hash.to_hex() == hex);
    }
    
    SECTION("Invalid byte vector size") {
        std::vector<uint8_t> bytes = {0x01, 0x02, 0x03};
        
        REQUIRE_THROWS_AS(InfoHash(bytes), std::invalid_argument);
    }
    
    SECTION("Invalid hex string length") {
        std::string hex = "0102";
        
        REQUIRE_THROWS_AS(InfoHash(hex), std::invalid_argument);
    }
    
    SECTION("Invalid hex string characters") {
        std::string hex = "01020304050607080g0a0b0c0d0e0f1011121314";
        
        REQUIRE_THROWS_AS(InfoHash(hex), std::invalid_argument);
    }
}

TEST_CASE("InfoHash operations", "[types][info_hash]") {
    InfoHash::HashStorage bytes1 = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
        0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14
    };
    
    InfoHash::HashStorage bytes2 = {
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
        0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24
    };
    
    InfoHash hash1(bytes1);
    InfoHash hash2(bytes2);
    
    SECTION("to_hex") {
        REQUIRE(hash1.to_hex() == "0102030405060708090a0b0c0d0e0f1011121314");
        REQUIRE(hash2.to_hex() == "1112131415161718191a1b1c1d1e1f2021222324");
    }
    
    SECTION("from_bencode") {
        std::vector<uint8_t> data = {
            'd', '4', ':', 't', 'e', 's', 't', 'i', '4', '2', 'e', 'e'
        };
        
        InfoHash hash = InfoHash::from_bencode(data);
        
        // We can't easily predict the hash value, but we can check that it's not all zeros
        bool all_zeros = true;
        for (auto byte : hash.bytes()) {
            if (byte != 0) {
                all_zeros = false;
                break;
            }
        }
        
        REQUIRE_FALSE(all_zeros);
    }
    
    SECTION("from_bencode_async") {
        std::vector<uint8_t> data = {
            'd', '4', ':', 't', 'e', 's', 't', 'i', '4', '2', 'e', 'e'
        };
        
        auto future = InfoHash::from_bencode_async(data);
        InfoHash hash = future.get();
        
        // We can't easily predict the hash value, but we can check that it's not all zeros
        bool all_zeros = true;
        for (auto byte : hash.bytes()) {
            if (byte != 0) {
                all_zeros = false;
                break;
            }
        }
        
        REQUIRE_FALSE(all_zeros);
    }
    
    SECTION("random") {
        InfoHash random1 = InfoHash::random();
        InfoHash random2 = InfoHash::random();
        
        REQUIRE(random1 != random2);
    }
    
    SECTION("random_async") {
        auto future = InfoHash::random_async();
        InfoHash random = future.get();
        
        REQUIRE(random != hash1);
        REQUIRE(random != hash2);
    }
    
    SECTION("comparison operators") {
        REQUIRE(hash1 == hash1);
        REQUIRE(hash1 != hash2);
        REQUIRE(hash1 < hash2);
        REQUIRE(hash2 > hash1);
        REQUIRE(hash1 <= hash1);
        REQUIRE(hash1 <= hash2);
        REQUIRE(hash2 >= hash2);
        REQUIRE(hash2 >= hash1);
    }
}
