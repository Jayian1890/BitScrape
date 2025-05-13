#pragma once

#include "bitscrape/bencode/bencode_value.hpp"
#include "bitscrape/types/event_types.hpp"
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace bitscrape::bencode {

/**
 * @brief Bencode validation event types
 */
enum class BencodeValidationEventType : uint16_t {
    VALIDATION_REQUEST = 1500,     ///< Request to validate a bencode value
    VALIDATION_RESPONSE,           ///< Response with validation results
    SCHEMA_VALIDATION_REQUEST,     ///< Request to validate a bencode value against a schema
    SCHEMA_VALIDATION_RESPONSE,    ///< Response with schema validation results
    VALIDATION_ERROR               ///< Error during validation
};

/**
 * @brief Base class for bencode validation events
 */
class BencodeValidationEvent : public types::Event {
public:
    /**
     * @brief Create a new bencode validation event
     * 
     * @param type Validation event type
     * @param request_id Request ID for matching requests and responses
     */
    BencodeValidationEvent(BencodeValidationEventType type, uint64_t request_id)
        : types::Event(types::Event::Type::USER_DEFINED, static_cast<uint32_t>(type)),
          validation_event_type_(type),
          request_id_(request_id) {}
    
    /**
     * @brief Get the validation event type
     * 
     * @return Validation event type
     */
    BencodeValidationEventType validation_event_type() const { return validation_event_type_; }
    
    /**
     * @brief Get the request ID
     * 
     * @return Request ID
     */
    uint64_t request_id() const { return request_id_; }
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<BencodeValidationEvent>(*this);
    }
    
    /**
     * @brief Convert the event to a string representation
     * 
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = types::Event::to_string();
        std::string type;
        
        switch (validation_event_type_) {
            case BencodeValidationEventType::VALIDATION_REQUEST:
                type = "VALIDATION_REQUEST";
                break;
            case BencodeValidationEventType::VALIDATION_RESPONSE:
                type = "VALIDATION_RESPONSE";
                break;
            case BencodeValidationEventType::SCHEMA_VALIDATION_REQUEST:
                type = "SCHEMA_VALIDATION_REQUEST";
                break;
            case BencodeValidationEventType::SCHEMA_VALIDATION_RESPONSE:
                type = "SCHEMA_VALIDATION_RESPONSE";
                break;
            case BencodeValidationEventType::VALIDATION_ERROR:
                type = "VALIDATION_ERROR";
                break;
            default:
                type = "UNKNOWN";
                break;
        }
        
        return base + " [BencodeValidationEvent: " + type + ", RequestID: " + std::to_string(request_id_) + "]";
    }
    
private:
    BencodeValidationEventType validation_event_type_; ///< Validation event type
    uint64_t request_id_;                             ///< Request ID
};

/**
 * @brief Event for bencode validation requests
 */
class BencodeValidationRequestEvent : public BencodeValidationEvent {
public:
    /**
     * @brief Create a new bencode validation request event
     * 
     * @param request_id Request ID for matching requests and responses
     * @param value BencodeValue to validate
     * @param expected_type Expected type of the value (optional)
     */
    BencodeValidationRequestEvent(uint64_t request_id, const BencodeValue& value, const std::string& expected_type = "")
        : BencodeValidationEvent(BencodeValidationEventType::VALIDATION_REQUEST, request_id),
          value_(value),
          expected_type_(expected_type) {}
    
    /**
     * @brief Get the BencodeValue to validate
     * 
     * @return BencodeValue to validate
     */
    const BencodeValue& value() const { return value_; }
    
    /**
     * @brief Get the expected type
     * 
     * @return Expected type of the value
     */
    const std::string& expected_type() const { return expected_type_; }
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<BencodeValidationRequestEvent>(*this);
    }
    
    /**
     * @brief Convert the event to a string representation
     * 
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = BencodeValidationEvent::to_string();
        std::string type_str = expected_type_.empty() ? "any" : expected_type_;
        return base + " [Value: " + value_.to_string() + ", ExpectedType: " + type_str + "]";
    }
    
private:
    BencodeValue value_;       ///< BencodeValue to validate
    std::string expected_type_; ///< Expected type of the value
};

/**
 * @brief Event for bencode validation responses
 */
class BencodeValidationResponseEvent : public BencodeValidationEvent {
public:
    /**
     * @brief Create a new bencode validation response event
     * 
     * @param request_id Request ID for matching requests and responses
     * @param is_valid Whether the value is valid
     * @param validation_messages Validation messages (errors or warnings)
     */
    BencodeValidationResponseEvent(uint64_t request_id, bool is_valid, const std::vector<std::string>& validation_messages = {})
        : BencodeValidationEvent(BencodeValidationEventType::VALIDATION_RESPONSE, request_id),
          is_valid_(is_valid),
          validation_messages_(validation_messages) {}
    
    /**
     * @brief Check if the value is valid
     * 
     * @return true if the value is valid, false otherwise
     */
    bool is_valid() const { return is_valid_; }
    
    /**
     * @brief Get the validation messages
     * 
     * @return Validation messages (errors or warnings)
     */
    const std::vector<std::string>& validation_messages() const { return validation_messages_; }
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<BencodeValidationResponseEvent>(*this);
    }
    
    /**
     * @brief Convert the event to a string representation
     * 
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = BencodeValidationEvent::to_string();
        std::ostringstream oss;
        oss << base << " [Valid: " << (is_valid_ ? "true" : "false");
        
        if (!validation_messages_.empty()) {
            oss << ", Messages: [";
            for (size_t i = 0; i < validation_messages_.size(); ++i) {
                if (i > 0) {
                    oss << ", ";
                }
                oss << "\"" << validation_messages_[i] << "\"";
            }
            oss << "]";
        }
        
        oss << "]";
        return oss.str();
    }
    
private:
    bool is_valid_;                           ///< Whether the value is valid
    std::vector<std::string> validation_messages_; ///< Validation messages
};

/**
 * @brief Event for bencode schema validation requests
 */
class BencodeSchemaValidationRequestEvent : public BencodeValidationEvent {
public:
    /**
     * @brief Create a new bencode schema validation request event
     * 
     * @param request_id Request ID for matching requests and responses
     * @param value BencodeValue to validate
     * @param schema BencodeValue representing the schema
     */
    BencodeSchemaValidationRequestEvent(uint64_t request_id, const BencodeValue& value, const BencodeValue& schema)
        : BencodeValidationEvent(BencodeValidationEventType::SCHEMA_VALIDATION_REQUEST, request_id),
          value_(value),
          schema_(schema) {}
    
    /**
     * @brief Get the BencodeValue to validate
     * 
     * @return BencodeValue to validate
     */
    const BencodeValue& value() const { return value_; }
    
    /**
     * @brief Get the schema
     * 
     * @return BencodeValue representing the schema
     */
    const BencodeValue& schema() const { return schema_; }
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<BencodeSchemaValidationRequestEvent>(*this);
    }
    
    /**
     * @brief Convert the event to a string representation
     * 
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = BencodeValidationEvent::to_string();
        return base + " [Value: " + value_.to_string() + ", Schema: " + schema_.to_string() + "]";
    }
    
private:
    BencodeValue value_;  ///< BencodeValue to validate
    BencodeValue schema_; ///< BencodeValue representing the schema
};

} // namespace bitscrape::bencode
