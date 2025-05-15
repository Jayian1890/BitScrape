#pragma once

#include "bitscrape/event/event_processor.hpp"
#include "bitscrape/network/address.hpp"
#include "bitscrape/network/buffer.hpp"
#include "bitscrape/network/http_client.hpp"
#include "bitscrape/network/tcp_socket.hpp"
#include "bitscrape/network/udp_socket.hpp"
#include "bitscrape/types/event_types.hpp"
#include "bitscrape/lock/lock_manager_singleton.hpp"
#include "bitscrape/lock/lock_guard.hpp"

#include <future>
#include <memory>
#include <string>

namespace bitscrape::network {

/**
 * @brief Network event types
 */
enum class NetworkEventType {
  UDP_SEND,
  UDP_SEND_RESULT,
  UDP_RECEIVE,
  TCP_CONNECT,
  TCP_CONNECT_RESULT,
  TCP_SEND,
  TCP_SEND_RESULT,
  TCP_RECEIVE,
  HTTP_REQUEST,
  HTTP_RESPONSE
};

/**
 * @brief Base class for network events
 */
class NetworkEvent : public types::Event {
public:
  /**
   * @brief Construct a new NetworkEvent object
   *
   * @param type The network event type
   */
  explicit NetworkEvent(NetworkEventType type);

  /**
   * @brief Get the network event type
   *
   * @return The network event type
   */
  NetworkEventType network_event_type() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override = 0;

private:
  NetworkEventType network_event_type_;
};

/**
 * @brief UDP send event
 */
class UDPSendEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new UDPSendEvent object
   *
   * @param buffer The buffer to send
   * @param address The destination address
   */
  UDPSendEvent(const Buffer &buffer, const Address &address);

  /**
   * @brief Get the buffer
   *
   * @return The buffer
   */
  const Buffer &buffer() const;

  /**
   * @brief Get the address
   *
   * @return The address
   */
  const Address &address() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  Buffer buffer_;
  Address address_;
};

/**
 * @brief UDP receive event
 */
class UDPReceiveEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new UDPReceiveEvent object
   *
   * @param buffer The received buffer
   * @param address The source address
   */
  UDPReceiveEvent(const Buffer &buffer, const Address &address);

  /**
   * @brief Get the buffer
   *
   * @return The buffer
   */
  const Buffer &buffer() const;

  /**
   * @brief Get the address
   *
   * @return The address
   */
  const Address &address() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  Buffer buffer_;
  Address address_;
};

/**
 * @brief TCP connect event
 */
class TCPConnectEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new TCPConnectEvent object
   *
   * @param address The address to connect to
   */
  explicit TCPConnectEvent(const Address &address);

  /**
   * @brief Get the address
   *
   * @return The address
   */
  const Address &address() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  Address address_;
};

/**
 * @brief TCP send event
 */
class TCPSendEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new TCPSendEvent object
   *
   * @param buffer The buffer to send
   */
  explicit TCPSendEvent(const Buffer &buffer);

  /**
   * @brief Get the buffer
   *
   * @return The buffer
   */
  const Buffer &buffer() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  Buffer buffer_;
};

/**
 * @brief TCP receive event
 */
class TCPReceiveEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new TCPReceiveEvent object
   *
   * @param buffer The received buffer
   */
  explicit TCPReceiveEvent(const Buffer &buffer);

  /**
   * @brief Get the buffer
   *
   * @return The buffer
   */
  const Buffer &buffer() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  Buffer buffer_;
};

/**
 * @brief HTTP request event
 */
class HTTPRequestEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new HTTPRequestEvent object
   *
   * @param method The HTTP method
   * @param url The URL to request
   * @param headers The request headers
   * @param body The request body
   */
  HTTPRequestEvent(HTTPMethod method, const std::string &url,
                   const std::map<std::string, std::string> &headers = {},
                   const Buffer &body = Buffer());

  /**
   * @brief Get the HTTP method
   *
   * @return The HTTP method
   */
  HTTPMethod method() const;

  /**
   * @brief Get the URL
   *
   * @return The URL
   */
  const std::string &url() const;

  /**
   * @brief Get the headers
   *
   * @return The headers
   */
  const std::map<std::string, std::string> &headers() const;

  /**
   * @brief Get the body
   *
   * @return The body
   */
  const Buffer &body() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  HTTPMethod method_;
  std::string url_;
  std::map<std::string, std::string> headers_;
  Buffer body_;
};

