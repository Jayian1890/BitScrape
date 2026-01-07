#include <bitscrape/testing.hpp>

#include "bitscrape/event/event_filter.hpp"
#include "bitscrape/types/event_types.hpp"

#include <memory>
#include <string>

using namespace bitscrape::event;
using namespace bitscrape::types;

// Test event class
class TestEvent : public Event {
public:
    explicit TestEvent(const std::string& data)
        : Event(Type::SYSTEM_STARTUP), data_(data) {}
    
    TestEvent(Type type, const std::string& data)
        : Event(type), data_(data) {}
    
    TestEvent(Type type, uint32_t custom_type_id, const std::string& data)
        : Event(type, custom_type_id), data_(data) {}
    
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

TEST(EventFilterTest, TypeFilter) {
    auto filter = create_type_filter(Event::Type::SYSTEM_STARTUP);
    
    // Test events
    TestEvent event1(Event::Type::SYSTEM_STARTUP, "System Startup");
    TestEvent event2(Event::Type::SYSTEM_SHUTDOWN, "System Shutdown");
    TestEvent event3(Event::Type::NETWORK_CONNECTED, "Network Connected");
    
    // Check filter results
    EXPECT_TRUE(filter->passes(event1));
    EXPECT_FALSE(filter->passes(event2));
    EXPECT_FALSE(filter->passes(event3));
}

TEST(EventFilterTest, PredicateFilter) {
    auto filter = create_predicate_filter([](const Event& event) {
        // Check if the event is a system event
        return event.type() == Event::Type::SYSTEM_STARTUP ||
               event.type() == Event::Type::SYSTEM_SHUTDOWN ||
               event.type() == Event::Type::SYSTEM_ERROR;
    });
    
    // Test events
    TestEvent event1(Event::Type::SYSTEM_STARTUP, "System Startup");
    TestEvent event2(Event::Type::SYSTEM_SHUTDOWN, "System Shutdown");
    TestEvent event3(Event::Type::NETWORK_CONNECTED, "Network Connected");
    
    // Check filter results
    EXPECT_TRUE(filter->passes(event1));
    EXPECT_TRUE(filter->passes(event2));
    EXPECT_FALSE(filter->passes(event3));
}

TEST(EventFilterTest, AndFilter) {
    auto filter1 = create_type_filter(Event::Type::SYSTEM_STARTUP);
    auto filter2 = create_predicate_filter([](const Event& event) {
        // Check if the event data contains "Test"
        auto test_event = dynamic_cast<const TestEvent*>(&event);
        return test_event && test_event->data().find("Test") != std::string::npos;
    });
    
    auto filter = create_and_filter(std::move(filter1), std::move(filter2));
    
    // Test events
    TestEvent event1(Event::Type::SYSTEM_STARTUP, "Test Event");
    TestEvent event2(Event::Type::SYSTEM_STARTUP, "System Startup");
    TestEvent event3(Event::Type::SYSTEM_SHUTDOWN, "Test Event");
    
    // Check filter results
    EXPECT_TRUE(filter->passes(event1));
    EXPECT_FALSE(filter->passes(event2));
    EXPECT_FALSE(filter->passes(event3));
}

TEST(EventFilterTest, OrFilter) {
    auto filter1 = create_type_filter(Event::Type::SYSTEM_STARTUP);
    auto filter2 = create_type_filter(Event::Type::SYSTEM_SHUTDOWN);
    
    auto filter = create_or_filter(std::move(filter1), std::move(filter2));
    
    // Test events
    TestEvent event1(Event::Type::SYSTEM_STARTUP, "System Startup");
    TestEvent event2(Event::Type::SYSTEM_SHUTDOWN, "System Shutdown");
    TestEvent event3(Event::Type::NETWORK_CONNECTED, "Network Connected");
    
    // Check filter results
    EXPECT_TRUE(filter->passes(event1));
    EXPECT_TRUE(filter->passes(event2));
    EXPECT_FALSE(filter->passes(event3));
}

TEST(EventFilterTest, NotFilter) {
    auto filter1 = create_type_filter(Event::Type::SYSTEM_STARTUP);
    
    auto filter = create_not_filter(std::move(filter1));
    
    // Test events
    TestEvent event1(Event::Type::SYSTEM_STARTUP, "System Startup");
    TestEvent event2(Event::Type::SYSTEM_SHUTDOWN, "System Shutdown");
    TestEvent event3(Event::Type::NETWORK_CONNECTED, "Network Connected");
    
    // Check filter results
    EXPECT_FALSE(filter->passes(event1));
    EXPECT_TRUE(filter->passes(event2));
    EXPECT_TRUE(filter->passes(event3));
}
