#include <bitscrape/testing.hpp>

#include "bitscrape/dht/dht_message_factory.hpp"
#include "bitscrape/bencode/bencode_encoder.hpp"

using namespace bitscrape::dht;
using namespace bitscrape::types;
using namespace bitscrape::bencode;

TEST(DHTMessageFactoryTest, CreatePing) {
  DHTMessageFactory factory;
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  
  auto message = factory.create_ping("aa", node_id);
  
  EXPECT_NE(message, nullptr);
  EXPECT_EQ(message->type(), DHTMessage::Type::PING);
  EXPECT_EQ(message->transaction_id(), "aa");
  EXPECT_EQ(message->node_id(), node_id);
}

TEST(DHTMessageFactoryTest, CreatePingResponse) {
  DHTMessageFactory factory;
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  
  auto message = factory.create_ping_response("aa", node_id);
  
  EXPECT_NE(message, nullptr);
  EXPECT_EQ(message->type(), DHTMessage::Type::PING_RESPONSE);
  EXPECT_EQ(message->transaction_id(), "aa");
  EXPECT_EQ(message->node_id(), node_id);
}

TEST(DHTMessageFactoryTest, GenerateTransactionId) {
  std::string id1 = DHTMessageFactory::generate_transaction_id();
  std::string id2 = DHTMessageFactory::generate_transaction_id();
  
  EXPECT_EQ(id1.size(), 2);
  EXPECT_EQ(id2.size(), 2);
  EXPECT_NE(id1, id2);
}

TEST(DHTMessageFactoryTest, CreateFromBencode) {
  DHTMessageFactory factory;
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  
  // Create a ping message
  auto ping = factory.create_ping("aa", node_id);
  
  // Convert it to bencode
  BencodeValue value = ping->to_bencode();
  
  // Parse the bencode back into a message
  auto message = factory.create_from_bencode(value);
  
  EXPECT_NE(message, nullptr);
  EXPECT_EQ(message->type(), DHTMessage::Type::PING);
  EXPECT_EQ(message->transaction_id(), "aa");
  
  // Cast to DHTPingMessage to check the node ID
  auto ping_message = std::dynamic_pointer_cast<DHTPingMessage>(message);
  EXPECT_NE(ping_message, nullptr);
  EXPECT_EQ(ping_message->node_id(), node_id);
}

TEST(DHTMessageFactoryTest, CreateFromBencodeAsync) {
  DHTMessageFactory factory;
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  
  // Create a ping message
  auto ping = factory.create_ping("aa", node_id);
  
  // Convert it to bencode
  BencodeValue value = ping->to_bencode();
  
  // Parse the bencode back into a message asynchronously
  auto future = factory.create_from_bencode_async(value);
  auto message = future.get();
  
  EXPECT_NE(message, nullptr);
  EXPECT_EQ(message->type(), DHTMessage::Type::PING);
  EXPECT_EQ(message->transaction_id(), "aa");
  
  // Cast to DHTPingMessage to check the node ID
  auto ping_message = std::dynamic_pointer_cast<DHTPingMessage>(message);
  EXPECT_NE(ping_message, nullptr);
  EXPECT_EQ(ping_message->node_id(), node_id);
}

TEST(DHTMessageFactoryTest, CreateFromData) {
  DHTMessageFactory factory;
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  
  // Create a ping message
  auto ping = factory.create_ping("aa", node_id);
  
  // Encode it to a byte vector
  std::vector<uint8_t> data = ping->encode();
  
  // Parse the data back into a message
  auto message = factory.create_from_data(data);
  
  EXPECT_NE(message, nullptr);
  EXPECT_EQ(message->type(), DHTMessage::Type::PING);
  EXPECT_EQ(message->transaction_id(), "aa");
  
  // Cast to DHTPingMessage to check the node ID
  auto ping_message = std::dynamic_pointer_cast<DHTPingMessage>(message);
  EXPECT_NE(ping_message, nullptr);
  EXPECT_EQ(ping_message->node_id(), node_id);
}

TEST(DHTMessageFactoryTest, CreateFromDataAsync) {
  DHTMessageFactory factory;
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  
  // Create a ping message
  auto ping = factory.create_ping("aa", node_id);
  
  // Encode it to a byte vector
  std::vector<uint8_t> data = ping->encode();
  
  // Parse the data back into a message asynchronously
  auto future = factory.create_from_data_async(data);
  auto message = future.get();
  
  EXPECT_NE(message, nullptr);
  EXPECT_EQ(message->type(), DHTMessage::Type::PING);
  EXPECT_EQ(message->transaction_id(), "aa");
  
  // Cast to DHTPingMessage to check the node ID
  auto ping_message = std::dynamic_pointer_cast<DHTPingMessage>(message);
  EXPECT_NE(ping_message, nullptr);
  EXPECT_EQ(ping_message->node_id(), node_id);
}

TEST(DHTMessageFactoryTest, CreateFromBencodePingResponse) {
  DHTMessageFactory factory;
  NodeID node_id(std::string("0102030405060708090a0b0c0d0e0f1011121314"));
  
  // Create a ping response message
  auto ping = factory.create_ping_response("aa", node_id);
  
  // Convert it to bencode
  BencodeValue value = ping->to_bencode();
  
  // Parse the bencode back into a message
  auto message = factory.create_from_bencode(value);
  
  EXPECT_NE(message, nullptr);
  EXPECT_EQ(message->type(), DHTMessage::Type::PING_RESPONSE);
  EXPECT_EQ(message->transaction_id(), "aa");
  
  // Cast to DHTPingMessage to check the node ID
  auto ping_message = std::dynamic_pointer_cast<DHTPingMessage>(message);
  EXPECT_NE(ping_message, nullptr);
  EXPECT_EQ(ping_message->node_id(), node_id);
}

TEST(DHTMessageFactoryTest, CreateFromInvalidData) {
  DHTMessageFactory factory;
  
  // Create some invalid data
  std::vector<uint8_t> data = {0x01, 0x02, 0x03};
  
  // Try to parse it
  auto message = factory.create_from_data(data);
  
  EXPECT_EQ(message, nullptr);
}

TEST(DHTMessageFactoryTest, CreateFromInvalidBencode) {
  DHTMessageFactory factory;
  
  // Create an invalid bencode value (not a dictionary)
  BencodeValue value(42);
  
  // Try to parse it
  auto message = factory.create_from_bencode(value);
  
  EXPECT_EQ(message, nullptr);
}

TEST(DHTMessageFactoryTest, CreateFromIncompleteData) {
  DHTMessageFactory factory;
  
  // Create a bencode dictionary with missing fields
  BencodeValue value = BencodeValue::create_dictionary();
  value.dictionary_set("t", BencodeValue("aa"));
  
  // Try to parse it
  auto message = factory.create_from_bencode(value);
  
  EXPECT_EQ(message, nullptr);
}
