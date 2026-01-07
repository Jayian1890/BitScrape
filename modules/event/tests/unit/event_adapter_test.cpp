#include <bitscrape/testing.hpp>

#include "bitscrape/event/event_adapter.hpp"
#include "bitscrape/event/event_bus.hpp"
#include "bitscrape/types/event_types.hpp"

#include <memory>
#include <string>
#include <vector>

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

// Test adapter class
class TestAdapter : public EventAdapterBase {
public:
    TestAdapter() = default;
    
    void send_message(const std::string& message) {
        if (is_connected()) {
            publish(TestEvent(Event::Type::SYSTEM_STARTUP, message));
        }
    }
    
    std::vector<std::string> received_messages() const {
        return received_messages_;
    }
    
protected:
    void register_handlers() override {
        subscribe<TestEvent>([this](const TestEvent& event) {
            if (event.type() == Event::Type::NETWORK_CONNECTED) {
                received_messages_.push_back(event.data());
            }
        });
    }
    
private:
    std::vector<std::string> received_messages_;
};

TEST(EventAdapterTest, ConnectAndDisconnect) {
    auto event_bus = create_event_bus();
    auto adapter = std::make_unique<TestAdapter>();
    
    // Check initial state
    EXPECT_FALSE(adapter->is_connected());
    
    // Connect to the event bus
    adapter->connect(*event_bus);
    EXPECT_TRUE(adapter->is_connected());
    
    // Disconnect from the event bus
    adapter->disconnect();
    EXPECT_FALSE(adapter->is_connected());
}

TEST(EventAdapterTest, SendAndReceiveMessages) {
    auto event_bus = create_event_bus();
    auto adapter1 = std::make_unique<TestAdapter>();
    auto adapter2 = std::make_unique<TestAdapter>();
    
    // Connect to the event bus
    adapter1->connect(*event_bus);
    adapter2->connect(*event_bus);
    
    // Send messages from adapter1
    adapter1->send_message("Message 1");
    adapter1->send_message("Message 2");
    
    // Check that adapter2 didn't receive the messages (wrong type)
    EXPECT_TRUE(adapter2->received_messages().empty());
    
    // Publish network connected events
    event_bus->publish(TestEvent(Event::Type::NETWORK_CONNECTED, "Network 1"));
    event_bus->publish(TestEvent(Event::Type::NETWORK_CONNECTED, "Network 2"));
    
    // Check that adapter1 received the messages
    ASSERT_EQ(adapter1->received_messages().size(), 2);
    EXPECT_EQ(adapter1->received_messages()[0], "Network 1");
    EXPECT_EQ(adapter1->received_messages()[1], "Network 2");
    
    // Check that adapter2 received the messages
    ASSERT_EQ(adapter2->received_messages().size(), 2);
    EXPECT_EQ(adapter2->received_messages()[0], "Network 1");
    EXPECT_EQ(adapter2->received_messages()[1], "Network 2");
    
    // Disconnect adapter1
    adapter1->disconnect();
    
    // Publish another network connected event
    event_bus->publish(TestEvent(Event::Type::NETWORK_CONNECTED, "Network 3"));
    
    // Check that adapter1 didn't receive the message
    ASSERT_EQ(adapter1->received_messages().size(), 2);
    
    // Check that adapter2 received the message
    ASSERT_EQ(adapter2->received_messages().size(), 3);
    EXPECT_EQ(adapter2->received_messages()[2], "Network 3");
}
