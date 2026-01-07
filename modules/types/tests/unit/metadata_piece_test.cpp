#include <bitscrape/testing.hpp>

#include "bitscrape/types/metadata_piece.hpp"

using namespace bitscrape::types;

TEST(MetadataPieceTest, DefaultConstructor) {
  MetadataPiece piece;

  EXPECT_EQ(piece.index(), 0);
  EXPECT_TRUE(piece.data().empty());
  EXPECT_EQ(piece.total_size(), 0);
  EXPECT_FALSE(piece.is_valid());
}

TEST(MetadataPieceTest, ConstructionWithIndexDataAndTotalSize) {
  uint32_t index = 1;
  std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
  uint32_t total_size = 1000;

  MetadataPiece piece(index, data, total_size);

  EXPECT_EQ(piece.index(), index);
  EXPECT_EQ(piece.data(), data);
  EXPECT_EQ(piece.total_size(), total_size);
  EXPECT_TRUE(piece.is_valid());
}

TEST(MetadataPieceTest, SetIndex) {
  MetadataPiece piece;
  piece.set_index(1);
  EXPECT_EQ(piece.index(), 1);
}

TEST(MetadataPieceTest, SetData) {
  MetadataPiece piece;
  std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
  piece.set_data(data);
  EXPECT_EQ(piece.data(), data);
}

TEST(MetadataPieceTest, SetTotalSize) {
  MetadataPiece piece;
  piece.set_total_size(1000);
  EXPECT_EQ(piece.total_size(), 1000);
}

TEST(MetadataPieceTest, IsValid) {
  MetadataPiece piece;
  // An empty metadata piece is not valid
  EXPECT_FALSE(piece.is_valid());

  // Set the data and total size to make it valid
  piece.set_data({0x01, 0x02, 0x03});
  piece.set_total_size(1000);
  EXPECT_TRUE(piece.is_valid());
}

TEST(MetadataPieceTest, ToString) {
  MetadataPiece piece;
  piece.set_index(1);
  piece.set_data({0x01, 0x02, 0x03, 0x04, 0x05});
  piece.set_total_size(1000);

  std::string str = piece.to_string();

  // Check that the string contains the metadata piece info
  EXPECT_NE(str.find("MetadataPiece"), std::string::npos);
  EXPECT_NE(str.find("index=1"), std::string::npos);
  EXPECT_NE(str.find("data_size=5"), std::string::npos);
  EXPECT_NE(str.find("total_size=1000"), std::string::npos);
}

TEST(MetadataPieceTest, ComparisonOperators) {
  MetadataPiece piece1(1, {0x01, 0x02, 0x03}, 1000);
  MetadataPiece piece2(1, {0x01, 0x02, 0x03}, 1000);
  MetadataPiece piece3(2, {0x01, 0x02, 0x03}, 1000);
  MetadataPiece piece4(1, {0x04, 0x05, 0x06}, 1000);
  MetadataPiece piece5(1, {0x01, 0x02, 0x03}, 2000);

  EXPECT_EQ(piece1, piece2);
  EXPECT_NE(piece1, piece3);
  EXPECT_NE(piece1, piece4);
  EXPECT_NE(piece1, piece5);
}
