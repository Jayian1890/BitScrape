#include "bitscrape/network/network_event_processor.hpp"

#include "bitscrape/event/event_bus.hpp"
#include "bitscrape/event/event_processor.hpp"
#include "bitscrape/types/event_types.hpp"

#include <atomic>
#include <future>
#include <map>
#include <memory>
#include <string>

namespace bitscrape::network {

// NetworkEvent implementation
NetworkEvent::NetworkEvent(NetworkEventType type)
    : types::Event(types::Event::Type::CUSTOM, static_cast<uint32_t>(type)),
      network_event_type_(type) {}

NetworkEventType NetworkEvent::network_event_type() const {
  return network_event_type_;
}

// UDPSendEvent implementation
UDPSendEvent::UDPSendEvent(const Buffer &buffer, const Address &address)
    : NetworkEvent(NetworkEventType::UDP_SEND), buffer_(buffer),
      address_(address) {}

const Buffer &UDPSendEvent::buffer() const { return buffer_; }

const Address &UDPSendEvent::address() const { return address_; }

// UDPReceiveEvent implementation
UDPReceiveEvent::UDPReceiveEvent(const Buffer &buffer, const Address &address)
    : NetworkEvent(NetworkEventType::UDP_RECEIVE), buffer_(buffer),
      address_(address) {}

const Buffer &UDPReceiveEvent::buffer() const { return buffer_; }

const Address &UDPReceiveEvent::address() const { return address_; }

// TCPConnectEvent implementation
TCPConnectEvent::TCPConnectEvent(const Address &address)
    : NetworkEvent(NetworkEventType::TCP_CONNECT), address_(address) {}

const Address &TCPConnectEvent::address() const { return address_; }

// TCPSendEvent implementation
TCPSendEvent::TCPSendEvent(const Buffer &buffer)
    : NetworkEvent(NetworkEventType::TCP_SEND), buffer_(buffer) {}

const Buffer &TCPSendEvent::buffer() const { return buffer_; }

// TCPReceiveEvent implementation
TCPReceiveEvent::TCPReceiveEvent(const Buffer &buffer)
    : NetworkEvent(NetworkEventType::TCP_RECEIVE), buffer_(buffer) {}

const Buffer &TCPReceiveEvent::buffer() const { return buffer_; }

// HTTPRequestEvent implementation
HTTPRequestEvent::HTTPRequestEvent(
    HTTPMethod method, const std::string &url,
    const std::map<std::string, std::string> &headers, const Buffer &body)
    : NetworkEvent(NetworkEventType::HTTP_REQUEST), method_(method), url_(url),
      headers_(headers), body_(body) {}

HTTPMethod HTTPRequestEvent::method() const { return method_; }

const std::string &HTTPRequestEvent::url() const { return url_; }

const std::map<std::string, std::string> &HTTPRequestEvent::headers() const {
  return headers_;
}

const Buffer &HTTPRequestEvent::body() const { return body_; }

// HTTPResponseEvent implementation
HTTPResponseEvent::HTTPResponseEvent(const HTTPResponse &response)
    : NetworkEvent(NetworkEventType::HTTP_REQUEST), response_(response) {}

const HTTPResponse &HTTPResponseEvent::response() const { return response_; }

// NetworkEventProcessor implementation
NetworkEventProcessor::NetworkEventProcessor()
    : running_(false), event_bus_(nullptr),
      udp_socket_(std::make_unique<UDPSocket>()),
      tcp_socket_(std::make_unique<TCPSocket>()),
      http_client_(std::make_unique<HTTPClient>()) {}

void NetworkEventProcessor::start(event::EventBus &event_bus) {
  if (running_) {
    return;
  }

  running_ = true;
  event_bus_ = &event_bus;

  // Subscribe to all network events
  token_ = event_bus_->subscribe<NetworkEvent>(
      [this](const NetworkEvent &event) { this->process(event); });
}

void NetworkEventProcessor::stop() {
  if (!running_) {
    return;
  }

  running_ = false;

  if (event_bus_) {
    event_bus_->unsubscribe(token_);
    event_bus_ = nullptr;
  }
}

bool NetworkEventProcessor::is_running() const { return running_; }

void NetworkEventProcessor::process(const types::Event &event) {
  if (!running_) {
    return;
  }

  // Check if the event is a NetworkEvent
  if (event.type() == types::Event::Type::CUSTOM) {
    // Try to process the event
    process_event(event);
  }
}

std::future<void>
NetworkEventProcessor::process_async(const types::Event &event) {
  return std::async(std::launch::async,
                    [this, event = event.clone()]() { this->process(*event); });
}

bool NetworkEventProcessor::process_event(const types::Event &event) {
  // Check if the event is a NetworkEvent
  if (event.type() < static_cast<uint32_t>(NetworkEventType::UDP_SEND) ||
      event.type() > static_cast<uint32_t>(NetworkEventType::HTTP_REQUEST)) {
    return false;
  }

  // Cast to NetworkEvent
  const NetworkEvent &network_event = static_cast<const NetworkEvent &>(event);

  // Process based on event type
  switch (network_event.network_event_type()) {
  case NetworkEventType::UDP_SEND: {
    const UDPSendEvent &send_event =
        static_cast<const UDPSendEvent &>(network_event);
    int bytes_sent =
        udp_socket_->send_to(send_event.buffer(), send_event.address());

    // Publish result event
    // In a real implementation, we would publish a UDPSendResultEvent

    return true;
  }

  case NetworkEventType::UDP_RECEIVE: {
    // This is a bit different, as we would typically set up a receive loop
    // For now, we'll just handle a single receive
    Buffer buffer;
    Address address;
    int bytes_received = udp_socket_->receive_from(buffer, address);

    if (bytes_received > 0) {
      // Publish receive event
      event::EventBus::instance().publish(UDPReceiveEvent(buffer, address));
    }

    return true;
  }

  case NetworkEventType::TCP_CONNECT: {
    const TCPConnectEvent &connect_event =
        static_cast<const TCPConnectEvent &>(network_event);
    bool success = tcp_socket_->connect(connect_event.address());

    // Publish result event
    // In a real implementation, we would publish a TCPConnectResultEvent

    return true;
  }

  case NetworkEventType::TCP_SEND: {
    const TCPSendEvent &send_event =
        static_cast<const TCPSendEvent &>(network_event);
    int bytes_sent = tcp_socket_->send(send_event.buffer());

    // Publish result event
    // In a real implementation, we would publish a TCPSendResultEvent

    return true;
  }

  case NetworkEventType::TCP_RECEIVE: {
    // This is a bit different, as we would typically set up a receive loop
    // For now, we'll just handle a single receive
    Buffer buffer;
    int bytes_received = tcp_socket_->receive(buffer);

    if (bytes_received > 0) {
      // Publish receive event
      event::EventBus::instance().publish(TCPReceiveEvent(buffer));
    }

    return true;
  }

  case NetworkEventType::HTTP_REQUEST: {
    const HTTPRequestEvent &request_event =
        static_cast<const HTTPRequestEvent &>(network_event);
    HTTPResponse response =
        http_client_->request(request_event.method(), request_event.url(),
                              request_event.headers(), request_event.body());

    // Publish response event
    event::EventBus::instance().publish(HTTPResponseEvent(response));

    return true;
  }

  default:
    return false;
  }
}

} // namespace bitscrape::network
