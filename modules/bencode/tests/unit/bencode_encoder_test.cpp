#include <bitscrape/testing.hpp>

#include "bitscrape/bencode/bencode_encoder.hpp"
#include "bitscrape/bencode/bencode_value.hpp"

using namespace bitscrape::bencode;

TEST(BencodeEncoderTest, EncodeString) {
    auto encoder = create_bencode_encoder();
    
    std::string value = "test";
    auto encoded = encoder->encode_string(value);
    
    std::string expected = "4:test";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeStringAsync) {
    auto encoder = create_bencode_encoder();
    
    std::string value = "test";
    auto future = encoder->encode_string_async(value);
    auto encoded = future.get();
    
    std::string expected = "4:test";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeInteger) {
    auto encoder = create_bencode_encoder();
    
    int64_t value = 42;
    auto encoded = encoder->encode_integer(value);
    
    std::string expected = "i42e";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeIntegerAsync) {
    auto encoder = create_bencode_encoder();
    
    int64_t value = 42;
    auto future = encoder->encode_integer_async(value);
    auto encoded = future.get();
    
    std::string expected = "i42e";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeList) {
    auto encoder = create_bencode_encoder();
    
    std::vector<BencodeValue> value = {
        BencodeValue(std::string("test")),
        BencodeValue(42)
    };
    
    auto encoded = encoder->encode_list(value);
    
    std::string expected = "l4:testi42ee";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeListAsync) {
    auto encoder = create_bencode_encoder();
    
    std::vector<BencodeValue> value = {
        BencodeValue(std::string("test")),
        BencodeValue(42)
    };
    
    auto future = encoder->encode_list_async(value);
    auto encoded = future.get();
    
    std::string expected = "l4:testi42ee";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeDict) {
    auto encoder = create_bencode_encoder();
    
    std::map<std::string, BencodeValue> value = {
        {"string", BencodeValue(std::string("test"))},
        {"integer", BencodeValue(42)}
    };
    
    auto encoded = encoder->encode_dict(value);
    
    // Note: The order of keys in a map is sorted, so "integer" comes before "string"
    std::string expected = "d7:integeri42e6:string4:teste";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeDictAsync) {
    auto encoder = create_bencode_encoder();
    
    std::map<std::string, BencodeValue> value = {
        {"string", BencodeValue(std::string("test"))},
        {"integer", BencodeValue(42)}
    };
    
    auto future = encoder->encode_dict_async(value);
    auto encoded = future.get();
    
    // Note: The order of keys in a map is sorted, so "integer" comes before "string"
    std::string expected = "d7:integeri42e6:string4:teste";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeStringValue) {
    auto encoder = create_bencode_encoder();
    
    BencodeValue value(std::string("test"));
    auto encoded = encoder->encode(value);
    
    std::string expected = "4:test";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeIntegerValue) {
    auto encoder = create_bencode_encoder();
    
    BencodeValue value(42);
    auto encoded = encoder->encode(value);
    
    std::string expected = "i42e";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeListValue) {
    auto encoder = create_bencode_encoder();
    
    BencodeValue value(std::vector<BencodeValue>{
        BencodeValue(std::string("test")),
        BencodeValue(42)
    });
    
    auto encoded = encoder->encode(value);
    
    std::string expected = "l4:testi42ee";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeDictValue) {
    auto encoder = create_bencode_encoder();
    
    BencodeValue value(std::map<std::string, BencodeValue>{
        {"string", BencodeValue(std::string("test"))},
        {"integer", BencodeValue(42)}
    });
    
    auto encoded = encoder->encode(value);
    
    // Note: The order of keys in a map is sorted, so "integer" comes before "string"
    std::string expected = "d7:integeri42e6:string4:teste";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, EncodeValueAsync) {
    auto encoder = create_bencode_encoder();
    
    BencodeValue value(std::map<std::string, BencodeValue>{
        {"string", BencodeValue(std::string("test"))},
        {"integer", BencodeValue(42)}
    });
    
    auto future = encoder->encode_async(value);
    auto encoded = future.get();
    
    // Note: The order of keys in a map is sorted, so "integer" comes before "string"
    std::string expected = "d7:integeri42e6:string4:teste";
    std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());
    
    EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEncoderTest, CreateBencodeEncoder) {
    auto encoder = create_bencode_encoder();
    
    EXPECT_NE(encoder, nullptr);
}
