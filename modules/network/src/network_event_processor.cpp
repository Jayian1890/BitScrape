#include "bitscrape/network/network_event_processor.hpp"

#include "bitscrape/event/event_bus.hpp"
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

std::unique_ptr<types::Event> UDPSendEvent::clone() const {
  return std::make_unique<UDPSendEvent>(*this);
}

// UDPReceiveEvent implementation
UDPReceiveEvent::UDPReceiveEvent(const Buffer &buffer, const Address &address)
    : NetworkEvent(NetworkEventType::UDP_RECEIVE), buffer_(buffer),
      address_(address) {}

const Buffer &UDPReceiveEvent::buffer() const { return buffer_; }

const Address &UDPReceiveEvent::address() const { return address_; }

std::unique_ptr<types::Event> UDPReceiveEvent::clone() const {
  return std::make_unique<UDPReceiveEvent>(*this);
}

// TCPConnectEvent implementation
TCPConnectEvent::TCPConnectEvent(const Address &address)
    : NetworkEvent(NetworkEventType::TCP_CONNECT), address_(address) {}

const Address &TCPConnectEvent::address() const { return address_; }

std::unique_ptr<types::Event> TCPConnectEvent::clone() const {
  return std::make_unique<TCPConnectEvent>(*this);
}

// TCPSendEvent implementation
TCPSendEvent::TCPSendEvent(const Buffer &buffer)
    : NetworkEvent(NetworkEventType::TCP_SEND), buffer_(buffer) {}

const Buffer &TCPSendEvent::buffer() const { return buffer_; }

std::unique_ptr<types::Event> TCPSendEvent::clone() const {
  return std::make_unique<TCPSendEvent>(*this);
}

// TCPReceiveEvent implementation
TCPReceiveEvent::TCPReceiveEvent(const Buffer &buffer)
    : NetworkEvent(NetworkEventType::TCP_RECEIVE), buffer_(buffer) {}

const Buffer &TCPReceiveEvent::buffer() const { return buffer_; }

std::unique_ptr<types::Event> TCPReceiveEvent::clone() const {
  return std::make_unique<TCPReceiveEvent>(*this);
}

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

std::unique_ptr<types::Event> HTTPRequestEvent::clone() const {
  return std::make_unique<HTTPRequestEvent>(*this);
}

// HTTPResponseEvent implementation
HTTPResponseEvent::HTTPResponseEvent(const HTTPResponse &response)
    : NetworkEvent(NetworkEventType::HTTP_RESPONSE), response_(response) {}

const HTTPResponse &HTTPResponseEvent::response() const { return response_; }

std::unique_ptr<types::Event> HTTPResponseEvent::clone() const {
  return std::make_unique<HTTPResponseEvent>(*this);
}

// UDPSendResultEvent implementation
UDPSendResultEvent::UDPSendResultEvent(int bytes_sent, const Address &address)
    : NetworkEvent(NetworkEventType::UDP_SEND_RESULT), bytes_sent_(bytes_sent),
      address_(address) {}

int UDPSendResultEvent::bytes_sent() const { return bytes_sent_; }

const Address &UDPSendResultEvent::address() const { return address_; }

bool UDPSendResultEvent::is_success() const { return bytes_sent_ > 0; }

std::unique_ptr<types::Event> UDPSendResultEvent::clone() const {
  return std::make_unique<UDPSendResultEvent>(*this);
}

// TCPConnectResultEvent implementation
TCPConnectResultEvent::TCPConnectResultEvent(bool success, const Address &address)
    : NetworkEvent(NetworkEventType::TCP_CONNECT_RESULT), success_(success),
      address_(address) {}

bool TCPConnectResultEvent::is_success() const { return success_; }

const Address &TCPConnectResultEvent::address() const { return address_; }

std::unique_ptr<types::Event> TCPConnectResultEvent::clone() const {
  return std::make_unique<TCPConnectResultEvent>(*this);
}

// TCPSendResultEvent implementation
TCPSendResultEvent::TCPSendResultEvent(int bytes_sent)
    : NetworkEvent(NetworkEventType::TCP_SEND_RESULT), bytes_sent_(bytes_sent) {}

int TCPSendResultEvent::bytes_sent() const { return bytes_sent_; }

bool TCPSendResultEvent::is_success() const { return bytes_sent_ > 0; }

std::unique_ptr<types::Event> TCPSendResultEvent::clone() const {
  return std::make_unique<TCPSendResultEvent>(*this);
}

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

    // Publish UDPSendResultEvent with the result of the send operation
    if (event_bus_) {
      event_bus_->publish(UDPSendResultEvent(bytes_sent, send_event.address()));
    }

    return true;
  }

  case NetworkEventType::UDP_RECEIVE: {
    // This is a bit different, as we would typically set up a receive loop
    // For now, we'll just handle a single receive
    Buffer buffer;
    Address address;
    int bytes_received = udp_socket_->receive_from(buffer, address);

    if (bytes_received > 0 && event_bus_) {
      // Publish UDPReceiveEvent with the received data
      event_bus_->publish(UDPReceiveEvent(buffer, address));
    }

    return true;
  }

  case NetworkEventType::TCP_CONNECT: {
    const TCPConnectEvent &connect_event =
        static_cast<const TCPConnectEvent &>(network_event);
    bool success = tcp_socket_->connect(connect_event.address());

    // Publish TCPConnectResultEvent with the result of the connect operation
    if (event_bus_) {
      event_bus_->publish(TCPConnectResultEvent(success, connect_event.address()));
    }

    return true;
  }

  case NetworkEventType::TCP_SEND: {
    const TCPSendEvent &send_event =
        static_cast<const TCPSendEvent &>(network_event);
    int bytes_sent = tcp_socket_->send(send_event.buffer());

    // Publish TCPSendResultEvent with the result of the send operation
    if (event_bus_) {
      event_bus_->publish(TCPSendResultEvent(bytes_sent));
    }

    return true;
  }

  case NetworkEventType::TCP_RECEIVE: {
    // This is a bit different, as we would typically set up a receive loop
    // For now, we'll just handle a single receive
    Buffer buffer;
    int bytes_received = tcp_socket_->receive(buffer);

    if (bytes_received > 0 && event_bus_) {
      // Publish TCPReceiveEvent with the received data
      event_bus_->publish(TCPReceiveEvent(buffer));
    }

    return true;
  }

  case NetworkEventType::HTTP_REQUEST: {
    const HTTPRequestEvent &request_event =
        static_cast<const HTTPRequestEvent &>(network_event);
    HTTPResponse response =
        http_client_->request(request_event.method(), request_event.url(),
                              request_event.headers(), request_event.body());

    // Publish HTTPResponseEvent with the response data
    if (event_bus_) {
      event_bus_->publish(HTTPResponseEvent(response));
    }

    return true;
  }

  default:
    return false;
  }
}

} // namespace bitscrape::network
