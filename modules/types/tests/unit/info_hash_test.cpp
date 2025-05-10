#include <gtest/gtest.h>

#include "bitscrape/types/info_hash.hpp"

using namespace bitscrape::types;

TEST(InfoHashTest, DefaultConstructorCreatesZeroInfoHash) {
  InfoHash hash;

  for (auto byte : hash.bytes()) {
    EXPECT_EQ(byte, 0);
  }
}

TEST(InfoHashTest, ConstructionFromByteArray) {
  InfoHash::HashStorage bytes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                                 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};

  InfoHash hash(bytes);

  EXPECT_EQ(hash.bytes(), bytes);
}

TEST(InfoHashTest, ConstructionFromByteVector) {
  std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                                0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};

  InfoHash hash(bytes);

  EXPECT_TRUE(
      std::equal(hash.bytes().begin(), hash.bytes().end(), bytes.begin()));
}

TEST(InfoHashTest, ConstructionFromHexString) {
  std::string hex = "0102030405060708090a0b0c0d0e0f1011121314";

  InfoHash hash(hex);

  EXPECT_EQ(hash.to_hex(), hex);
}

TEST(InfoHashTest, ConstructionFromStringView) {
  std::string_view hex = "0102030405060708090a0b0c0d0e0f1011121314";

  InfoHash hash(hex);

  EXPECT_EQ(hash.to_hex(), hex);
}

TEST(InfoHashTest, InvalidByteVectorSize) {
  std::vector<uint8_t> bytes = {0x01, 0x02, 0x03};

  EXPECT_THROW({ InfoHash hash(bytes); }, std::invalid_argument);
}

TEST(InfoHashTest, InvalidHexStringLength) {
  std::string hex = "0102";

  EXPECT_THROW({ InfoHash hash(hex); }, std::invalid_argument);
}

TEST(InfoHashTest, InvalidHexStringCharacters) {
  // Use a string with an invalid hex character 'g'
  std::string hex = "0102030405060708090a0b0c0d0e0f10111213g4";

  EXPECT_THROW({ InfoHash hash(hex); }, std::invalid_argument);
}

TEST(InfoHashTest, ToHex) {
  InfoHash::HashStorage bytes1 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                                  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};

  InfoHash::HashStorage bytes2 = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E,
                                  0x1F, 0x20, 0x21, 0x22, 0x23, 0x24};

  InfoHash hash1(bytes1);
  InfoHash hash2(bytes2);

  EXPECT_EQ(hash1.to_hex(), "0102030405060708090a0b0c0d0e0f1011121314");
  EXPECT_EQ(hash2.to_hex(), "1112131415161718191a1b1c1d1e1f2021222324");
}

TEST(InfoHashTest, FromBencode) {
  std::vector<uint8_t> data = {'d', '4', ':', 't', 'e', 's',
                               't', 'i', '4', '2', 'e', 'e'};

  InfoHash hash = InfoHash::from_bencode(data);

  // We can't easily predict the hash value, but we can check that it's not
  // all zeros
  bool all_zeros = true;
  for (auto byte : hash.bytes()) {
    if (byte != 0) {
      all_zeros = false;
      break;
    }
  }

  EXPECT_FALSE(all_zeros);
}

TEST(InfoHashTest, FromBencodeAsync) {
  std::vector<uint8_t> data = {'d', '4', ':', 't', 'e', 's',
                               't', 'i', '4', '2', 'e', 'e'};

  auto future = InfoHash::from_bencode_async(data);
  InfoHash hash = future.get();

  // We can't easily predict the hash value, but we can check that it's not
  // all zeros
  bool all_zeros = true;
  for (auto byte : hash.bytes()) {
    if (byte != 0) {
      all_zeros = false;
      break;
    }
  }

  EXPECT_FALSE(all_zeros);
}

TEST(InfoHashTest, Random) {
  InfoHash random1 = InfoHash::random();
  InfoHash random2 = InfoHash::random();

  EXPECT_NE(random1, random2);
}

TEST(InfoHashTest, RandomAsync) {
  InfoHash::HashStorage bytes1 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                                  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};

  InfoHash::HashStorage bytes2 = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E,
                                  0x1F, 0x20, 0x21, 0x22, 0x23, 0x24};

  InfoHash hash1(bytes1);
  InfoHash hash2(bytes2);

  auto future = InfoHash::random_async();
  InfoHash random = future.get();

  EXPECT_NE(random, hash1);
  EXPECT_NE(random, hash2);
}

TEST(InfoHashTest, ComparisonOperators) {
  InfoHash::HashStorage bytes1 = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                                  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14};

  InfoHash::HashStorage bytes2 = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E,
                                  0x1F, 0x20, 0x21, 0x22, 0x23, 0x24};

  InfoHash hash1(bytes1);
  InfoHash hash2(bytes2);

  EXPECT_EQ(hash1, hash1);
  EXPECT_NE(hash1, hash2);
  EXPECT_LT(hash1, hash2);
  EXPECT_GT(hash2, hash1);
  EXPECT_LE(hash1, hash1);
  EXPECT_LE(hash1, hash2);
  EXPECT_GE(hash2, hash2);
  EXPECT_GE(hash2, hash1);
}
