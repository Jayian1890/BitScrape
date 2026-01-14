#include "bitscrape/tracker/udp_tracker.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>
#include <sstream>
#include <stdexcept>

namespace bitscrape::tracker {

namespace {

// Protocol constants
constexpr uint64_t CONNECTION_ID_INIT = 0x41727101980ULL;
constexpr int MAX_RETRIES = 8;
constexpr int BASE_TIMEOUT_MS = 15000; // 15 seconds

// Generate a random transaction ID
uint32_t generate_transaction_id() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis;
  return dis(gen);
}

// Convert a 4-byte array to uint32_t (big-endian)
uint32_t bytes_to_uint32(const uint8_t *bytes) {
  return (static_cast<uint32_t>(bytes[0]) << 24) |
         (static_cast<uint32_t>(bytes[1]) << 16) |
         (static_cast<uint32_t>(bytes[2]) << 8) |
         static_cast<uint32_t>(bytes[3]);
}

// Convert a 8-byte array to uint64_t (big-endian)
uint64_t bytes_to_uint64(const uint8_t *bytes) {
  return (static_cast<uint64_t>(bytes[0]) << 56) |
         (static_cast<uint64_t>(bytes[1]) << 48) |
         (static_cast<uint64_t>(bytes[2]) << 40) |
         (static_cast<uint64_t>(bytes[3]) << 32) |
         (static_cast<uint64_t>(bytes[4]) << 24) |
         (static_cast<uint64_t>(bytes[5]) << 16) |
         (static_cast<uint64_t>(bytes[6]) << 8) |
         static_cast<uint64_t>(bytes[7]);
}

// Convert uint32_t to a 4-byte array (big-endian)
void uint32_to_bytes(uint32_t value, uint8_t *bytes) {
  bytes[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
  bytes[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
  bytes[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
  bytes[3] = static_cast<uint8_t>(value & 0xFF);
}

// Convert uint64_t to a 8-byte array (big-endian)
void uint64_to_bytes(uint64_t value, uint8_t *bytes) {
  bytes[0] = static_cast<uint8_t>((value >> 56) & 0xFF);
  bytes[1] = static_cast<uint8_t>((value >> 48) & 0xFF);
  bytes[2] = static_cast<uint8_t>((value >> 40) & 0xFF);
  bytes[3] = static_cast<uint8_t>((value >> 32) & 0xFF);
  bytes[4] = static_cast<uint8_t>((value >> 24) & 0xFF);
  bytes[5] = static_cast<uint8_t>((value >> 16) & 0xFF);
  bytes[6] = static_cast<uint8_t>((value >> 8) & 0xFF);
  bytes[7] = static_cast<uint8_t>(value & 0xFF);
}

} // namespace

UDPTracker::UDPTracker(std::string url)
    : url_(std::move(url)), socket_(std::make_unique<network::UDPSocket>()),
      connection_id_(0), connection_timeout_ms_(30000),
      request_timeout_ms_(30000), transaction_id_(0) {
  // Parse the URL to get the host and port
  auto [host, port] = parse_url(url_);
  address_ = network::Address(host, port);
}

UDPTracker::~UDPTracker() = default;

const std::string &UDPTracker::url() const { return url_; }

void UDPTracker::set_url(const std::string &url) {
  url_ = url;

  // Parse the URL to get the host and port
  auto [host, port] = parse_url(url_);
  address_ = network::Address(host, port);
}

AnnounceResponse UDPTracker::announce(const AnnounceRequest &request) {
  try {
    // Get a connection ID
    uint64_t connection_id = connect();

    // Create the announce request packet
    network::Buffer buffer(98); // Max size for announce request
    buffer.resize(98);

    // Connection ID (8 bytes)
    uint64_to_bytes(connection_id, buffer.data());

    // Action (4 bytes): announce = 1
    uint32_to_bytes(static_cast<uint32_t>(Action::ANNOUNCE), buffer.data() + 8);

    // Transaction ID (4 bytes)
    uint32_t transaction_id = transaction_id_++;
    uint32_to_bytes(transaction_id, buffer.data() + 12);

    // Info hash (20 bytes)
    std::memcpy(buffer.data() + 16, request.info_hash().bytes().data(), 20);

    // Peer ID (20 bytes)
    std::memcpy(buffer.data() + 36, request.peer_id().data(),
                std::min(request.peer_id().size(), static_cast<size_t>(20)));

    // Downloaded (8 bytes)
    uint64_to_bytes(request.downloaded(), buffer.data() + 56);

    // Left (8 bytes)
    uint64_to_bytes(request.left(), buffer.data() + 64);

    // Uploaded (8 bytes)
    uint64_to_bytes(request.uploaded(), buffer.data() + 72);

    // Event (4 bytes)
    uint32_t event = 0; // 0 = none, 1 = completed, 2 = started, 3 = stopped
    if (request.event() == "completed") {
      event = 1;
    } else if (request.event() == "started") {
      event = 2;
    } else if (request.event() == "stopped") {
      event = 3;
    }
    uint32_to_bytes(event, buffer.data() + 80);

    // IP address (4 bytes): 0 = use sender's address
    uint32_to_bytes(0, buffer.data() + 84);

    // Key (4 bytes): random value
    uint32_t key = 0;
    try {
      key = std::stoul(request.key());
    } catch (...) {
      key = generate_transaction_id();
    }
    uint32_to_bytes(key, buffer.data() + 88);

    // Num want (4 bytes): -1 = default
    uint32_to_bytes(static_cast<uint32_t>(request.numwant()),
                    buffer.data() + 92);

    // Port (2 bytes)
    buffer.data()[96] = static_cast<uint8_t>((request.port() >> 8) & 0xFF);
    buffer.data()[97] = static_cast<uint8_t>(request.port() & 0xFF);

    // Send the request
    int bytes_sent = send_packet(buffer);
    if (bytes_sent != static_cast<int>(buffer.size())) {
      AnnounceResponse response;
      response.set_error_message("Failed to send announce request");
      return response;
    }

    // Receive the response
    network::Buffer response_buffer;
    int bytes_received = receive_packet(response_buffer, request_timeout_ms_);
    if (bytes_received < 20) {
      AnnounceResponse response;
      response.set_error_message("Invalid announce response: too short");
      return response;
    }

    // Parse the response
    AnnounceResponse response;

    // Check the action
    uint32_t action = bytes_to_uint32(response_buffer.data());
    if (action == static_cast<uint32_t>(Action::ERROR)) {
      // Error response
      if (bytes_received >= 8) {
        std::string error_message(
            reinterpret_cast<const char *>(response_buffer.data() + 8),
            bytes_received - 8);
        response.set_error_message(error_message);
      } else {
        response.set_error_message("Unknown error");
      }
      return response;
    } else if (action != static_cast<uint32_t>(Action::ANNOUNCE)) {
      response.set_error_message("Invalid announce response: wrong action");
      return response;
    }

    // Check the transaction ID
    uint32_t response_transaction_id =
        bytes_to_uint32(response_buffer.data() + 4);
    if (response_transaction_id != transaction_id) {
      response.set_error_message(
          "Invalid announce response: wrong transaction ID");
      return response;
    }

    // Parse the response fields
    response.set_interval(
        static_cast<int>(bytes_to_uint32(response_buffer.data() + 8)));
    response.set_incomplete(
        static_cast<int>(bytes_to_uint32(response_buffer.data() + 12)));
    response.set_complete(
        static_cast<int>(bytes_to_uint32(response_buffer.data() + 16)));

    // Parse the peers
    int num_peers = (bytes_received - 20) / 6;
    for (int i = 0; i < num_peers; ++i) {
      const uint8_t *peer_data = response_buffer.data() + 20 + i * 6;

      // Extract IP (4 bytes)
      std::ostringstream ip_stream;
      ip_stream << static_cast<int>(peer_data[0]) << "."
                << static_cast<int>(peer_data[1]) << "."
                << static_cast<int>(peer_data[2]) << "."
                << static_cast<int>(peer_data[3]);

      // Extract port (2 bytes, big-endian)
      uint16_t port = (static_cast<uint16_t>(peer_data[4]) << 8) |
                      static_cast<uint16_t>(peer_data[5]);

      // Add peer
      response.add_peer(network::Address(ip_stream.str(), port));
    }

    return response;
  } catch (const std::exception &e) {
    AnnounceResponse response;
    response.set_error_message("Exception: " + std::string(e.what()));
    return response;
  }
}

std::future<AnnounceResponse>
UDPTracker::announce_async(const AnnounceRequest &request) {
  return std::async(std::launch::async,
                    [this, request]() { return announce(request); });
}

ScrapeResponse UDPTracker::scrape(const ScrapeRequest &request) {
  try {
    // Get a connection ID
    uint64_t connection_id = connect();

    // Create the scrape request packet
    network::Buffer buffer(16 + 20 * request.info_hashes().size());
    buffer.resize(16 + 20 * request.info_hashes().size());

    // Connection ID (8 bytes)
    uint64_to_bytes(connection_id, buffer.data());

    // Action (4 bytes): scrape = 2
    uint32_to_bytes(static_cast<uint32_t>(Action::SCRAPE), buffer.data() + 8);

    // Transaction ID (4 bytes)
    uint32_t transaction_id = transaction_id_++;
    uint32_to_bytes(transaction_id, buffer.data() + 12);

    // Info hashes (20 bytes each)
    for (size_t i = 0; i < request.info_hashes().size(); ++i) {
      std::memcpy(buffer.data() + 16 + i * 20,
                  request.info_hashes()[i].bytes().data(), 20);
    }

    // Send the request
    int bytes_sent = send_packet(buffer);
    if (bytes_sent != static_cast<int>(buffer.size())) {
      ScrapeResponse response;
      response.set_error_message("Failed to send scrape request");
      return response;
    }

    // Receive the response
    network::Buffer response_buffer;
    int bytes_received = receive_packet(response_buffer, request_timeout_ms_);
    if (bytes_received < 8) {
      ScrapeResponse response;
      response.set_error_message("Invalid scrape response: too short");
      return response;
    }

    // Parse the response
    ScrapeResponse response;

    // Check the action
    uint32_t action = bytes_to_uint32(response_buffer.data());
    if (action == static_cast<uint32_t>(Action::ERROR)) {
      // Error response
      if (bytes_received >= 8) {
        std::string error_message(
            reinterpret_cast<const char *>(response_buffer.data() + 8),
            bytes_received - 8);
        response.set_error_message(error_message);
      } else {
        response.set_error_message("Unknown error");
      }
      return response;
    } else if (action != static_cast<uint32_t>(Action::SCRAPE)) {
      response.set_error_message("Invalid scrape response: wrong action");
      return response;
    }

    // Check the transaction ID
    uint32_t response_transaction_id =
        bytes_to_uint32(response_buffer.data() + 4);
    if (response_transaction_id != transaction_id) {
      response.set_error_message(
          "Invalid scrape response: wrong transaction ID");
      return response;
    }

    // Parse the response fields
    int num_hashes = (bytes_received - 8) / 12;
    for (int i = 0;
         i < num_hashes && i < static_cast<int>(request.info_hashes().size());
         ++i) {
      const uint8_t *hash_data = response_buffer.data() + 8 + i * 12;

      ScrapeResponse::ScrapeData data;
      data.complete = static_cast<int>(bytes_to_uint32(hash_data));
      data.downloaded = static_cast<int>(bytes_to_uint32(hash_data + 4));
      data.incomplete = static_cast<int>(bytes_to_uint32(hash_data + 8));

      response.add_file(request.info_hashes()[i], data);
    }

    return response;
  } catch (const std::exception &e) {
    ScrapeResponse response;
    response.set_error_message("Exception: " + std::string(e.what()));
    return response;
  }
}

std::future<ScrapeResponse>
UDPTracker::scrape_async(const ScrapeRequest &request) {
  return std::async(std::launch::async,
                    [this, request]() { return scrape(request); });
}

void UDPTracker::set_connection_timeout(int timeout_ms) {
  connection_timeout_ms_ = timeout_ms;
}

void UDPTracker::set_request_timeout(int timeout_ms) {
  request_timeout_ms_ = timeout_ms;
}

uint64_t UDPTracker::connect() {
  std::lock_guard<std::mutex> lock(connection_mutex_);

  // Check if we have a valid connection ID
  auto now = std::chrono::steady_clock::now();
  if (connection_id_ != 0 &&
      (now - connection_time_) < std::chrono::minutes(1)) {
    return connection_id_;
  }

  // Create the connect request packet
  network::Buffer buffer(16);
  buffer.resize(16);

  // Connection ID (8 bytes): 0x41727101980
  uint64_to_bytes(CONNECTION_ID_INIT, buffer.data());

  // Action (4 bytes): connect = 0
  uint32_to_bytes(static_cast<uint32_t>(Action::CONNECT), buffer.data() + 8);

  // Transaction ID (4 bytes)
  uint32_t transaction_id = transaction_id_++;
  uint32_to_bytes(transaction_id, buffer.data() + 12);

  // Send the request with exponential backoff
  for (int retry = 0; retry < MAX_RETRIES; ++retry) {
    // Send the request
    int bytes_sent = send_packet(buffer);
    if (bytes_sent != static_cast<int>(buffer.size())) {
      continue;
    }

    // Calculate timeout for this retry
    int timeout_ms = BASE_TIMEOUT_MS * (1 << retry);

    // Receive the response
    network::Buffer response_buffer;
    int bytes_received = receive_packet(response_buffer, timeout_ms);
    if (bytes_received < 16) {
      continue;
    }

    // Check the action
    uint32_t action = bytes_to_uint32(response_buffer.data());
    if (action != static_cast<uint32_t>(Action::CONNECT)) {
      continue;
    }

    // Check the transaction ID
    uint32_t response_transaction_id =
        bytes_to_uint32(response_buffer.data() + 4);
    if (response_transaction_id != transaction_id) {
      continue;
    }

    // Extract the connection ID
    connection_id_ = bytes_to_uint64(response_buffer.data() + 8);
    connection_time_ = now;

    return connection_id_;
  }

  throw std::runtime_error("Failed to connect to UDP tracker after " +
                           std::to_string(MAX_RETRIES) + " retries");
}

std::future<uint64_t> UDPTracker::connect_async() {
  return std::async(std::launch::async, [this]() { return connect(); });
}

int UDPTracker::send_packet(const network::Buffer &buffer) {
  return socket_->send_to(buffer, address_);
}

int UDPTracker::receive_packet(network::Buffer &buffer, int timeout_ms) {
  // Set the socket timeout
  socket_->set_receive_timeout(timeout_ms);

  // Receive the packet
  network::Address sender;
  int bytes_received = socket_->receive_from(buffer, sender);

  return bytes_received;
}

std::pair<std::string, uint16_t> UDPTracker::parse_url(const std::string &url) {
  // Check if the URL starts with "udp://"
  if (url.substr(0, 6) != "udp://") {
    throw std::invalid_argument("Invalid UDP tracker URL: " + url);
  }

  // Extract the host and port
  std::string host_port = url.substr(6);

  // Remove path and query string
  size_t path_start = host_port.find('/');
  if (path_start != std::string::npos) {
    host_port = host_port.substr(0, path_start);
  }

  // Split host and port
  size_t port_start = host_port.find(':');
  if (port_start == std::string::npos) {
    throw std::invalid_argument("UDP tracker URL must include port: " + url);
  }

  std::string host = host_port.substr(0, port_start);
  std::string port_str = host_port.substr(port_start + 1);

  // Parse the port
  uint16_t port;
  try {
    port = static_cast<uint16_t>(std::stoi(port_str));
  } catch (...) {
    port = 80;
  }

  return {host, port};
}

} // namespace bitscrape::tracker
