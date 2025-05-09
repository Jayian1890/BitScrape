#include "bitscrape/bencode/bencode_event_processor.hpp"
#include "bitscrape/bencode/bencode_decoder.hpp"
#include "bitscrape/bencode/bencode_encoder.hpp"

#include <algorithm>
#include <atomic>
#include <future>
#include <mutex>
#include <unordered_map>

namespace bitscrape::bencode {

// BencodeEvent implementation
BencodeEvent::BencodeEvent(Type type, uint64_t request_id)
    : types::Event(types::Event::Type::USER_DEFINED, 1001), type_(type),
      request_id_(request_id) {}

BencodeEvent::Type BencodeEvent::event_type() const { return type_; }

uint64_t BencodeEvent::request_id() const { return request_id_; }

std::unique_ptr<types::Event> BencodeEvent::clone() const {
  return std::make_unique<BencodeEvent>(*this);
}

// BencodeEncodeRequestEvent implementation
BencodeEncodeRequestEvent::BencodeEncodeRequestEvent(uint64_t request_id,
                                                     const BencodeValue &value)
    : BencodeEvent(Type::ENCODE_REQUEST, request_id), value_(value) {}

const BencodeValue &BencodeEncodeRequestEvent::value() const { return value_; }

std::unique_ptr<types::Event> BencodeEncodeRequestEvent::clone() const {
  return std::make_unique<BencodeEncodeRequestEvent>(*this);
}

// BencodeEncodeResponseEvent implementation
BencodeEncodeResponseEvent::BencodeEncodeResponseEvent(
    uint64_t request_id, const std::vector<uint8_t> &data)
    : BencodeEvent(Type::ENCODE_RESPONSE, request_id), data_(data) {}

const std::vector<uint8_t> &BencodeEncodeResponseEvent::data() const {
  return data_;
}

std::unique_ptr<types::Event> BencodeEncodeResponseEvent::clone() const {
  return std::make_unique<BencodeEncodeResponseEvent>(*this);
}

// BencodeDecodeRequestEvent implementation
BencodeDecodeRequestEvent::BencodeDecodeRequestEvent(
    uint64_t request_id, const std::vector<uint8_t> &data)
    : BencodeEvent(Type::DECODE_REQUEST, request_id), data_(data) {}

const std::vector<uint8_t> &BencodeDecodeRequestEvent::data() const {
  return data_;
}

std::unique_ptr<types::Event> BencodeDecodeRequestEvent::clone() const {
  return std::make_unique<BencodeDecodeRequestEvent>(*this);
}

// BencodeDecodeResponseEvent implementation
BencodeDecodeResponseEvent::BencodeDecodeResponseEvent(
    uint64_t request_id, const BencodeValue &value)
    : BencodeEvent(Type::DECODE_RESPONSE, request_id), value_(value) {}

const BencodeValue &BencodeDecodeResponseEvent::value() const { return value_; }

std::unique_ptr<types::Event> BencodeDecodeResponseEvent::clone() const {
  return std::make_unique<BencodeDecodeResponseEvent>(*this);
}

// BencodeErrorEvent implementation
BencodeErrorEvent::BencodeErrorEvent(uint64_t request_id,
                                     const std::string &error_message)
    : BencodeEvent(Type::ERROR, request_id), error_message_(error_message) {}

const std::string &BencodeErrorEvent::error_message() const {
  return error_message_;
}

std::unique_ptr<types::Event> BencodeErrorEvent::clone() const {
  return std::make_unique<BencodeErrorEvent>(*this);
}

/**
 * @brief Implementation of the bencode event processor
 */
class BencodeEventProcessorImpl : public BencodeEventProcessor {
public:
  /**
   * @brief Constructor
   */
  BencodeEventProcessorImpl()
      : running_(false), next_request_id_(1), token_(0),
        encoder_(create_bencode_encoder()), decoder_(create_bencode_decoder()) {
  }

  /**
   * @brief Destructor
   */
  ~BencodeEventProcessorImpl() override { stop(); }

