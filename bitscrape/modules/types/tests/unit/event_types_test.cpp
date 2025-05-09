#include <catch2/catch.hpp>

#include "bitscrape/types/event_types.hpp"
#include <memory>
#include <regex>

using namespace bitscrape::types;

// Concrete event class for testing
class TestEvent : public Event {
public:
    explicit TestEvent(const std::string& data)
        : Event(Type::SYSTEM_STARTUP), data_(data) {
    }
    
    TestEvent(Type type, const std::string& data)
        : Event(type), data_(data) {
    }
    
    TestEvent(Type type, uint32_t custom_type_id, const std::string& data)
        : Event(type, custom_type_id), data_(data) {
    }
    
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

TEST_CASE("Event construction", "[types][event]") {
    SECTION("Construction with type") {
        TestEvent event(Event::Type::SYSTEM_STARTUP, "test");
        
        REQUIRE(event.type() == Event::Type::SYSTEM_STARTUP);
        REQUIRE(event.custom_type_id() == 0);
        REQUIRE(event.data() == "test");
    }
    
    SECTION("Construction with type and custom type ID") {
        TestEvent event(Event::Type::USER_DEFINED, 42, "test");
        
        REQUIRE(event.type() == Event::Type::USER_DEFINED);
        REQUIRE(event.custom_type_id() == 42);
        REQUIRE(event.data() == "test");
    }
}

TEST_CASE("Event operations", "[types][event]") {
    SECTION("to_string") {
        TestEvent event(Event::Type::SYSTEM_STARTUP, "test");
        
        std::string str = event.to_string();
        
        // Check that the string contains the event type and data
        REQUIRE(str.find("SYSTEM_STARTUP") != std::string::npos);
        REQUIRE(str.find("test") != std::string::npos);
        
        // Check that the string contains a timestamp in the format YYYY-MM-DD HH:MM:SS.mmm
        std::regex timestamp_regex("\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d{3}");
        REQUIRE(std::regex_search(str, timestamp_regex));
    }
    
    SECTION("clone") {
        TestEvent event(Event::Type::SYSTEM_STARTUP, "test");
        
        auto clone = event.clone();
        
        REQUIRE(clone->type() == event.type());
        REQUIRE(clone->custom_type_id() == event.custom_type_id());
        REQUIRE(clone->timestamp() == event.timestamp());
        
        // Cast to TestEvent to check the data
        auto test_clone = dynamic_cast<TestEvent*>(clone.get());
        REQUIRE(test_clone != nullptr);
        REQUIRE(test_clone->data() == event.data());
    }
}

TEST_CASE("SubscriptionToken", "[types][event]") {
    SECTION("Construction") {
        SubscriptionToken token(42);
        
        REQUIRE(token.id() == 42);
    }
    
    SECTION("Comparison operators") {
        SubscriptionToken token1(42);
        SubscriptionToken token2(42);
        SubscriptionToken token3(43);
        
        REQUIRE(token1 == token2);
        REQUIRE(token1 != token3);
    }
    
    SECTION("Hash function") {
        SubscriptionToken token1(42);
        SubscriptionToken token2(42);
        SubscriptionToken token3(43);
        
        SubscriptionTokenHash hash;
        
        REQUIRE(hash(token1) == hash(token2));
        REQUIRE(hash(token1) != hash(token3));
    }
}