/**
 * @brief UDP send result event
 */
class UDPSendResultEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new UDPSendResultEvent object
   *
   * @param bytes_sent Number of bytes sent, or -1 on error
   * @param address The destination address
   */
  UDPSendResultEvent(int bytes_sent, const Address &address);

  /**
   * @brief Get the number of bytes sent
   *
   * @return Number of bytes sent, or -1 on error
   */
  int bytes_sent() const;

  /**
   * @brief Get the address
   *
   * @return The address
   */
  const Address &address() const;

  /**
   * @brief Check if the send was successful
   *
   * @return true if the send was successful, false otherwise
   */
  bool is_success() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  int bytes_sent_;
  Address address_;
};

/**
 * @brief TCP connect result event
 */
class TCPConnectResultEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new TCPConnectResultEvent object
   *
   * @param success Whether the connection was successful
   * @param address The address that was connected to
   */
  TCPConnectResultEvent(bool success, const Address &address);

  /**
   * @brief Check if the connection was successful
   *
   * @return true if the connection was successful, false otherwise
   */
  bool is_success() const;

  /**
   * @brief Get the address
   *
   * @return The address
   */
  const Address &address() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  bool success_;
  Address address_;
};

/**
 * @brief TCP send result event
 */
class TCPSendResultEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new TCPSendResultEvent object
   *
   * @param bytes_sent Number of bytes sent, or -1 on error
   */
  explicit TCPSendResultEvent(int bytes_sent);

  /**
   * @brief Get the number of bytes sent
   *
   * @return Number of bytes sent, or -1 on error
   */
  int bytes_sent() const;

  /**
   * @brief Check if the send was successful
   *
   * @return true if the send was successful, false otherwise
   */
  bool is_success() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  int bytes_sent_;
};

/**
 * @brief HTTP response event
 */
class HTTPResponseEvent : public NetworkEvent {
public:
  /**
   * @brief Construct a new HTTPResponseEvent object
   *
   * @param response The HTTP response
   */
  explicit HTTPResponseEvent(const HTTPResponse &response);

  /**
   * @brief Get the HTTP response
   *
   * @return The HTTP response
   */
  const HTTPResponse &response() const;

  /**
   * @brief Clone the event
   *
   * @return A unique pointer to a copy of the event
   */
  std::unique_ptr<types::Event> clone() const override;

private:
  HTTPResponse response_;
};

/**
 * @brief NetworkEventProcessor class for processing network events
 *
 * This class processes network events and performs the corresponding network
 * operations. It supports both synchronous and asynchronous event processing.
 */
class NetworkEventProcessor : public event::EventProcessor {
public:
  /**
   * @brief Construct a new NetworkEventProcessor object
   */
  NetworkEventProcessor();

  /**
   * @brief Destroy the NetworkEventProcessor object
   */
  ~NetworkEventProcessor() override = default;

  /**
   * @brief Start processing events
   *
   * @param event_bus Event bus to process events from
   */
  void start(event::EventBus &event_bus) override;

  /**
   * @brief Stop processing events
   */
  void stop() override;

  /**
   * @brief Check if the processor is running
   *
   * @return true if the processor is running, false otherwise
   */
  bool is_running() const override;

  /**
   * @brief Process an event
   *
   * @param event Event to process
   */
  void process(const types::Event &event) override;

  /**
   * @brief Process an event asynchronously
   *
   * @param event Event to process
   * @return Future that will be completed when the event has been processed
   */
  std::future<void> process_async(const types::Event &event) override;

  /**
   * @brief Process an event
   *
   * @param event The event to process
   * @return true if the event was processed, false otherwise
   */
  bool process_event(const types::Event &event);

private:
  bool running_;
  uint64_t processor_state_resource_id_; // Resource ID for the processor state
  event::EventBus *event_bus_;
  types::SubscriptionToken token_;
  std::unique_ptr<UDPSocket> udp_socket_;
  std::unique_ptr<TCPSocket> tcp_socket_;
  std::unique_ptr<HTTPClient> http_client_;
};

} // namespace bitscrape::network
