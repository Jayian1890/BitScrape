#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "bitscrape/types/node_id.hpp"

using namespace bitscrape::types;

TEST_CASE("NodeID construction", "[types][node_id]") {
    SECTION("Default constructor creates a random NodeID") {
        NodeID id1;
        NodeID id2;
        
        REQUIRE(id1 != id2);
    }
    
    SECTION("Construction from byte array") {
        NodeID::IDStorage bytes = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
            0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14
        };
        
        NodeID id(bytes);
        
        REQUIRE(id.bytes() == bytes);
    }
    
    SECTION("Construction from byte vector") {
        std::vector<uint8_t> bytes = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
            0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14
        };
        
        NodeID id(bytes);
        
        REQUIRE(std::equal(id.bytes().begin(), id.bytes().end(), bytes.begin()));
    }
    
    SECTION("Construction from hex string") {
        std::string hex = "0102030405060708090a0b0c0d0e0f1011121314";
        
        NodeID id(hex);
        
        REQUIRE(id.to_hex() == hex);
    }
    
    SECTION("Construction from string_view") {
        std::string_view hex = "0102030405060708090a0b0c0d0e0f1011121314";
        
        NodeID id(hex);
        
        REQUIRE(id.to_hex() == hex);
    }
    
    SECTION("Invalid byte vector size") {
        std::vector<uint8_t> bytes = {0x01, 0x02, 0x03};
        
        REQUIRE_THROWS_AS(NodeID(bytes), std::invalid_argument);
    }
    
    SECTION("Invalid hex string length") {
        std::string hex = "0102";
        
        REQUIRE_THROWS_AS(NodeID(hex), std::invalid_argument);
    }
    
    SECTION("Invalid hex string characters") {
        std::string hex = "01020304050607080g0a0b0c0d0e0f1011121314";
        
        REQUIRE_THROWS_AS(NodeID(hex), std::invalid_argument);
    }
}

TEST_CASE("NodeID operations", "[types][node_id]") {
    NodeID::IDStorage bytes1 = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
        0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14
    };
    
    NodeID::IDStorage bytes2 = {
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
        0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24
    };
    
    NodeID id1(bytes1);
    NodeID id2(bytes2);
    
    SECTION("to_hex") {
        REQUIRE(id1.to_hex() == "0102030405060708090a0b0c0d0e0f1011121314");
        REQUIRE(id2.to_hex() == "1112131415161718191a1b1c1d1e1f2021222324");
    }
    
    SECTION("distance") {
        NodeID dist = id1.distance(id2);
        
        NodeID::IDStorage expected = {
            0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
            0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0x30, 0x30, 0x30, 0x30
        };
        
        REQUIRE(dist.bytes() == expected);
    }
    
    SECTION("distance_async") {
        auto future = id1.distance_async(id2);
        NodeID dist = future.get();
        
        NodeID::IDStorage expected = {
            0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
            0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0x30, 0x30, 0x30, 0x30
        };
        
        REQUIRE(dist.bytes() == expected);
    }
    
    SECTION("random") {
        NodeID random1 = NodeID::random();
        NodeID random2 = NodeID::random();
        
        REQUIRE(random1 != random2);
    }
    
    SECTION("random_async") {
        auto future = NodeID::random_async();
        NodeID random = future.get();
        
        REQUIRE(random != id1);
        REQUIRE(random != id2);
    }
    
    SECTION("comparison operators") {
        REQUIRE(id1 == id1);
        REQUIRE(id1 != id2);
        REQUIRE(id1 < id2);
        REQUIRE(id2 > id1);
        REQUIRE(id1 <= id1);
        REQUIRE(id1 <= id2);
        REQUIRE(id2 >= id2);
        REQUIRE(id2 >= id1);
    }
}
