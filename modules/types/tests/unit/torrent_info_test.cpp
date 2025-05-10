#include <gtest/gtest.h>

#include "bitscrape/types/torrent_info.hpp"

using namespace bitscrape::types;

TEST(TorrentInfoTest, DefaultConstructor) {
  TorrentInfo info;

  EXPECT_TRUE(info.announce().empty());
  EXPECT_TRUE(info.announce_list().empty());
  EXPECT_FALSE(info.creation_date().has_value());
  EXPECT_TRUE(info.comment().empty());
  EXPECT_TRUE(info.created_by().empty());
  EXPECT_FALSE(info.is_valid());
}

TEST(TorrentInfoTest, ConstructionFromRawData) {
  // Create some dummy bencode data
  std::vector<uint8_t> data = {'d', '4', ':', 't', 'e', 's',
                               't', 'i', '4', '2', 'e', 'e'};

  TorrentInfo info(data);

  // In our simplified implementation, parse() always returns true
  // but doesn't actually set any fields
  EXPECT_EQ(info.raw_data(), data);
}

TEST(TorrentInfoTest, ConstructionFromInfoHashAndMetadata) {
  InfoHash hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  MetadataInfo metadata;
  metadata.set_name("test");
  metadata.set_piece_length(16384);
  metadata.set_pieces({{0x01, 0x02, 0x03}});
  metadata.set_total_size(1000000);

  TorrentInfo info(hash, metadata);

  EXPECT_EQ(info.info_hash(), hash);
  EXPECT_EQ(info.metadata().name(), "test");
  EXPECT_EQ(info.metadata().piece_length(), 16384);
  EXPECT_EQ(info.metadata().pieces().size(), 1UL);
  EXPECT_EQ(info.metadata().total_size(), 1000000);
}

TEST(TorrentInfoTest, SetInfoHash) {
  TorrentInfo info;
  InfoHash hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  info.set_info_hash(hash);
  EXPECT_EQ(info.info_hash(), hash);
}

TEST(TorrentInfoTest, SetMetadata) {
  TorrentInfo info;
  MetadataInfo metadata;
  metadata.set_name("test");
  metadata.set_piece_length(16384);
  metadata.set_pieces({{0x01, 0x02, 0x03}});
  metadata.set_total_size(1000000);

  info.set_metadata(metadata);

  EXPECT_EQ(info.metadata().name(), "test");
  EXPECT_EQ(info.metadata().piece_length(), 16384);
  EXPECT_EQ(info.metadata().pieces().size(), 1UL);
  EXPECT_EQ(info.metadata().total_size(), 1000000);
}

TEST(TorrentInfoTest, SetAnnounce) {
  TorrentInfo info;
  info.set_announce("http://tracker.example.com:6969/announce");
  EXPECT_EQ(info.announce(), "http://tracker.example.com:6969/announce");
}

TEST(TorrentInfoTest, SetAnnounceList) {
  TorrentInfo info;
  std::vector<std::string> list = {"http://tracker1.example.com:6969/announce",
                                   "http://tracker2.example.com:6969/announce"};

  info.set_announce_list(list);
  EXPECT_EQ(info.announce_list(), list);
}

TEST(TorrentInfoTest, SetCreationDate) {
  TorrentInfo info;
  auto now = std::chrono::system_clock::now();
  info.set_creation_date(now);
  EXPECT_TRUE(info.creation_date().has_value());
  EXPECT_EQ(*info.creation_date(), now);
}

TEST(TorrentInfoTest, SetComment) {
  TorrentInfo info;
  info.set_comment("Test comment");
  EXPECT_EQ(info.comment(), "Test comment");
}

TEST(TorrentInfoTest, SetCreatedBy) {
  TorrentInfo info;
  info.set_created_by("Test client");
  EXPECT_EQ(info.created_by(), "Test client");
}

TEST(TorrentInfoTest, IsValid) {
  TorrentInfo info;
  // An empty torrent info is not valid
  EXPECT_FALSE(info.is_valid());

  // Set the metadata to make it valid
  MetadataInfo metadata;
  metadata.set_name("test");
  metadata.set_piece_length(16384);
  metadata.set_pieces({{0x01, 0x02, 0x03}});
  metadata.set_total_size(1000000);

  info.set_metadata(metadata);
  EXPECT_TRUE(info.is_valid());
}

TEST(TorrentInfoTest, Parse) {
  TorrentInfo info;
  // Create some dummy bencode data
  std::vector<uint8_t> data = {'d', '4', ':', 't', 'e', 's',
                               't', 'i', '4', '2', 'e', 'e'};

  EXPECT_TRUE(info.parse(data));
  EXPECT_EQ(info.raw_data(), data);
}

TEST(TorrentInfoTest, ParseAsync) {
  TorrentInfo info;
  // Create some dummy bencode data
  std::vector<uint8_t> data = {'d', '4', ':', 't', 'e', 's',
                               't', 'i', '4', '2', 'e', 'e'};

  auto future = info.parse_async(data);
  bool result = future.get();

  EXPECT_TRUE(result);
  EXPECT_EQ(info.raw_data(), data);
}

TEST(TorrentInfoTest, ToString) {
  TorrentInfo info;
  InfoHash hash(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  MetadataInfo metadata;
  metadata.set_name("test");
  metadata.set_piece_length(16384);
  metadata.set_pieces({{0x01, 0x02, 0x03}});
  metadata.set_total_size(1000000);

  info.set_info_hash(hash);
  info.set_metadata(metadata);
  info.set_announce("http://tracker.example.com:6969/announce");
  info.set_announce_list({"http://tracker1.example.com:6969/announce",
                          "http://tracker2.example.com:6969/announce"});
  info.set_comment("Test comment");
  info.set_created_by("Test client");

  std::string str = info.to_string();

  // Check that the string contains the torrent info
  EXPECT_NE(str.find("TorrentInfo"), std::string::npos);
  EXPECT_NE(str.find("info_hash=0102030405060708090a0b0c0d0e0f1011121314"),
            std::string::npos);
  EXPECT_NE(str.find("name=test"), std::string::npos);
  EXPECT_NE(str.find("announce=http://tracker.example.com:6969/announce"),
            std::string::npos);
  EXPECT_NE(str.find("announce_list=2"), std::string::npos);
  EXPECT_NE(str.find("comment=Test comment"), std::string::npos);
  EXPECT_NE(str.find("created_by=Test client"), std::string::npos);
}
