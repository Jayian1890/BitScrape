#include <gtest/gtest.h>

#include "bitscrape/bencode/bencode_validation_event.hpp"
#include "bitscrape/bencode/bencode_value.hpp"
#include <memory>
#include <string>
#include <vector>

using namespace bitscrape::bencode;
using namespace bitscrape::types;

TEST(BencodeValidationEventTest, BencodeValidationRequestEventConstruction) {
    BencodeValue value(std::string("test"));
    BencodeValidationRequestEvent event(42, value, "string");
    
    EXPECT_EQ(event.type(), Event::Type::USER_DEFINED);
    EXPECT_EQ(event.custom_type_id(), static_cast<uint32_t>(BencodeValidationEventType::VALIDATION_REQUEST));
    EXPECT_EQ(event.validation_event_type(), BencodeValidationEventType::VALIDATION_REQUEST);
    EXPECT_EQ(event.request_id(), 42);
    EXPECT_TRUE(event.value().is_string());
    EXPECT_EQ(event.value().as_string(), "test");
    EXPECT_EQ(event.expected_type(), "string");
}

TEST(BencodeValidationEventTest, BencodeValidationResponseEventConstruction) {
    std::vector<std::string> messages = {"Value is not a dictionary", "Missing required field 'name'"};
    BencodeValidationResponseEvent event(42, false, messages);
    
    EXPECT_EQ(event.type(), Event::Type::USER_DEFINED);
    EXPECT_EQ(event.custom_type_id(), static_cast<uint32_t>(BencodeValidationEventType::VALIDATION_RESPONSE));
    EXPECT_EQ(event.validation_event_type(), BencodeValidationEventType::VALIDATION_RESPONSE);
    EXPECT_EQ(event.request_id(), 42);
    EXPECT_FALSE(event.is_valid());
    EXPECT_EQ(event.validation_messages().size(), 2);
    EXPECT_EQ(event.validation_messages()[0], "Value is not a dictionary");
    EXPECT_EQ(event.validation_messages()[1], "Missing required field 'name'");
}

TEST(BencodeValidationEventTest, BencodeSchemaValidationRequestEventConstruction) {
    BencodeValue value(std::string("test"));
    BencodeValue schema;
    schema.as_dictionary()["type"] = BencodeValue(std::string("string"));
    schema.as_dictionary()["minLength"] = BencodeValue(static_cast<int64_t>(1));
    schema.as_dictionary()["maxLength"] = BencodeValue(static_cast<int64_t>(10));
    
    BencodeSchemaValidationRequestEvent event(42, value, schema);
    
    EXPECT_EQ(event.type(), Event::Type::USER_DEFINED);
    EXPECT_EQ(event.custom_type_id(), static_cast<uint32_t>(BencodeValidationEventType::SCHEMA_VALIDATION_REQUEST));
    EXPECT_EQ(event.validation_event_type(), BencodeValidationEventType::SCHEMA_VALIDATION_REQUEST);
    EXPECT_EQ(event.request_id(), 42);
    EXPECT_TRUE(event.value().is_string());
    EXPECT_EQ(event.value().as_string(), "test");
    EXPECT_TRUE(event.schema().is_dictionary());
    EXPECT_TRUE(event.schema().as_dictionary().at("type").is_string());
    EXPECT_EQ(event.schema().as_dictionary().at("type").as_string(), "string");
    EXPECT_TRUE(event.schema().as_dictionary().at("minLength").is_integer());
    EXPECT_EQ(event.schema().as_dictionary().at("minLength").as_integer(), 1);
    EXPECT_TRUE(event.schema().as_dictionary().at("maxLength").is_integer());
    EXPECT_EQ(event.schema().as_dictionary().at("maxLength").as_integer(), 10);
}

TEST(BencodeValidationEventTest, BencodeValidationRequestEventClone) {
    BencodeValue value(std::string("test"));
    BencodeValidationRequestEvent event(42, value, "string");
    auto clone = event.clone();
    
    EXPECT_NE(clone.get(), &event);
    
    auto* validation_event = dynamic_cast<BencodeValidationRequestEvent*>(clone.get());
    EXPECT_NE(validation_event, nullptr);
    EXPECT_EQ(validation_event->validation_event_type(), BencodeValidationEventType::VALIDATION_REQUEST);
    EXPECT_EQ(validation_event->request_id(), 42);
    EXPECT_TRUE(validation_event->value().is_string());
    EXPECT_EQ(validation_event->value().as_string(), "test");
    EXPECT_EQ(validation_event->expected_type(), "string");
}

TEST(BencodeValidationEventTest, BencodeValidationResponseEventClone) {
    std::vector<std::string> messages = {"Value is not a dictionary", "Missing required field 'name'"};
    BencodeValidationResponseEvent event(42, false, messages);
    auto clone = event.clone();
    
    EXPECT_NE(clone.get(), &event);
    
    auto* validation_event = dynamic_cast<BencodeValidationResponseEvent*>(clone.get());
    EXPECT_NE(validation_event, nullptr);
    EXPECT_EQ(validation_event->validation_event_type(), BencodeValidationEventType::VALIDATION_RESPONSE);
    EXPECT_EQ(validation_event->request_id(), 42);
    EXPECT_FALSE(validation_event->is_valid());
    EXPECT_EQ(validation_event->validation_messages().size(), 2);
    EXPECT_EQ(validation_event->validation_messages()[0], "Value is not a dictionary");
    EXPECT_EQ(validation_event->validation_messages()[1], "Missing required field 'name'");
}

TEST(BencodeValidationEventTest, BencodeValidationRequestEventToString) {
    BencodeValue value(std::string("test"));
    BencodeValidationRequestEvent event(42, value, "string");
    std::string str = event.to_string();
    
    // Check that the string contains the event type and data
    EXPECT_NE(str.find("VALIDATION_REQUEST"), std::string::npos);
    EXPECT_NE(str.find("RequestID: 42"), std::string::npos);
    EXPECT_NE(str.find("Value: "), std::string::npos);
    EXPECT_NE(str.find("ExpectedType: string"), std::string::npos);
}

TEST(BencodeValidationEventTest, BencodeValidationResponseEventToString) {
    std::vector<std::string> messages = {"Value is not a dictionary", "Missing required field 'name'"};
    BencodeValidationResponseEvent event(42, false, messages);
    std::string str = event.to_string();
    
    // Check that the string contains the event type and data
    EXPECT_NE(str.find("VALIDATION_RESPONSE"), std::string::npos);
    EXPECT_NE(str.find("RequestID: 42"), std::string::npos);
    EXPECT_NE(str.find("Valid: false"), std::string::npos);
    EXPECT_NE(str.find("Messages: ["), std::string::npos);
    EXPECT_NE(str.find("\"Value is not a dictionary\""), std::string::npos);
    EXPECT_NE(str.find("\"Missing required field 'name'\""), std::string::npos);
}
