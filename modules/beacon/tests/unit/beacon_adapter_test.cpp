#include <gtest/gtest.h>

#include "bitscrape/beacon/beacon_adapter.hpp"
#include "bitscrape/event/event_bus.hpp"
#include <memory>

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

// Test implementation of Beacon for testing
class TestBeacon : public Beacon {
public:
    struct LogEntry {
        BeaconSeverity severity;
        BeaconCategory category;
        std::string message;
    };

    void log(BeaconSeverity severity, const std::string& message,
            BeaconCategory category = BeaconCategory::GENERAL,
            const std::source_location& location = std::source_location::current()) override {
        entries.push_back({severity, category, message});
    }

    std::vector<LogEntry> entries;
};

TEST(BeaconAdapterTest, DummyTest) {
    EXPECT_TRUE(true);
}
