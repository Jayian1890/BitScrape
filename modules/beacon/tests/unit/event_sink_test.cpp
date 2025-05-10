#include <gtest/gtest.h>

#include "bitscrape/beacon/event_sink.hpp"
#include "bitscrape/event/event_bus.hpp"
#include <memory>

using namespace bitscrape::beacon;
using namespace bitscrape::types;
using namespace bitscrape::event;

class EventSinkTest : public ::testing::Test {
protected:
    void SetUp() override {
        event_bus_ = create_event_bus();
        received_events_.clear();

        // Subscribe to BeaconEvent
        token_ = event_bus_->subscribe<BeaconEvent>([this](const BeaconEvent& event) {
            received_events_.push_back({
                event.severity(),
                event.category(),
                event.message()
            });
        });
    }

    void TearDown() override {
        event_bus_->unsubscribe(token_);
    }

    struct ReceivedEvent {
        BeaconSeverity severity;
        BeaconCategory category;
        std::string message;
    };

    std::unique_ptr<EventBus> event_bus_;
    std::vector<ReceivedEvent> received_events_;
    SubscriptionToken token_{0}; // Initialize with a default value
};

TEST_F(EventSinkTest, WriteEvent) {
    // Create an event sink
    EventSink sink(*event_bus_);

    // Write a message
    sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Test message", std::source_location::current());

    // Check that the event was received
    EXPECT_EQ(received_events_.size(), 1UL);
    EXPECT_EQ(received_events_[0].severity, BeaconSeverity::INFO);
    EXPECT_EQ(received_events_[0].category, BeaconCategory::SYSTEM);
    EXPECT_EQ(received_events_[0].message, "Test message");
}

TEST_F(EventSinkTest, WriteAsyncEvent) {
    // Create an event sink
    EventSink sink(*event_bus_);

    // Write a message asynchronously
    auto future = sink.write_async(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Async message", std::source_location::current());

    // Wait for the future to complete
    future.wait();

    // Check that the event was received
    EXPECT_EQ(received_events_.size(), 1UL);
    EXPECT_EQ(received_events_[0].severity, BeaconSeverity::INFO);
    EXPECT_EQ(received_events_[0].category, BeaconCategory::SYSTEM);
    EXPECT_EQ(received_events_[0].message, "Async message");
}

TEST_F(EventSinkTest, FilterBySeverity) {
    // Create an event sink
    EventSink sink(*event_bus_);

    // Set minimum severity to WARNING
    sink.set_min_severity(BeaconSeverity::WARNING);

    // Write messages at different severity levels
    sink.write(BeaconSeverity::DEBUG, BeaconCategory::SYSTEM, "Debug message", std::source_location::current());
    sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Info message", std::source_location::current());
    sink.write(BeaconSeverity::WARNING, BeaconCategory::SYSTEM, "Warning message", std::source_location::current());
    sink.write(BeaconSeverity::ERROR, BeaconCategory::SYSTEM, "Error message", std::source_location::current());

    // Check that only WARNING and above were received
    EXPECT_EQ(received_events_.size(), 2UL);
    EXPECT_EQ(received_events_[0].severity, BeaconSeverity::WARNING);
    EXPECT_EQ(received_events_[1].severity, BeaconSeverity::ERROR);
}

TEST_F(EventSinkTest, FilterByCategory) {
    // Create an event sink
    EventSink sink(*event_bus_);

    // Set category filter to SYSTEM and NETWORK
    sink.set_categories({BeaconCategory::SYSTEM, BeaconCategory::NETWORK});

    // Write messages in different categories
    sink.write(BeaconSeverity::INFO, BeaconCategory::GENERAL, "General message", std::source_location::current());
    sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "System message", std::source_location::current());
    sink.write(BeaconSeverity::INFO, BeaconCategory::NETWORK, "Network message", std::source_location::current());
    sink.write(BeaconSeverity::INFO, BeaconCategory::DHT, "DHT message", std::source_location::current());

    // Check that only SYSTEM and NETWORK were received
    EXPECT_EQ(received_events_.size(), 2UL);
    EXPECT_EQ(received_events_[0].category, BeaconCategory::SYSTEM);
    EXPECT_EQ(received_events_[1].category, BeaconCategory::NETWORK);
}
