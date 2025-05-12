#include <gtest/gtest.h>

#include "bitscrape/beacon/beacon_adapter.hpp"
#include "bitscrape/event/event_bus.hpp"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace bitscrape::beacon;
using namespace bitscrape::types;
using namespace bitscrape::event;

// Test event class
class TestEvent : public Event {
public:
    explicit TestEvent(const std::string& data)
        : Event(Type::USER_DEFINED, 2001), data_(data) {}

    const std::string& data() const { return data_; }

    std::unique_ptr<Event> clone() const override {
        return std::make_unique<TestEvent>(*this);
    }

    std::string to_string() const override {
        return Event::to_string() + " - " + data_;
    }

private:
    std::string data_;
};

// Another test event class
class AnotherTestEvent : public Event {
public:
    explicit AnotherTestEvent(int value)
        : Event(Type::USER_DEFINED, 2002), value_(value) {}

    int value() const { return value_; }

    std::unique_ptr<Event> clone() const override {
        return std::make_unique<AnotherTestEvent>(*this);
    }

    std::string to_string() const override {
        return Event::to_string() + " - " + std::to_string(value_);
    }

private:
    int value_;
};

// Test implementation of BeaconSink for testing
class TestSink : public BeaconSink {
public:
    struct LogEntry {
        BeaconSeverity severity;
        BeaconCategory category;
        std::string message;
        std::source_location location;
    };

    void write(BeaconSeverity severity, BeaconCategory category,
              const std::string& message, const std::source_location& location) override {
        if (should_log(severity, category)) {
            entries.push_back({severity, category, message, location});
        }
    }

    std::vector<LogEntry> entries;
};

// Test implementation of Beacon for testing
class TestBeacon : public Beacon {
public:
    TestBeacon() {
        // Add a test sink
        sink_ = std::make_unique<TestSink>();
        sink_ptr_ = sink_.get();
        add_sink(std::move(sink_));
    }

    TestSink* sink() const {
        return sink_ptr_;
    }

private:
    std::unique_ptr<TestSink> sink_;
    TestSink* sink_ptr_;
};

TEST(BeaconAdapterTest, BasicEventMapping) {
    // Create a test beacon
    TestBeacon beacon;

    // Create a beacon adapter
    auto adapter = create_beacon_adapter(beacon);

    // Add event mapping
    adapter->add_event_mapping<TestEvent>(BeaconSeverity::WARNING, BeaconCategory::SYSTEM);

    // Call handle_event directly with a test event
    TestEvent event("Test event");
    adapter->handle_event(event);

    // Check that the event was logged with the correct severity and category
    ASSERT_EQ(beacon.sink()->entries.size(), 1UL);
    EXPECT_EQ(beacon.sink()->entries[0].severity, BeaconSeverity::WARNING);
    EXPECT_EQ(beacon.sink()->entries[0].category, BeaconCategory::SYSTEM);
    EXPECT_NE(beacon.sink()->entries[0].message.find("Test event"), std::string::npos);
}

TEST(BeaconAdapterTest, EventFormatter) {
    // Create a test beacon
    TestBeacon beacon;

    // Create a beacon adapter
    auto adapter = create_beacon_adapter(beacon);

    // Add event mapping (this is needed for the formatter to work)
    adapter->add_event_mapping<TestEvent>(BeaconSeverity::INFO, BeaconCategory::GENERAL);

    // Add event formatter
    adapter->add_event_formatter<TestEvent>([](const TestEvent& event) {
        return "Formatted: " + event.data();
    });

    // Call handle_event directly with a test event
    TestEvent event("Test event");
    adapter->handle_event(event);

    // Check that the event was logged with the formatted message
    ASSERT_EQ(beacon.sink()->entries.size(), 1UL);

    // Print the actual message for debugging
    std::cout << "Actual message: " << beacon.sink()->entries[0].message << std::endl;

    // Check that the message contains the formatted text
    EXPECT_TRUE(beacon.sink()->entries[0].message.find("Formatted: Test event") != std::string::npos);
}

