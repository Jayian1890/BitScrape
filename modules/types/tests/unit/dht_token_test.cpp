#include <gtest/gtest.h>
#include <thread>

#include "bitscrape/types/dht_token.hpp"

using namespace bitscrape::types;

TEST(DHTTokenTest, DefaultConstructor) {
  DHTToken token1;
  DHTToken token2;

  EXPECT_NE(token1, token2);
}

TEST(DHTTokenTest, ConstructionFromByteVector) {
  std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04, 0x05};
  DHTToken token(bytes);
  EXPECT_EQ(token.bytes(), bytes);
}

TEST(DHTTokenTest, ConstructionFromString) {
  std::string str = "hello";
  DHTToken token(str);
  EXPECT_EQ(token.to_string(), str);
}

TEST(DHTTokenTest, ToString) {
  std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04, 0x05};
  DHTToken token(bytes);
  std::string str = token.to_string();

  EXPECT_EQ(str.size(), bytes.size());
  EXPECT_TRUE(std::equal(str.begin(), str.end(), bytes.begin()));
}

TEST(DHTTokenTest, IsExpired) {
  std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04, 0x05};
  DHTToken token(bytes);

  // Token should not be expired immediately with a reasonable timeout
  EXPECT_FALSE(token.is_expired(std::chrono::seconds(10)));

  // Sleep for a longer time to ensure expiration
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  // Token should be expired after a very short timeout
  // The token is created with the current time, so after sleeping it should be
  // expired
  EXPECT_TRUE(token.is_expired(std::chrono::seconds(1)));

  // Token should not be expired with a longer timeout
  EXPECT_FALSE(token.is_expired(std::chrono::seconds(10)));
}

TEST(DHTTokenTest, Random) {
  DHTToken random1 = DHTToken::random();
  DHTToken random2 = DHTToken::random();

  EXPECT_NE(random1, random2);
}

TEST(DHTTokenTest, RandomAsync) {
  std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04, 0x05};
  DHTToken token(bytes);

  auto future = DHTToken::random_async();
  DHTToken random = future.get();

  EXPECT_NE(random, token);
}

TEST(DHTTokenTest, ComparisonOperators) {
  std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04, 0x05};
  DHTToken token(bytes);

  DHTToken same(bytes);
  std::vector<uint8_t> diff_bytes = {0x05, 0x04, 0x03, 0x02, 0x01};
  DHTToken different(diff_bytes);

  EXPECT_EQ(token, same);
  EXPECT_NE(token, different);
}
