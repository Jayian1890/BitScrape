#include <bitscrape/testing.hpp>

#include "bitscrape/dht/dht_message.hpp"
#include "bitscrape/bencode/bencode_decoder.hpp"

using namespace bitscrape::dht;
using namespace bitscrape::types;
using namespace bitscrape::bencode;

TEST(DHTMessageTest, Constructor) {
  DHTMessage message(DHTMessage::Type::PING, "aa");
  
  EXPECT_EQ(message.type(), DHTMessage::Type::PING);
  EXPECT_EQ(message.transaction_id(), "aa");
}

TEST(DHTMessageTest, SetTransactionId) {
  DHTMessage message(DHTMessage::Type::PING, "aa");
  
  message.set_transaction_id("bb");
  EXPECT_EQ(message.transaction_id(), "bb");
}

TEST(DHTMessageTest, IsValid) {
  DHTMessage message1(DHTMessage::Type::PING, "aa");
  EXPECT_TRUE(message1.is_valid());
  
  DHTMessage message2(DHTMessage::Type::PING, "");
  EXPECT_FALSE(message2.is_valid());
}

TEST(DHTMessageTest, ToString) {
  DHTMessage message(DHTMessage::Type::PING, "aa");
  
  std::string str = message.to_string();
  EXPECT_NE(str.find("PING"), std::string::npos);
  EXPECT_NE(str.find("aa"), std::string::npos);
}

TEST(DHTMessageTest, ToBencode) {
  DHTMessage message(DHTMessage::Type::PING, "aa");
  
  BencodeValue value = message.to_bencode();
  EXPECT_TRUE(value.is_dictionary());
  EXPECT_TRUE(value.dictionary_contains("t"));
  EXPECT_TRUE(value.dictionary_contains("y"));
  EXPECT_TRUE(value.dictionary_contains("q"));
  EXPECT_TRUE(value.dictionary_contains("v"));
  
  EXPECT_EQ(value.dictionary_get("t").string_value(), "aa");
  EXPECT_EQ(value.dictionary_get("y").string_value(), "q");
  EXPECT_EQ(value.dictionary_get("q").string_value(), "ping");
  EXPECT_EQ(value.dictionary_get("v").string_value(), "BS");
}

TEST(DHTMessageTest, ToBencodeAsync) {
  DHTMessage message(DHTMessage::Type::PING, "aa");
  
  auto future = message.to_bencode_async();
  BencodeValue value = future.get();
  
  EXPECT_TRUE(value.is_dictionary());
  EXPECT_TRUE(value.dictionary_contains("t"));
  EXPECT_TRUE(value.dictionary_contains("y"));
  EXPECT_TRUE(value.dictionary_contains("q"));
  EXPECT_TRUE(value.dictionary_contains("v"));
  
  EXPECT_EQ(value.dictionary_get("t").string_value(), "aa");
  EXPECT_EQ(value.dictionary_get("y").string_value(), "q");
  EXPECT_EQ(value.dictionary_get("q").string_value(), "ping");
  EXPECT_EQ(value.dictionary_get("v").string_value(), "BS");
}

TEST(DHTMessageTest, Encode) {
  DHTMessage message(DHTMessage::Type::PING, "aa");
  
  std::vector<uint8_t> data = message.encode();
  EXPECT_FALSE(data.empty());
  
  // Decode the data to verify it
  BencodeDecoder decoder;
  BencodeValue value = decoder.decode(data);
  
  EXPECT_TRUE(value.is_dictionary());
  EXPECT_TRUE(value.dictionary_contains("t"));
  EXPECT_TRUE(value.dictionary_contains("y"));
  EXPECT_TRUE(value.dictionary_contains("q"));
  EXPECT_TRUE(value.dictionary_contains("v"));
  
  EXPECT_EQ(value.dictionary_get("t").string_value(), "aa");
  EXPECT_EQ(value.dictionary_get("y").string_value(), "q");
  EXPECT_EQ(value.dictionary_get("q").string_value(), "ping");
  EXPECT_EQ(value.dictionary_get("v").string_value(), "BS");
}

TEST(DHTMessageTest, EncodeAsync) {
  DHTMessage message(DHTMessage::Type::PING, "aa");
  
  auto future = message.encode_async();
  std::vector<uint8_t> data = future.get();
  
  EXPECT_FALSE(data.empty());
  
  // Decode the data to verify it
  BencodeDecoder decoder;
  BencodeValue value = decoder.decode(data);
  
  EXPECT_TRUE(value.is_dictionary());
  EXPECT_TRUE(value.dictionary_contains("t"));
  EXPECT_TRUE(value.dictionary_contains("y"));
  EXPECT_TRUE(value.dictionary_contains("q"));
  EXPECT_TRUE(value.dictionary_contains("v"));
  
  EXPECT_EQ(value.dictionary_get("t").string_value(), "aa");
  EXPECT_EQ(value.dictionary_get("y").string_value(), "q");
  EXPECT_EQ(value.dictionary_get("q").string_value(), "ping");
  EXPECT_EQ(value.dictionary_get("v").string_value(), "BS");
}

