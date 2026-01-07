#include <bitscrape/testing.hpp>

#include "bitscrape/event/event_bus.hpp"
#include "bitscrape/types/event_types.hpp"

#include <algorithm>
#include <future>
#include <memory>
#include <string>
#include <vector>

using namespace bitscrape::event;
using namespace bitscrape::types;

// Test event class
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

TEST(EventBusTest, SubscribeAndPublish) {
  auto event_bus = create_event_bus();

  std::vector<std::string> received_events;

  // Subscribe to events
  auto token = event_bus->subscribe<TestEvent>(
      [&received_events](const TestEvent &event) {
        received_events.push_back(event.data());
      });

  // Publish events
  event_bus->publish(TestEvent("Event 1"));
  event_bus->publish(TestEvent("Event 2"));
  event_bus->publish(TestEvent("Event 3"));

  // Check that all events were received
  ASSERT_EQ(received_events.size(), 3);
  EXPECT_EQ(received_events[0], "Event 1");
  EXPECT_EQ(received_events[1], "Event 2");
  EXPECT_EQ(received_events[2], "Event 3");

  // Unsubscribe
  EXPECT_TRUE(event_bus->unsubscribe(token));

  // Publish another event
  event_bus->publish(TestEvent("Event 4"));

  // Check that the event was not received
  ASSERT_EQ(received_events.size(), 3);
}

TEST(EventBusTest, MultipleSubscribers) {
  auto event_bus = create_event_bus();

  std::vector<std::string> received_events1;
  std::vector<std::string> received_events2;

  // Subscribe to events
  auto token1 = event_bus->subscribe<TestEvent>(
      [&received_events1](const TestEvent &event) {
        received_events1.push_back(event.data());
      });

  auto token2 = event_bus->subscribe<TestEvent>(
      [&received_events2](const TestEvent &event) {
        received_events2.push_back(event.data());
      });

  // Publish events
  event_bus->publish(TestEvent("Event 1"));
  event_bus->publish(TestEvent("Event 2"));

  // Check that all events were received by both subscribers
  ASSERT_EQ(received_events1.size(), 2);
  EXPECT_EQ(received_events1[0], "Event 1");
  EXPECT_EQ(received_events1[1], "Event 2");

  ASSERT_EQ(received_events2.size(), 2);
  EXPECT_EQ(received_events2[0], "Event 1");
  EXPECT_EQ(received_events2[1], "Event 2");

  // Unsubscribe one subscriber
  EXPECT_TRUE(event_bus->unsubscribe(token1));

  // Publish another event
  event_bus->publish(TestEvent("Event 3"));

  // Check that the event was received only by the second subscriber
  ASSERT_EQ(received_events1.size(), 2);

  ASSERT_EQ(received_events2.size(), 3);
  EXPECT_EQ(received_events2[2], "Event 3");
}

TEST(EventBusTest, AsyncPublish) {
  auto event_bus = create_event_bus();

  std::vector<std::string> received_events;

  // Subscribe to events
  auto token = event_bus->subscribe<TestEvent>(
      [&received_events](const TestEvent &event) {
        received_events.push_back(event.data());
      });

  // Publish events asynchronously
  auto future1 = event_bus->publish_async(TestEvent("Event 1"));
  auto future2 = event_bus->publish_async(TestEvent("Event 2"));
  auto future3 = event_bus->publish_async(TestEvent("Event 3"));

  // Wait for all events to be published
  future1.wait();
  future2.wait();
  future3.wait();

  // Check that all events were received
  ASSERT_EQ(received_events.size(), 3);

  // Check that all events are in the received_events vector
  // (order may not be guaranteed with async operations)
  std::vector<std::string> expected_events = {"Event 1", "Event 2", "Event 3"};
  std::sort(received_events.begin(), received_events.end());
  std::sort(expected_events.begin(), expected_events.end());
  EXPECT_EQ(received_events, expected_events);
}

TEST(EventBusTest, TypedSubscription) {
  auto event_bus = create_event_bus();

  std::vector<std::string> system_events;
  std::vector<std::string> network_events;

  // Subscribe to system events
  auto system_token =
      event_bus->subscribe<TestEvent>([&system_events](const TestEvent &event) {
        if (event.type() == Event::Type::SYSTEM_STARTUP ||
            event.type() == Event::Type::SYSTEM_SHUTDOWN ||
            event.type() == Event::Type::SYSTEM_ERROR) {
          system_events.push_back(event.data());
        }
      });

  // Subscribe to network events
  auto network_token = event_bus->subscribe<TestEvent>(
      [&network_events](const TestEvent &event) {
        if (event.type() == Event::Type::NETWORK_CONNECTED ||
            event.type() == Event::Type::NETWORK_DISCONNECTED ||
            event.type() == Event::Type::NETWORK_ERROR) {
          network_events.push_back(event.data());
        }
      });

  // Publish events
  event_bus->publish(TestEvent(Event::Type::SYSTEM_STARTUP, "System Startup"));
  event_bus->publish(
      TestEvent(Event::Type::NETWORK_CONNECTED, "Network Connected"));
  event_bus->publish(TestEvent(Event::Type::SYSTEM_ERROR, "System Error"));
  event_bus->publish(
      TestEvent(Event::Type::NETWORK_DISCONNECTED, "Network Disconnected"));

  // Check that events were received by the correct subscribers
  ASSERT_EQ(system_events.size(), 2);
  EXPECT_EQ(system_events[0], "System Startup");
  EXPECT_EQ(system_events[1], "System Error");

  ASSERT_EQ(network_events.size(), 2);
  EXPECT_EQ(network_events[0], "Network Connected");
  EXPECT_EQ(network_events[1], "Network Disconnected");
}