TEST(BeaconAdapterTest, MultipleEventTypes) {
    // Create a test beacon
    TestBeacon beacon;

    // Create a beacon adapter
    auto adapter = create_beacon_adapter(beacon);

    // Add event mappings for different event types
    adapter->add_event_mapping<TestEvent>(BeaconSeverity::WARNING, BeaconCategory::SYSTEM);
    adapter->add_event_mapping<AnotherTestEvent>(BeaconSeverity::ERROR, BeaconCategory::NETWORK);

    // Call handle_event directly with test events
    TestEvent event1("Test event");
    AnotherTestEvent event2(42);
    adapter->handle_event(event1);
    adapter->handle_event(event2);

    // Check that both events were logged with the correct severity and category
    ASSERT_EQ(beacon.sink()->entries.size(), 2UL);

    // Check the first event
    EXPECT_EQ(beacon.sink()->entries[0].severity, BeaconSeverity::WARNING);
    EXPECT_EQ(beacon.sink()->entries[0].category, BeaconCategory::SYSTEM);
    EXPECT_NE(beacon.sink()->entries[0].message.find("Test event"), std::string::npos);

    // Check the second event
    EXPECT_EQ(beacon.sink()->entries[1].severity, BeaconSeverity::ERROR);
    EXPECT_EQ(beacon.sink()->entries[1].category, BeaconCategory::NETWORK);
    EXPECT_NE(beacon.sink()->entries[1].message.find("42"), std::string::npos);
}

TEST(BeaconAdapterTest, ConnectDisconnect) {
    // Create a test beacon
    TestBeacon beacon;

    // Create a beacon adapter
    auto adapter = create_beacon_adapter(beacon);

    // Create an event bus
    auto event_bus = create_event_bus();

    // Connect the adapter to the event bus
    adapter->connect(*event_bus);
    EXPECT_TRUE(adapter->is_connected());

    // Disconnect the adapter
    adapter->disconnect();
    EXPECT_FALSE(adapter->is_connected());
}

TEST(BeaconAdapterTest, AsyncEventHandling) {
    // Create a test beacon
    TestBeacon beacon;

    // Create a beacon adapter
    auto adapter = create_beacon_adapter(beacon);

    // Call handle_event directly with a test event
    TestEvent event("Async event");

    // Use std::async to call handle_event asynchronously
    auto future = std::async(std::launch::async, [&]() {
        adapter->handle_event(event);
    });

    // Wait for the future to complete
    future.wait();

    // Check that the event was logged
    ASSERT_EQ(beacon.sink()->entries.size(), 1UL);
    EXPECT_NE(beacon.sink()->entries[0].message.find("Async event"), std::string::npos);
}

TEST(BeaconAdapterTest, DefaultMapping) {
    // Create a test beacon
    TestBeacon beacon;

    // Create a beacon adapter
    auto adapter = create_beacon_adapter(beacon);

    // Call handle_event directly with a test event (without adding a mapping)
    TestEvent event("Test event");
    adapter->handle_event(event);

    // Check that the event was logged with the default severity and category
    ASSERT_EQ(beacon.sink()->entries.size(), 1UL);
    EXPECT_EQ(beacon.sink()->entries[0].severity, BeaconSeverity::INFO);
    EXPECT_EQ(beacon.sink()->entries[0].category, BeaconCategory::GENERAL);
    EXPECT_NE(beacon.sink()->entries[0].message.find("Test event"), std::string::npos);
}

TEST(BeaconAdapterTest, MultipleAsyncEvents) {
    // Create a test beacon
    TestBeacon beacon;

    // Create a beacon adapter
    auto adapter = create_beacon_adapter(beacon);

    // Add event mappings for different event types
    adapter->add_event_mapping<TestEvent>(BeaconSeverity::WARNING, BeaconCategory::SYSTEM);
    adapter->add_event_mapping<AnotherTestEvent>(BeaconSeverity::ERROR, BeaconCategory::NETWORK);

    // Create test events
    TestEvent event1("Async event 1");
    AnotherTestEvent event2(42);

    // Use std::async to call handle_event asynchronously for both events
    auto future1 = std::async(std::launch::async, [&]() {
        adapter->handle_event(event1);
    });

    auto future2 = std::async(std::launch::async, [&]() {
        adapter->handle_event(event2);
    });

    // Wait for the futures to complete
    future1.wait();
    future2.wait();

    // Check that the events were logged with the correct severity and category
    ASSERT_EQ(beacon.sink()->entries.size(), 2UL);

    // Note: The order of events might not be guaranteed in asynchronous processing
    // So we check that both events are present without assuming order
    bool found_event1 = false;
    bool found_event2 = false;

    for (const auto& entry : beacon.sink()->entries) {
        if (entry.message.find("Async event 1") != std::string::npos) {
            found_event1 = true;
            EXPECT_EQ(entry.severity, BeaconSeverity::WARNING);
            EXPECT_EQ(entry.category, BeaconCategory::SYSTEM);
        } else if (entry.message.find("42") != std::string::npos) {
            found_event2 = true;
            EXPECT_EQ(entry.severity, BeaconSeverity::ERROR);
            EXPECT_EQ(entry.category, BeaconCategory::NETWORK);
        }
    }

    EXPECT_TRUE(found_event1);
    EXPECT_TRUE(found_event2);
}