TEST(DHTPingMessageTest, Constructor) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  DHTPingMessage message("aa", node_id);
  
  EXPECT_EQ(message.type(), DHTMessage::Type::PING);
  EXPECT_EQ(message.transaction_id(), "aa");
  EXPECT_EQ(message.node_id(), node_id);
}

TEST(DHTPingMessageTest, ConstructorWithResponse) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  DHTPingMessage message("aa", node_id, true);
  
  EXPECT_EQ(message.type(), DHTMessage::Type::PING_RESPONSE);
  EXPECT_EQ(message.transaction_id(), "aa");
  EXPECT_EQ(message.node_id(), node_id);
}

TEST(DHTPingMessageTest, SetNodeId) {
  NodeID node_id1(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID node_id2(std::string("1112131415161718191a1b1c1d1e1f2021222324"));
  
  DHTPingMessage message("aa", node_id1);
  EXPECT_EQ(message.node_id(), node_id1);
  
  message.set_node_id(node_id2);
  EXPECT_EQ(message.node_id(), node_id2);
}

TEST(DHTPingMessageTest, IsValid) {
  NodeID valid_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  NodeID invalid_id;
  
  DHTPingMessage message1("aa", valid_id);
  EXPECT_TRUE(message1.is_valid());
  
  DHTPingMessage message2("", valid_id);
  EXPECT_FALSE(message2.is_valid());
  
  DHTPingMessage message3("aa", invalid_id);
  EXPECT_FALSE(message3.is_valid());
}

TEST(DHTPingMessageTest, ToString) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  DHTPingMessage message("aa", node_id);
  
  std::string str = message.to_string();
  EXPECT_NE(str.find("PING"), std::string::npos);
  EXPECT_NE(str.find("aa"), std::string::npos);
  EXPECT_NE(str.find(node_id.to_hex().substr(0, 8)), std::string::npos);
}

TEST(DHTPingMessageTest, ToBencode) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  DHTPingMessage message("aa", node_id);
  
  BencodeValue value = message.to_bencode();
  EXPECT_TRUE(value.is_dictionary());
  EXPECT_TRUE(value.dictionary_contains("t"));
  EXPECT_TRUE(value.dictionary_contains("y"));
  EXPECT_TRUE(value.dictionary_contains("q"));
  EXPECT_TRUE(value.dictionary_contains("a"));
  EXPECT_TRUE(value.dictionary_contains("v"));
  
  EXPECT_EQ(value.dictionary_get("t").string_value(), "aa");
  EXPECT_EQ(value.dictionary_get("y").string_value(), "q");
  EXPECT_EQ(value.dictionary_get("q").string_value(), "ping");
  EXPECT_EQ(value.dictionary_get("v").string_value(), "BS");
  
  BencodeValue args = value.dictionary_get("a");
  EXPECT_TRUE(args.is_dictionary());
  EXPECT_TRUE(args.dictionary_contains("id"));
  
  std::string id_str = args.dictionary_get("id").string_value();
  std::vector<uint8_t> id_bytes(id_str.begin(), id_str.end());
  NodeID parsed_id(id_bytes);
  
  EXPECT_EQ(parsed_id, node_id);
}

TEST(DHTPingMessageTest, ToBencodeResponse) {
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  DHTPingMessage message("aa", node_id, true);
  
  BencodeValue value = message.to_bencode();
  EXPECT_TRUE(value.is_dictionary());
  EXPECT_TRUE(value.dictionary_contains("t"));
  EXPECT_TRUE(value.dictionary_contains("y"));
  EXPECT_TRUE(value.dictionary_contains("r"));
  EXPECT_TRUE(value.dictionary_contains("v"));
  
  EXPECT_EQ(value.dictionary_get("t").string_value(), "aa");
  EXPECT_EQ(value.dictionary_get("y").string_value(), "r");
  EXPECT_EQ(value.dictionary_get("v").string_value(), "BS");
  
  BencodeValue response = value.dictionary_get("r");
  EXPECT_TRUE(response.is_dictionary());
  EXPECT_TRUE(response.dictionary_contains("id"));
  
  std::string id_str = response.dictionary_get("id").string_value();
  std::vector<uint8_t> id_bytes(id_str.begin(), id_str.end());
  NodeID parsed_id(id_bytes);
  
  EXPECT_EQ(parsed_id, node_id);
}
