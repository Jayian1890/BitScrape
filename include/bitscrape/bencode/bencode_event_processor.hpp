#pragma once

#include "bitscrape/bencode/bencode_value.hpp"
#include "bitscrape/event/event_processor.hpp"
#include "bitscrape/event/event_bus.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <future>
#include <functional>

namespace bitscrape::bencode {

/**
 * @brief Event for bencode encoding/decoding
 */
class BencodeEvent : public types::Event {
public:
    /**
     * @brief Event type
     */
    enum class Type {
        ENCODE_REQUEST,   ///< Request to encode a BencodeValue
        ENCODE_RESPONSE,  ///< Response with encoded data
        DECODE_REQUEST,   ///< Request to decode bencode data
        DECODE_RESPONSE,  ///< Response with decoded BencodeValue
        ERROR             ///< Error during encoding/decoding
    };
    
    /**
     * @brief Create a new bencode event
     * 
     * @param type Event type
     * @param request_id Request ID for matching requests and responses
     */
    BencodeEvent(Type type, uint64_t request_id);
    
    /**
     * @brief Get the event type
     * 
     * @return Event type
     */
    Type event_type() const;
    
    /**
     * @brief Get the request ID
     * 
     * @return Request ID
     */
    uint64_t request_id() const;
    
    /**
     * @brief Clone the event
     * 
     * @return Unique pointer to a clone of the event
     */
    std::unique_ptr<types::Event> clone() const override;
    
private:
    Type type_;           ///< Event type
    uint64_t request_id_; ///< Request ID
};

/**
 * @brief Event for bencode encoding requests
 */
class BencodeEncodeRequestEvent : public BencodeEvent {
public:
    /**
     * @brief Create a new bencode encode request event
     * 
     * @param request_id Request ID for matching requests and responses
     * @param value BencodeValue to encode
     */
    BencodeEncodeRequestEvent(uint64_t request_id, const BencodeValue& value);
    
    /**
     * @brief Get the BencodeValue to encode
     * 
     * @return BencodeValue to encode
     */
    const BencodeValue& value() const;
    
    /**
     * @brief Clone the event
     * 
     * @return Unique pointer to a clone of the event
     */
    std::unique_ptr<types::Event> clone() const override;
    
private:
    BencodeValue value_; ///< BencodeValue to encode
};

/**
 * @brief Event for bencode encoding responses
 */
class BencodeEncodeResponseEvent : public BencodeEvent {
public:
    /**
     * @brief Create a new bencode encode response event
     * 
     * @param request_id Request ID for matching requests and responses
     * @param data Encoded data
     */
    BencodeEncodeResponseEvent(uint64_t request_id, const std::vector<uint8_t>& data);
    
    /**
     * @brief Get the encoded data
     * 
     * @return Encoded data
     */
    const std::vector<uint8_t>& data() const;
    
    /**
     * @brief Clone the event
     * 
     * @return Unique pointer to a clone of the event
     */
    std::unique_ptr<types::Event> clone() const override;
    
private:
    std::vector<uint8_t> data_; ///< Encoded data
};

/**
 * @brief Event for bencode decoding requests
 */
class BencodeDecodeRequestEvent : public BencodeEvent {
public:
    /**
     * @brief Create a new bencode decode request event
     * 
     * @param request_id Request ID for matching requests and responses
     * @param data Bencode data to decode
     */
    BencodeDecodeRequestEvent(uint64_t request_id, const std::vector<uint8_t>& data);
    
    /**
     * @brief Get the bencode data to decode
     * 
     * @return Bencode data to decode
     */
    const std::vector<uint8_t>& data() const;
    
    /**
     * @brief Clone the event
     * 
     * @return Unique pointer to a clone of the event
     */
    std::unique_ptr<types::Event> clone() const override;
    
private:
    std::vector<uint8_t> data_; ///< Bencode data to decode
};

/**
 * @brief Event for bencode decoding responses
 */
class BencodeDecodeResponseEvent : public BencodeEvent {
public:
    /**
     * @brief Create a new bencode decode response event
     * 
     * @param request_id Request ID for matching requests and responses
     * @param value Decoded BencodeValue
     */
    BencodeDecodeResponseEvent(uint64_t request_id, const BencodeValue& value);
    
    /**
     * @brief Get the decoded BencodeValue
     * 
     * @return Decoded BencodeValue
     */
    const BencodeValue& value() const;
    
    /**
     * @brief Clone the event
     * 
     * @return Unique pointer to a clone of the event
     */
    std::unique_ptr<types::Event> clone() const override;
    
private:
    BencodeValue value_; ///< Decoded BencodeValue
};

/**
 * @brief Event for bencode errors
 */
class BencodeErrorEvent : public BencodeEvent {
public:
    /**
     * @brief Create a new bencode error event
     * 
     * @param request_id Request ID for matching requests and responses
     * @param error_message Error message
     */
    BencodeErrorEvent(uint64_t request_id, const std::string& error_message);
    
    /**
     * @brief Get the error message
     * 
     * @return Error message
     */
    const std::string& error_message() const;
    
    /**
     * @brief Clone the event
     * 
     * @return Unique pointer to a clone of the event
     */
    std::unique_ptr<types::Event> clone() const override;
    
private:
    std::string error_message_; ///< Error message
};

/**
 * @brief Interface for bencode event processors
 * 
 * BencodeEventProcessor is responsible for processing bencode events.
 */
class BencodeEventProcessor : public event::EventProcessor {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~BencodeEventProcessor() = default;
    
    /**
     * @brief Encode a BencodeValue asynchronously
     * 
     * @param value BencodeValue to encode
     * @return Future containing the encoded data
     */
    virtual std::future<std::vector<uint8_t>> encode_async(const BencodeValue& value) = 0;
    
    /**
     * @brief Decode bencode data asynchronously
     * 
     * @param data Bencode data to decode
     * @return Future containing the decoded BencodeValue
     */
    virtual std::future<BencodeValue> decode_async(const std::vector<uint8_t>& data) = 0;
};

/**
 * @brief Create a new bencode event processor
 * 
 * @return Unique pointer to a new bencode event processor
 */
std::unique_ptr<BencodeEventProcessor> create_bencode_event_processor();

} // namespace bitscrape::bencode
