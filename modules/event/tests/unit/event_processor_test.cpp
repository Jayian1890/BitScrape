#include <gtest/gtest.h>

#include "bitscrape/event/event_bus.hpp"
#include "bitscrape/event/event_processor.hpp"
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

TEST(EventProcessorTest, StartAndStop) {
  auto event_bus = create_event_bus();
  auto processor = create_event_processor();

  // Check initial state
  EXPECT_FALSE(processor->is_running());

  // Start the processor
  processor->start(*event_bus);
  EXPECT_TRUE(processor->is_running());

  // Stop the processor
  processor->stop();
  EXPECT_FALSE(processor->is_running());
}

TEST(EventProcessorTest, ProcessEvent) {
  auto event_bus = create_event_bus();
  auto processor = create_event_processor();

  std::vector<std::string> processed_events;

  // Start the processor
  processor->start(*event_bus);

  // Subscribe to events
  auto token = event_bus->subscribe<TestEvent>(
      [&processed_events](const TestEvent &event) {
        processed_events.push_back(event.data());
      });

  // Publish events
  event_bus->publish(TestEvent("Event 1"));
  event_bus->publish(TestEvent("Event 2"));
  event_bus->publish(TestEvent("Event 3"));

  // Check that all events were processed
  ASSERT_EQ(processed_events.size(), 3);
  EXPECT_EQ(processed_events[0], "Event 1");
  EXPECT_EQ(processed_events[1], "Event 2");
  EXPECT_EQ(processed_events[2], "Event 3");

  // Stop the processor
  processor->stop();
}

TEST(EventProcessorTest, ProcessEventAsync) {
  auto event_bus = create_event_bus();
  auto processor = create_event_processor();

  std::vector<std::string> processed_events;
  std::mutex mutex;
  std::condition_variable cv;
  int event_count = 0;

  // Start the processor
  processor->start(*event_bus);

  // Subscribe directly to the processor's events
  auto token = event_bus->subscribe<TestEvent>(
      [&processed_events, &mutex, &cv, &event_count](const TestEvent &event) {
        std::lock_guard<std::mutex> lock(mutex);
        processed_events.push_back(event.data());
        event_count++;
        if (event_count >= 3) {
          cv.notify_one();
        }
      });

  // Process events asynchronously
  auto future1 = event_bus->publish_async(TestEvent("Event 1"));
  auto future2 = event_bus->publish_async(TestEvent("Event 2"));
  auto future3 = event_bus->publish_async(TestEvent("Event 3"));

  // Wait for all events to be processed
  future1.wait();
  future2.wait();
  future3.wait();

  // Wait for all events to be received
  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, std::chrono::seconds(1),
                [&event_count] { return event_count >= 3; });
  }

  // Check that all events were processed
  std::lock_guard<std::mutex> lock(mutex);
  ASSERT_EQ(processed_events.size(), 3);

  // Check that all events are in the processed_events vector
  // (order may not be guaranteed with async operations)
  std::vector<std::string> expected_events = {"Event 1", "Event 2", "Event 3"};
  std::sort(processed_events.begin(), processed_events.end());
  std::sort(expected_events.begin(), expected_events.end());
  EXPECT_EQ(processed_events, expected_events);

  // Stop the processor
  processor->stop();
}
