#include <bitscrape/testing.hpp>

#include "bitscrape/types/metadata_info.hpp"

using namespace bitscrape::types;

TEST(MetadataInfoTest, DefaultConstructor) {
  MetadataInfo info;

  EXPECT_TRUE(info.name().empty());
  EXPECT_EQ(info.piece_length(), 0);
  EXPECT_TRUE(info.pieces().empty());
  EXPECT_EQ(info.total_size(), 0);
  EXPECT_TRUE(info.files().empty());
  EXPECT_FALSE(info.is_valid());
}

TEST(MetadataInfoTest, ConstructionFromRawData) {
  // Create some dummy bencode data
  std::vector<uint8_t> data = {'d', '4', ':', 't', 'e', 's',
                               't', 'i', '4', '2', 'e', 'e'};

  MetadataInfo info(data);

  // In our simplified implementation, parse() always returns true
  // but doesn't actually set any fields
  EXPECT_EQ(info.raw_data(), data);
}

TEST(MetadataInfoTest, SetName) {
  MetadataInfo info;
  info.set_name("test");
  EXPECT_EQ(info.name(), "test");
}

TEST(MetadataInfoTest, SetPieceLength) {
  MetadataInfo info;
  info.set_piece_length(16384);
  EXPECT_EQ(info.piece_length(), 16384);
}

TEST(MetadataInfoTest, SetPieces) {
  MetadataInfo info;
  std::vector<std::vector<uint8_t>> pieces = {{0x01, 0x02, 0x03},
                                              {0x04, 0x05, 0x06}};
  info.set_pieces(pieces);
  EXPECT_EQ(info.pieces(), pieces);
}

TEST(MetadataInfoTest, SetTotalSize) {
  MetadataInfo info;
  info.set_total_size(1000000);
  EXPECT_EQ(info.total_size(), 1000000);
}

TEST(MetadataInfoTest, SetFiles) {
  MetadataInfo info;
  std::vector<std::pair<std::string, uint64_t>> files = {{"file1.txt", 100},
                                                         {"file2.txt", 200}};
  info.set_files(files);
  EXPECT_EQ(info.files(), files);
}

TEST(MetadataInfoTest, SetInfoHash) {
  MetadataInfo info;
  InfoHash hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  info.set_info_hash(hash);
  EXPECT_EQ(info.info_hash(), hash);
}

TEST(MetadataInfoTest, IsValid) {
  MetadataInfo info;
  
  // An empty metadata info is not valid
  EXPECT_FALSE(info.is_valid());

  // Set the required fields to make it valid
  info.set_name("test");
  info.set_piece_length(16384);
  info.set_pieces({{0x01, 0x02, 0x03}});
  info.set_total_size(1000000);

  EXPECT_TRUE(info.is_valid());
}

TEST(MetadataInfoTest, Parse) {
  MetadataInfo info;
  
  // Create some dummy bencode data
  std::vector<uint8_t> data = {'d', '4', ':', 't', 'e', 's',
                               't', 'i', '4', '2', 'e', 'e'};

  EXPECT_TRUE(info.parse(data));
  EXPECT_EQ(info.raw_data(), data);
}

TEST(MetadataInfoTest, ParseAsync) {
  MetadataInfo info;
  
  // Create some dummy bencode data
  std::vector<uint8_t> data = {'d', '4', ':', 't', 'e', 's',
                               't', 'i', '4', '2', 'e', 'e'};

  auto future = info.parse_async(data);
  bool result = future.get();

  EXPECT_TRUE(result);
  EXPECT_EQ(info.raw_data(), data);
}

TEST(MetadataInfoTest, ToString) {
  MetadataInfo info;
  
  info.set_name("test");
  info.set_piece_length(16384);
  info.set_pieces({{0x01, 0x02, 0x03}});
  info.set_total_size(1000000);
  info.set_info_hash(
      InfoHash(std::string("0102030405060708090a0b0c0d0e0f1011121314")));

  std::string str = info.to_string();

  // Check that the string contains the metadata info
  EXPECT_NE(str.find("MetadataInfo"), std::string::npos);
  EXPECT_NE(str.find("name=test"), std::string::npos);
  EXPECT_NE(str.find("piece_length=16384"), std::string::npos);
  EXPECT_NE(str.find("pieces=1"), std::string::npos);
  EXPECT_NE(str.find("total_size=1000000"), std::string::npos);
  EXPECT_NE(str.find("info_hash=0102030405060708090a0b0c0d0e0f1011121314"),
          std::string::npos);
}
