#include <gtest/gtest.h>

#include "bitscrape/types/event_types.hpp"
#include <memory>
#include <regex>

using namespace bitscrape::types;

// Concrete event class for testing
class TestEvent : public Event {
public:
  explicit TestEvent(const std::string &data)
      : Event(Type::SYSTEM_STARTUP), data_(data) {}

  TestEvent(Type type, const std::string &data) : Event(type), data_(data) {}

  TestEvent(Type type, uint32_t custom_type_id, const std::string &data)
      : Event(type, custom_type_id), data_(data) {}

  const std::string &data() const { return data_; }

  std::unique_ptr<Event> clone() const override {
    return std::make_unique<TestEvent>(*this);
  }

  std::string to_string() const override {
    return Event::to_string() + " - " + data_;
  }

private:
  std::string data_;
};

TEST(EventTest, ConstructionWithType) {
  TestEvent event(Event::Type::SYSTEM_STARTUP, "test");

  EXPECT_EQ(event.type(), Event::Type::SYSTEM_STARTUP);
  EXPECT_EQ(event.custom_type_id(), 0);
  EXPECT_EQ(event.data(), "test");
}

TEST(EventTest, ConstructionWithTypeAndCustomTypeID) {
  TestEvent event(Event::Type::USER_DEFINED, 42, "test");

  EXPECT_EQ(event.type(), Event::Type::USER_DEFINED);
  EXPECT_EQ(event.custom_type_id(), 42);
  EXPECT_EQ(event.data(), "test");
}

TEST(EventTest, ToString) {
  TestEvent event(Event::Type::SYSTEM_STARTUP, "test");

  std::string str = event.to_string();

  // Check that the string contains the event type and data
  EXPECT_NE(str.find("SYSTEM_STARTUP"), std::string::npos);
  EXPECT_NE(str.find("test"), std::string::npos);

  // Check that the string contains a timestamp in the format YYYY-MM-DD
  // HH:MM:SS.mmm
  std::regex timestamp_regex(
      "\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d{3}");
  EXPECT_TRUE(std::regex_search(str, timestamp_regex));
}

TEST(EventTest, Clone) {
  TestEvent event(Event::Type::SYSTEM_STARTUP, "test");

  auto clone = event.clone();

  EXPECT_EQ(clone->type(), event.type());
  EXPECT_EQ(clone->custom_type_id(), event.custom_type_id());
  EXPECT_EQ(clone->timestamp(), event.timestamp());

  // Cast to TestEvent to check the data
  auto test_clone = dynamic_cast<TestEvent *>(clone.get());
  EXPECT_NE(test_clone, nullptr);
  EXPECT_EQ(test_clone->data(), event.data());
}

TEST(SubscriptionTokenTest, Construction) {
  SubscriptionToken token(42);

  EXPECT_EQ(token.id(), 42);
}

TEST(SubscriptionTokenTest, ComparisonOperators) {
  SubscriptionToken token1(42);
  SubscriptionToken token2(42);
  SubscriptionToken token3(43);

  EXPECT_EQ(token1, token2);
  EXPECT_NE(token1, token3);
}

TEST(SubscriptionTokenTest, HashFunction) {
  SubscriptionToken token1(42);
  SubscriptionToken token2(42);
  SubscriptionToken token3(43);

  SubscriptionTokenHash hash;

  EXPECT_EQ(hash(token1), hash(token2));
  EXPECT_NE(hash(token1), hash(token3));
}
