#include <gtest/gtest.h>

#include "bitscrape/types/message_types.hpp"
#include <memory>

using namespace bitscrape::types;

// Concrete message class for testing
class TestMessage : public Message {
public:
  explicit TestMessage(const std::string &data)
      : Message(Type::DHT_PING), data_(data) {}

  TestMessage(Type type, const std::string &data)
      : Message(type), data_(data) {}

  TestMessage(Type type, uint32_t custom_type_id, const std::string &data)
      : Message(type, custom_type_id), data_(data) {}

  const std::string &data() const { return data_; }

  std::vector<uint8_t> serialize() const override {
    std::vector<uint8_t> result(data_.begin(), data_.end());
    return result;
  }

  std::unique_ptr<Message> clone() const override {
    return std::make_unique<TestMessage>(*this);
  }

  std::string to_string() const override {
    return Message::to_string() + " - " + data_;
  }

private:
  std::string data_;
};

TEST(MessageTest, ConstructionWithType) {
  TestMessage message(Message::Type::DHT_PING, "test");

  EXPECT_EQ(message.type(), Message::Type::DHT_PING);
  EXPECT_EQ(message.custom_type_id(), 0);
  EXPECT_EQ(message.data(), "test");
}

TEST(MessageTest, ConstructionWithTypeAndCustomTypeID) {
  TestMessage message(Message::Type::USER_DEFINED, 42, "test");

  EXPECT_EQ(message.type(), Message::Type::USER_DEFINED);
  EXPECT_EQ(message.custom_type_id(), 42);
  EXPECT_EQ(message.data(), "test");
}

TEST(MessageTest, ToString) {
  TestMessage message(Message::Type::DHT_PING, "test");

  std::string str = message.to_string();

  // Check that the string contains the message type and data
  EXPECT_NE(str.find("DHT_PING"), std::string::npos);
  EXPECT_NE(str.find("test"), std::string::npos);
}

TEST(MessageTest, Serialize) {
  TestMessage message(Message::Type::DHT_PING, "test");

  std::vector<uint8_t> data = message.serialize();

  EXPECT_EQ(data.size(), 4UL);
  EXPECT_EQ(data[0], 't');
  EXPECT_EQ(data[1], 'e');
  EXPECT_EQ(data[2], 's');
  EXPECT_EQ(data[3], 't');
}

TEST(MessageTest, SerializeAsync) {
  TestMessage message(Message::Type::DHT_PING, "test");

  auto future = message.serialize_async();
  std::vector<uint8_t> data = future.get();

  EXPECT_EQ(data.size(), 4UL);
  EXPECT_EQ(data[0], 't');
  EXPECT_EQ(data[1], 'e');
  EXPECT_EQ(data[2], 's');
  EXPECT_EQ(data[3], 't');
}

TEST(MessageTest, Clone) {
  TestMessage message(Message::Type::DHT_PING, "test");

  auto clone = message.clone();

  EXPECT_EQ(clone->type(), message.type());
  EXPECT_EQ(clone->custom_type_id(), message.custom_type_id());

  // Cast to TestMessage to check the data
  auto test_clone = dynamic_cast<TestMessage *>(clone.get());
  EXPECT_NE(test_clone, nullptr);
  EXPECT_EQ(test_clone->data(), message.data());
}

TEST(MessageFactoryTest, CreateWithEmptyData) {
  std::vector<uint8_t> data;
  EXPECT_THROW(MessageFactory::create(data), std::runtime_error);
}

TEST(MessageFactoryTest, CreateAsyncWithEmptyData) {
  std::vector<uint8_t> data;
  auto future = MessageFactory::create_async(data);
  EXPECT_THROW(future.get(), std::runtime_error);
}