  /**
   * @brief Start processing events
   *
   * @param event_bus Event bus to process events from
   */
  void start(event::EventBus &event_bus) override {
    if (running_) {
      return;
    }

    running_ = true;
    event_bus_ = &event_bus;

    // Subscribe to bencode events
    token_ = event_bus_->subscribe<BencodeEvent>(
        [this](const BencodeEvent &event) { this->process(event); });
  }

  /**
   * @brief Stop processing events
   */
  void stop() override {
    if (!running_) {
      return;
    }

    running_ = false;

    // Unsubscribe from events
    if (event_bus_) {
      event_bus_->unsubscribe(token_);
    }

    // Clear pending requests
    {
      std::lock_guard<std::mutex> lock(mutex_);
      encode_promises_.clear();
      decode_promises_.clear();
    }
  }

  /**
   * @brief Check if the processor is running
   *
   * @return true if the processor is running, false otherwise
   */
  bool is_running() const override { return running_; }

  /**
   * @brief Process an event
   *
   * @param event Event to process
   */
  void process(const types::Event &event) override {
    if (!running_) {
      return;
    }

    // Check if the event is a bencode event
    const auto *bencode_event = dynamic_cast<const BencodeEvent *>(&event);
    if (!bencode_event) {
      return;
    }

    // Process the event based on its type
    switch (bencode_event->event_type()) {
    case BencodeEvent::Type::ENCODE_REQUEST:
      process_encode_request(
          *static_cast<const BencodeEncodeRequestEvent *>(bencode_event));
      break;
    case BencodeEvent::Type::DECODE_REQUEST:
      process_decode_request(
          *static_cast<const BencodeDecodeRequestEvent *>(bencode_event));
      break;
    case BencodeEvent::Type::ENCODE_RESPONSE:
      process_encode_response(
          *static_cast<const BencodeEncodeResponseEvent *>(bencode_event));
      break;
    case BencodeEvent::Type::DECODE_RESPONSE:
      process_decode_response(
          *static_cast<const BencodeDecodeResponseEvent *>(bencode_event));
      break;
    case BencodeEvent::Type::ERROR:
      process_error(*static_cast<const BencodeErrorEvent *>(bencode_event));
      break;
    }
  }

  /**
   * @brief Process an event asynchronously
   *
   * @param event Event to process
   * @return Future that will be completed when the event has been processed
   */
  std::future<void> process_async(const types::Event &event) override {
    return std::async(std::launch::async, [this, event = event.clone()]() {
      this->process(*event);
    });
  }

  /**
   * @brief Encode a BencodeValue asynchronously
   *
   * @param value BencodeValue to encode
   * @return Future containing the encoded data
   */
  std::future<std::vector<uint8_t>>
  encode_async(const BencodeValue &value) override {
    if (!running_ || !event_bus_) {
      return encoder_->encode_async(value);
    }

    // Create a promise for the result
    std::promise<std::vector<uint8_t>> promise;
    auto future = promise.get_future();

    // Generate a request ID
    uint64_t request_id = next_request_id_++;

    // Store the promise
    {
      std::lock_guard<std::mutex> lock(mutex_);
      encode_promises_[request_id] = std::move(promise);
    }

    // Publish the encode request event
    event_bus_->publish(BencodeEncodeRequestEvent(request_id, value));

    return future;
  }

