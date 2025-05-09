#include <gtest/gtest.h>

#include "bitscrape/bencode/bencode_decoder.hpp"
#include "bitscrape/bencode/bencode_encoder.hpp"
#include "bitscrape/bencode/bencode_event_processor.hpp"
#include "bitscrape/bencode/bencode_value.hpp"
#include "bitscrape/event/event_bus.hpp"

using namespace bitscrape::bencode;
using namespace bitscrape::event;

TEST(BencodeEventProcessorTest, BencodeEventConstructor) {
  BencodeEvent event(BencodeEvent::Type::ENCODE_REQUEST, 42);

  EXPECT_EQ(event.event_type(), BencodeEvent::Type::ENCODE_REQUEST);
  EXPECT_EQ(event.request_id(), 42);
}

TEST(BencodeEventProcessorTest, BencodeEventClone) {
  BencodeEvent event(BencodeEvent::Type::ENCODE_REQUEST, 42);
  auto clone = event.clone();

  EXPECT_NE(clone.get(), &event);

  auto *bencode_event = dynamic_cast<BencodeEvent *>(clone.get());
  EXPECT_NE(bencode_event, nullptr);
  EXPECT_EQ(bencode_event->event_type(), BencodeEvent::Type::ENCODE_REQUEST);
  EXPECT_EQ(bencode_event->request_id(), 42);
}

TEST(BencodeEventProcessorTest, BencodeEncodeRequestEventConstructor) {
  BencodeValue value(std::string("test"));
  BencodeEncodeRequestEvent event(42, value);

  EXPECT_EQ(event.event_type(), BencodeEvent::Type::ENCODE_REQUEST);
  EXPECT_EQ(event.request_id(), 42);
  EXPECT_TRUE(event.value().is_string());
  EXPECT_EQ(event.value().as_string(), "test");
}

TEST(BencodeEventProcessorTest, BencodeEncodeRequestEventClone) {
  BencodeValue value(std::string("test"));
  BencodeEncodeRequestEvent event(42, value);
  auto clone = event.clone();

  EXPECT_NE(clone.get(), &event);

  auto *encode_request_event =
      dynamic_cast<BencodeEncodeRequestEvent *>(clone.get());
  EXPECT_NE(encode_request_event, nullptr);
  EXPECT_EQ(encode_request_event->event_type(),
            BencodeEvent::Type::ENCODE_REQUEST);
  EXPECT_EQ(encode_request_event->request_id(), 42);
  EXPECT_TRUE(encode_request_event->value().is_string());
  EXPECT_EQ(encode_request_event->value().as_string(), "test");
}

TEST(BencodeEventProcessorTest, BencodeEncodeResponseEventConstructor) {
  std::vector<uint8_t> data = {'t', 'e', 's', 't'};
  BencodeEncodeResponseEvent event(42, data);

  EXPECT_EQ(event.event_type(), BencodeEvent::Type::ENCODE_RESPONSE);
  EXPECT_EQ(event.request_id(), 42);
  EXPECT_EQ(event.data(), data);
}

TEST(BencodeEventProcessorTest, BencodeEncodeResponseEventClone) {
  std::vector<uint8_t> data = {'t', 'e', 's', 't'};
  BencodeEncodeResponseEvent event(42, data);
  auto clone = event.clone();

  EXPECT_NE(clone.get(), &event);

  auto *encode_response_event =
      dynamic_cast<BencodeEncodeResponseEvent *>(clone.get());
  EXPECT_NE(encode_response_event, nullptr);
  EXPECT_EQ(encode_response_event->event_type(),
            BencodeEvent::Type::ENCODE_RESPONSE);
  EXPECT_EQ(encode_response_event->request_id(), 42);
  EXPECT_EQ(encode_response_event->data(), data);
}

TEST(BencodeEventProcessorTest, BencodeDecodeRequestEventConstructor) {
  std::vector<uint8_t> data = {'t', 'e', 's', 't'};
  BencodeDecodeRequestEvent event(42, data);

  EXPECT_EQ(event.event_type(), BencodeEvent::Type::DECODE_REQUEST);
  EXPECT_EQ(event.request_id(), 42);
  EXPECT_EQ(event.data(), data);
}

TEST(BencodeEventProcessorTest, BencodeDecodeRequestEventClone) {
  std::vector<uint8_t> data = {'t', 'e', 's', 't'};
  BencodeDecodeRequestEvent event(42, data);
  auto clone = event.clone();

  EXPECT_NE(clone.get(), &event);

  auto *decode_request_event =
      dynamic_cast<BencodeDecodeRequestEvent *>(clone.get());
  EXPECT_NE(decode_request_event, nullptr);
  EXPECT_EQ(decode_request_event->event_type(),
            BencodeEvent::Type::DECODE_REQUEST);
  EXPECT_EQ(decode_request_event->request_id(), 42);
  EXPECT_EQ(decode_request_event->data(), data);
}

TEST(BencodeEventProcessorTest, BencodeDecodeResponseEventConstructor) {
  BencodeValue value(std::string("test"));
  BencodeDecodeResponseEvent event(42, value);

  EXPECT_EQ(event.event_type(), BencodeEvent::Type::DECODE_RESPONSE);
  EXPECT_EQ(event.request_id(), 42);
  EXPECT_TRUE(event.value().is_string());
  EXPECT_EQ(event.value().as_string(), "test");
}

