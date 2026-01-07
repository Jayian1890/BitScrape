#include <bitscrape/testing.hpp>

#include "bitscrape/bencode/bencode_decoder.hpp"
#include "bitscrape/bencode/bencode_value.hpp"

using namespace bitscrape::bencode;

TEST(BencodeDecoderTest, DecodeString) {
  auto decoder = create_bencode_decoder();

  std::string data = "4:test";
  auto decoded = decoder->decode(data);

  EXPECT_TRUE(decoded.is_string());
  EXPECT_EQ(decoded.as_string(), "test");
}

TEST(BencodeDecoderTest, DecodeStringAsync) {
  auto decoder = create_bencode_decoder();

  std::string data = "4:test";
  auto future = decoder->decode_async(data);
  auto decoded = future.get();

  EXPECT_TRUE(decoded.is_string());
  EXPECT_EQ(decoded.as_string(), "test");
}

TEST(BencodeDecoderTest, DecodeInteger) {
  auto decoder = create_bencode_decoder();

  std::string data = "i42e";
  auto decoded = decoder->decode(data);

  EXPECT_TRUE(decoded.is_integer());
  EXPECT_EQ(decoded.as_integer(), 42);
}

TEST(BencodeDecoderTest, DecodeIntegerAsync) {
  auto decoder = create_bencode_decoder();

  std::string data = "i42e";
  auto future = decoder->decode_async(data);
  auto decoded = future.get();

  EXPECT_TRUE(decoded.is_integer());
  EXPECT_EQ(decoded.as_integer(), 42);
}

TEST(BencodeDecoderTest, DecodeList) {
  auto decoder = create_bencode_decoder();

  std::string data = "l4:testi42ee";
  auto decoded = decoder->decode(data);

  EXPECT_TRUE(decoded.is_list());
  EXPECT_EQ(decoded.as_list().size(), 2UL);
  EXPECT_TRUE(decoded.as_list()[0].is_string());
  EXPECT_EQ(decoded.as_list()[0].as_string(), "test");
  EXPECT_TRUE(decoded.as_list()[1].is_integer());
  EXPECT_EQ(decoded.as_list()[1].as_integer(), 42);
}

TEST(BencodeDecoderTest, DecodeListAsync) {
  auto decoder = create_bencode_decoder();

  std::string data = "l4:testi42ee";
  auto future = decoder->decode_async(data);
  auto decoded = future.get();

  EXPECT_TRUE(decoded.is_list());
  EXPECT_EQ(decoded.as_list().size(), 2UL);
  EXPECT_TRUE(decoded.as_list()[0].is_string());
  EXPECT_EQ(decoded.as_list()[0].as_string(), "test");
  EXPECT_TRUE(decoded.as_list()[1].is_integer());
  EXPECT_EQ(decoded.as_list()[1].as_integer(), 42);
}

TEST(BencodeDecoderTest, DecodeDict) {
  auto decoder = create_bencode_decoder();

  std::string data = "d6:string4:test7:integeri42ee";
  auto decoded = decoder->decode(data);

  EXPECT_TRUE(decoded.is_dict());
  EXPECT_EQ(decoded.as_dict().size(), 2UL);
  EXPECT_TRUE(decoded.as_dict().at("string").is_string());
  EXPECT_EQ(decoded.as_dict().at("string").as_string(), "test");
  EXPECT_TRUE(decoded.as_dict().at("integer").is_integer());
  EXPECT_EQ(decoded.as_dict().at("integer").as_integer(), 42);
}

TEST(BencodeDecoderTest, DecodeDictAsync) {
  auto decoder = create_bencode_decoder();

  std::string data = "d6:string4:test7:integeri42ee";
  auto future = decoder->decode_async(data);
  auto decoded = future.get();

  EXPECT_TRUE(decoded.is_dict());
  EXPECT_EQ(decoded.as_dict().size(), 2UL);
  EXPECT_TRUE(decoded.as_dict().at("string").is_string());
  EXPECT_EQ(decoded.as_dict().at("string").as_string(), "test");
  EXPECT_TRUE(decoded.as_dict().at("integer").is_integer());
  EXPECT_EQ(decoded.as_dict().at("integer").as_integer(), 42);
}