  /**
   * @brief Decode bencode data asynchronously
   *
   * @param data Bencode data to decode
   * @return Future containing the decoded BencodeValue
   */
  std::future<BencodeValue>
  decode_async(const std::vector<uint8_t> &data) override {
    if (!running_ || !event_bus_) {
      return decoder_->decode_async(data);
    }

    // Create a promise for the result
    std::promise<BencodeValue> promise;
    auto future = promise.get_future();

    // Generate a request ID
    uint64_t request_id = next_request_id_++;

    // Store the promise
    {
      std::lock_guard<std::mutex> lock(mutex_);
      decode_promises_[request_id] = std::move(promise);
    }

    // Publish the decode request event
    event_bus_->publish(BencodeDecodeRequestEvent(request_id, data));

    return future;
  }

private:
  /**
   * @brief Process an encode request event
   *
   * @param event Encode request event
   */
  void process_encode_request(const BencodeEncodeRequestEvent &event) {
    if (!running_ || !event_bus_) {
      return;
    }

    try {
      // Encode the value
      auto data = encoder_->encode(event.value());

      // Publish the encode response event
      event_bus_->publish(BencodeEncodeResponseEvent(event.request_id(), data));
    } catch (const std::exception &e) {
      // Publish an error event
      event_bus_->publish(BencodeErrorEvent(event.request_id(), e.what()));
    }
  }

  /**
   * @brief Process a decode request event
   *
   * @param event Decode request event
   */
  void process_decode_request(const BencodeDecodeRequestEvent &event) {
    if (!running_ || !event_bus_) {
      return;
    }

    try {
      // Decode the data
      auto value = decoder_->decode(event.data());

      // Publish the decode response event
      event_bus_->publish(
          BencodeDecodeResponseEvent(event.request_id(), value));
    } catch (const std::exception &e) {
      // Publish an error event
      event_bus_->publish(BencodeErrorEvent(event.request_id(), e.what()));
    }
  }

  /**
   * @brief Process an encode response event
   *
   * @param event Encode response event
   */
  void process_encode_response(const BencodeEncodeResponseEvent &event) {
    std::promise<std::vector<uint8_t>> promise;

    // Find and remove the promise
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = encode_promises_.find(event.request_id());
      if (it == encode_promises_.end()) {
        return;
      }

      promise = std::move(it->second);
      encode_promises_.erase(it);
    }

    // Set the value
    promise.set_value(event.data());
  }

  /**
   * @brief Process a decode response event
   *
   * @param event Decode response event
   */
  void process_decode_response(const BencodeDecodeResponseEvent &event) {
    std::promise<BencodeValue> promise;

    // Find and remove the promise
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = decode_promises_.find(event.request_id());
      if (it == decode_promises_.end()) {
        return;
      }

      promise = std::move(it->second);
      decode_promises_.erase(it);
    }

    // Set the value
    promise.set_value(event.value());
  }

  /**
   * @brief Process an error event
   *
   * @param event Error event
   */
  void process_error(const BencodeErrorEvent &event) {
    // Check if there's an encode promise
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = encode_promises_.find(event.request_id());
      if (it != encode_promises_.end()) {
        // Set the exception
        it->second.set_exception(
            std::make_exception_ptr(std::runtime_error(event.error_message())));

        // Remove the promise
        encode_promises_.erase(it);
        return;
      }
    }

    // Check if there's a decode promise
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = decode_promises_.find(event.request_id());
      if (it != decode_promises_.end()) {
        // Set the exception
        it->second.set_exception(
            std::make_exception_ptr(std::runtime_error(event.error_message())));

        // Remove the promise
        decode_promises_.erase(it);
        return;
      }
    }
  }

  std::atomic<bool> running_; ///< Flag indicating if the processor is running
  event::EventBus *event_bus_ = nullptr; ///< Event bus to process events from
  types::SubscriptionToken token_; ///< Subscription token for the event bus
  std::atomic<uint64_t> next_request_id_; ///< Next request ID
  std::mutex mutex_;                      ///< Mutex for thread safety
  std::unordered_map<uint64_t, std::promise<std::vector<uint8_t>>>
      encode_promises_; ///< Promises for encode requests
  std::unordered_map<uint64_t, std::promise<BencodeValue>>
      decode_promises_;                     ///< Promises for decode requests
  std::unique_ptr<BencodeEncoder> encoder_; ///< Bencode encoder
  std::unique_ptr<BencodeDecoder> decoder_; ///< Bencode decoder
};

std::unique_ptr<BencodeEventProcessor> create_bencode_event_processor() {
  return std::make_unique<BencodeEventProcessorImpl>();
}

} // namespace bitscrape::bencode