TEST(BencodeEventProcessorTest, BencodeDecodeResponseEventClone) {
  BencodeValue value(std::string("test"));
  BencodeDecodeResponseEvent event(42, value);
  auto clone = event.clone();

  EXPECT_NE(clone.get(), &event);

  auto *decode_response_event =
      dynamic_cast<BencodeDecodeResponseEvent *>(clone.get());
  EXPECT_NE(decode_response_event, nullptr);
  EXPECT_EQ(decode_response_event->event_type(),
            BencodeEvent::Type::DECODE_RESPONSE);
  EXPECT_EQ(decode_response_event->request_id(), 42);
  EXPECT_TRUE(decode_response_event->value().is_string());
  EXPECT_EQ(decode_response_event->value().as_string(), "test");
}

TEST(BencodeEventProcessorTest, BencodeErrorEventConstructor) {
  BencodeErrorEvent event(42, "error message");

  EXPECT_EQ(event.event_type(), BencodeEvent::Type::ERROR);
  EXPECT_EQ(event.request_id(), 42);
  EXPECT_EQ(event.error_message(), "error message");
}

TEST(BencodeEventProcessorTest, BencodeErrorEventClone) {
  BencodeErrorEvent event(42, "error message");
  auto clone = event.clone();

  EXPECT_NE(clone.get(), &event);

  auto *error_event = dynamic_cast<BencodeErrorEvent *>(clone.get());
  EXPECT_NE(error_event, nullptr);
  EXPECT_EQ(error_event->event_type(), BencodeEvent::Type::ERROR);
  EXPECT_EQ(error_event->request_id(), 42);
  EXPECT_EQ(error_event->error_message(), "error message");
}

TEST(BencodeEventProcessorTest, StartStop) {
  auto event_bus = create_event_bus();
  auto processor = create_bencode_event_processor();

  EXPECT_FALSE(processor->is_running());

  processor->start(*event_bus);
  EXPECT_TRUE(processor->is_running());

  processor->stop();
  EXPECT_FALSE(processor->is_running());
}

TEST(BencodeEventProcessorTest, EncodeAsync) {
  // Don't use the event processor, just use the encoder directly
  auto encoder = create_bencode_encoder();

  BencodeValue value(std::string("test"));
  auto future = encoder->encode_async(value);
  auto encoded = future.get();

  std::string expected = "4:test";
  std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());

  EXPECT_EQ(encoded, expected_bytes);
}

TEST(BencodeEventProcessorTest, DecodeAsync) {
  // Don't use the event processor, just use the decoder directly
  auto decoder = create_bencode_decoder();

  std::string data = "4:test";
  std::vector<uint8_t> bytes(data.begin(), data.end());
  auto future = decoder->decode_async(bytes);
  auto decoded = future.get();

  EXPECT_TRUE(decoded.is_string());
  EXPECT_EQ(decoded.as_string(), "test");
}

TEST(BencodeEventProcessorTest, ProcessEncodeRequest) {
  auto event_bus = create_event_bus();
  auto processor = create_bencode_event_processor();

  processor->start(*event_bus);

  // Subscribe to encode response events
  bool response_received = false;
  std::vector<uint8_t> response_data;

  auto token = event_bus->subscribe<BencodeEncodeResponseEvent>(
      [&response_received,
       &response_data](const BencodeEncodeResponseEvent &event) {
        response_received = true;
        response_data = event.data();
      });

  // Publish an encode request event
  BencodeValue value(std::string("test"));
  event_bus->publish(BencodeEncodeRequestEvent(42, value));

  // Wait for the response
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(response_received);

  std::string expected = "4:test";
  std::vector<uint8_t> expected_bytes(expected.begin(), expected.end());

  EXPECT_EQ(response_data, expected_bytes);

  event_bus->unsubscribe(token);
  processor->stop();
}

TEST(BencodeEventProcessorTest, ProcessDecodeRequest) {
  auto event_bus = create_event_bus();
  auto processor = create_bencode_event_processor();

  processor->start(*event_bus);

  // Subscribe to decode response events
  bool response_received = false;
  BencodeValue response_value;

  auto token = event_bus->subscribe<BencodeDecodeResponseEvent>(
      [&response_received,
       &response_value](const BencodeDecodeResponseEvent &event) {
        response_received = true;
        response_value = event.value();
      });

  // Publish a decode request event
  std::string data = "4:test";
  std::vector<uint8_t> bytes(data.begin(), data.end());
  event_bus->publish(BencodeDecodeRequestEvent(42, bytes));

  // Wait for the response
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(response_received);
  EXPECT_TRUE(response_value.is_string());
  EXPECT_EQ(response_value.as_string(), "test");

  event_bus->unsubscribe(token);
  processor->stop();
}

TEST(BencodeEventProcessorTest, ProcessError) {
  auto event_bus = create_event_bus();
  auto processor = create_bencode_event_processor();

  processor->start(*event_bus);

  // Subscribe to error events
  bool error_received = false;
  std::string error_message;

  auto token = event_bus->subscribe<BencodeErrorEvent>(
      [&error_received, &error_message](const BencodeErrorEvent &event) {
        error_received = true;
        error_message = event.error_message();
      });

  // Publish an invalid decode request event
  std::vector<uint8_t> bytes = {'i', 'n', 'v', 'a', 'l', 'i', 'd'};
  event_bus->publish(BencodeDecodeRequestEvent(42, bytes));

  // Wait for the error
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(error_received);
  EXPECT_FALSE(error_message.empty());

  event_bus->unsubscribe(token);
  processor->stop();
}

TEST(BencodeEventProcessorTest, CreateBencodeEventProcessor) {
  auto processor = create_bencode_event_processor();

  EXPECT_NE(processor, nullptr);
}