TEST(BencodeDecoderTest, DecodeNestedStructures) {
  auto decoder = create_bencode_decoder();

  std::string data = "d4:listl4:testi42ee4:dictd6:string4:test7:integeri42eee";
  auto decoded = decoder->decode(data);

  EXPECT_TRUE(decoded.is_dict());
  EXPECT_EQ(decoded.as_dict().size(), 2UL);

  // Check the list
  EXPECT_TRUE(decoded.as_dict().at("list").is_list());
  EXPECT_EQ(decoded.as_dict().at("list").as_list().size(), 2UL);
  EXPECT_TRUE(decoded.as_dict().at("list").as_list()[0].is_string());
  EXPECT_EQ(decoded.as_dict().at("list").as_list()[0].as_string(), "test");
  EXPECT_TRUE(decoded.as_dict().at("list").as_list()[1].is_integer());
  EXPECT_EQ(decoded.as_dict().at("list").as_list()[1].as_integer(), 42);

  // Check the dict
  EXPECT_TRUE(decoded.as_dict().at("dict").is_dict());
  EXPECT_EQ(decoded.as_dict().at("dict").as_dict().size(), 2UL);
  EXPECT_TRUE(decoded.as_dict().at("dict").as_dict().at("string").is_string());
  EXPECT_EQ(decoded.as_dict().at("dict").as_dict().at("string").as_string(),
            "test");
  EXPECT_TRUE(
      decoded.as_dict().at("dict").as_dict().at("integer").is_integer());
  EXPECT_EQ(decoded.as_dict().at("dict").as_dict().at("integer").as_integer(),
            42);
}

TEST(BencodeDecoderTest, DecodeEmptyString) {
  auto decoder = create_bencode_decoder();

  EXPECT_THROW(decoder->decode(std::string("")), std::runtime_error);
}

TEST(BencodeDecoderTest, DecodeInvalidString) {
  auto decoder = create_bencode_decoder();

  EXPECT_THROW(decoder->decode(std::string("invalid")), std::runtime_error);
}

TEST(BencodeDecoderTest, DecodeInvalidInteger) {
  auto decoder = create_bencode_decoder();

  EXPECT_THROW(decoder->decode(std::string("i42")), std::runtime_error);
}

TEST(BencodeDecoderTest, DecodeInvalidList) {
  auto decoder = create_bencode_decoder();

  EXPECT_THROW(decoder->decode(std::string("l4:test")), std::runtime_error);
}

TEST(BencodeDecoderTest, DecodeInvalidDict) {
  auto decoder = create_bencode_decoder();

  EXPECT_THROW(decoder->decode(std::string("d6:string4:test")),
               std::runtime_error);
}

TEST(BencodeDecoderTest, DecodeFromByteVector) {
  auto decoder = create_bencode_decoder();

  std::string data = "4:test";
  std::vector<uint8_t> bytes(data.begin(), data.end());
  auto decoded = decoder->decode(bytes);

  EXPECT_TRUE(decoded.is_string());
  EXPECT_EQ(decoded.as_string(), "test");
}

TEST(BencodeDecoderTest, DecodeFromByteVectorAsync) {
  auto decoder = create_bencode_decoder();

  std::string data = "4:test";
  std::vector<uint8_t> bytes(data.begin(), data.end());
  auto future = decoder->decode_async(bytes);
  auto decoded = future.get();

  EXPECT_TRUE(decoded.is_string());
  EXPECT_EQ(decoded.as_string(), "test");
}

TEST(BencodeDecoderTest, DecodeFromStringView) {
  auto decoder = create_bencode_decoder();

  std::string_view data = "4:test";
  auto decoded = decoder->decode(data);

  EXPECT_TRUE(decoded.is_string());
  EXPECT_EQ(decoded.as_string(), "test");
}

TEST(BencodeDecoderTest, DecodeFromStringViewAsync) {
  auto decoder = create_bencode_decoder();

  std::string_view data = "4:test";
  auto future = decoder->decode_async(data);
  auto decoded = future.get();

  EXPECT_TRUE(decoded.is_string());
  EXPECT_EQ(decoded.as_string(), "test");
}

TEST(BencodeDecoderTest, DecodeFromBytePointer) {
  auto decoder = create_bencode_decoder();

  std::string data = "4:test";
  auto decoded = decoder->decode(reinterpret_cast<const uint8_t *>(data.data()),
                                 data.size());

  EXPECT_TRUE(decoded.is_string());
  EXPECT_EQ(decoded.as_string(), "test");
}

TEST(BencodeDecoderTest, DecodeFromBytePointerAsync) {
  auto decoder = create_bencode_decoder();

  std::string data = "4:test";
  auto future = decoder->decode_async(
      reinterpret_cast<const uint8_t *>(data.data()), data.size());
  auto decoded = future.get();

  EXPECT_TRUE(decoded.is_string());
  EXPECT_EQ(decoded.as_string(), "test");
}

TEST(BencodeDecoderTest, CreateBencodeDecoder) {
  auto decoder = create_bencode_decoder();

  EXPECT_NE(decoder, nullptr);
}
