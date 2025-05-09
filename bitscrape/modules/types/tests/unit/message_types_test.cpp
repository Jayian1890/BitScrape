#include <catch2/catch.hpp>

#include "bitscrape/types/message_types.hpp"
#include <memory>

using namespace bitscrape::types;

// Concrete message class for testing
class TestMessage : public Message {
public:
    explicit TestMessage(const std::string& data)
        : Message(Type::DHT_PING), data_(data) {
    }
    
    TestMessage(Type type, const std::string& data)
        : Message(type), data_(data) {
    }
    
    TestMessage(Type type, uint32_t custom_type_id, const std::string& data)
        : Message(type, custom_type_id), data_(data) {
    }
    
    const std::string& data() const { return data_; }
    
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

TEST_CASE("Message construction", "[types][message]") {
    SECTION("Construction with type") {
        TestMessage message(Message::Type::DHT_PING, "test");
        
        REQUIRE(message.type() == Message::Type::DHT_PING);
        REQUIRE(message.custom_type_id() == 0);
        REQUIRE(message.data() == "test");
    }
    
    SECTION("Construction with type and custom type ID") {
        TestMessage message(Message::Type::USER_DEFINED, 42, "test");
        
        REQUIRE(message.type() == Message::Type::USER_DEFINED);
        REQUIRE(message.custom_type_id() == 42);
        REQUIRE(message.data() == "test");
    }
}

TEST_CASE("Message operations", "[types][message]") {
    SECTION("to_string") {
        TestMessage message(Message::Type::DHT_PING, "test");
        
        std::string str = message.to_string();
        
        // Check that the string contains the message type and data
        REQUIRE(str.find("DHT_PING") != std::string::npos);
        REQUIRE(str.find("test") != std::string::npos);
    }
    
    SECTION("serialize") {
        TestMessage message(Message::Type::DHT_PING, "test");
        
        std::vector<uint8_t> data = message.serialize();
        
        REQUIRE(data.size() == 4);
        REQUIRE(data[0] == 't');
        REQUIRE(data[1] == 'e');
        REQUIRE(data[2] == 's');
        REQUIRE(data[3] == 't');
    }
    
    SECTION("serialize_async") {
        TestMessage message(Message::Type::DHT_PING, "test");
        
        auto future = message.serialize_async();
        std::vector<uint8_t> data = future.get();
        
        REQUIRE(data.size() == 4);
        REQUIRE(data[0] == 't');
        REQUIRE(data[1] == 'e');
        REQUIRE(data[2] == 's');
        REQUIRE(data[3] == 't');
    }
    
    SECTION("clone") {
        TestMessage message(Message::Type::DHT_PING, "test");
        
        auto clone = message.clone();
        
        REQUIRE(clone->type() == message.type());
        REQUIRE(clone->custom_type_id() == message.custom_type_id());
        
        // Cast to TestMessage to check the data
        auto test_clone = dynamic_cast<TestMessage*>(clone.get());
        REQUIRE(test_clone != nullptr);
        REQUIRE(test_clone->data() == message.data());
    }
}

TEST_CASE("MessageFactory", "[types][message]") {
    SECTION("create with empty data") {
        std::vector<uint8_t> data;
        
        REQUIRE_THROWS_AS(MessageFactory::create(data), std::runtime_error);
    }
    
    SECTION("create_async with empty data") {
        std::vector<uint8_t> data;
        
        auto future = MessageFactory::create_async(data);
        REQUIRE_THROWS_AS(future.get(), std::runtime_error);
    }
}
