#include "bitscrape/dht/dht_session.hpp"
#include "bitscrape/bittorrent/bittorrent_event_processor.hpp"

#include <future>
#include <sstream>
#include <iostream>


namespace bitscrape::dht {

DHTSession::DHTSession(std::shared_ptr<lock::LockManager> lock_manager)
    : node_id_(types::NodeID()), port_(6881), event_bus_(nullptr),
      running_(false), lock_manager_(lock_manager),
      peers_resource_id_(lock_manager->register_resource(
          get_peers_resource_name(), lock::LockManager::LockPriority::NORMAL)),
      lookups_resource_id_(lock_manager->register_resource(
          get_lookups_resource_name(),
          lock::LockManager::LockPriority::NORMAL)) {
  // Create the socket
  socket_ = std::make_unique<network::UDPSocket>();

  // Create the routing table
  routing_table_ = std::make_unique<RoutingTable>(node_id_, lock_manager);

  // Create the message factory
  message_factory_ = std::make_unique<DHTMessageFactory>();

  // Create the token manager
  token_manager_ = std::make_unique<TokenManager>();
}

std::string DHTSession::get_peers_resource_name() const {
  std::stringstream ss;
  ss << "dht.session.peers." << node_id_.to_hex().substr(0, 8);
  return ss.str();
}

std::string DHTSession::get_lookups_resource_name() const {
  std::stringstream ss;
  ss << "dht.session.lookups." << node_id_.to_hex().substr(0, 8);
  return ss.str();
}

DHTSession::DHTSession(const types::NodeID &node_id,
                       std::shared_ptr<lock::LockManager> lock_manager)
    : node_id_(node_id), port_(6881), event_bus_(nullptr), running_(false),
      lock_manager_(lock_manager),
      peers_resource_id_(lock_manager->register_resource(
          get_peers_resource_name(), lock::LockManager::LockPriority::NORMAL)),
      lookups_resource_id_(lock_manager->register_resource(
          get_lookups_resource_name(),
          lock::LockManager::LockPriority::NORMAL)) {
  // Create the socket
  socket_ = std::make_unique<network::UDPSocket>();

  // Create the routing table
  routing_table_ = std::make_unique<RoutingTable>(node_id_, lock_manager);

  // Create the message factory
  message_factory_ = std::make_unique<DHTMessageFactory>();

  // Create the token manager
  token_manager_ = std::make_unique<TokenManager>();
}

DHTSession::DHTSession(const types::NodeID &node_id, uint16_t port,
                       event::EventBus &event_bus,
                       std::shared_ptr<lock::LockManager> lock_manager)
    : node_id_(node_id), port_(port), event_bus_(&event_bus), running_(false),
      lock_manager_(lock_manager),
      peers_resource_id_(lock_manager->register_resource(
          get_peers_resource_name(), lock::LockManager::LockPriority::NORMAL)),
      lookups_resource_id_(lock_manager->register_resource(
          get_lookups_resource_name(),
          lock::LockManager::LockPriority::NORMAL)) {
  // Create the socket
  socket_ = std::make_unique<network::UDPSocket>();

  // Create the routing table
  routing_table_ = std::make_unique<RoutingTable>(node_id_, lock_manager);

  // Create the message factory
  message_factory_ = std::make_unique<DHTMessageFactory>();

  // Create the token manager
  token_manager_ = std::make_unique<TokenManager>();
}

DHTSession::~DHTSession() {
  // Stop the session if it's running
  if (running_) {
    stop();
  }
}

bool DHTSession::start(const std::vector<types::Endpoint> &bootstrap_nodes) {
  // Check if the session is already running
  if (running_) {
    return true; // Already running, just return success
  }

  // Bind the socket to the port
  if (!socket_->bind(port_)) {
    // Failed to bind, could be because the port is already in use
    // Try to bind to a different port
    for (uint16_t p = port_ + 1; p < port_ + 10; ++p) {
      if (socket_->bind(p)) {
        port_ = p; // Update the port
        break;
      }
    }

    // If we still couldn't bind, return failure
    if (!socket_->is_valid()) {
      return false;
    }
  }

  // Start the receive loop
  running_ = true;
  start_receive_loop();

  // Bootstrap the DHT
  bootstrap_ = std::make_unique<Bootstrap>(node_id_, *routing_table_, *socket_,
                                           *message_factory_, *this);
  return bootstrap_->start(bootstrap_nodes);
}

std::future<bool>
DHTSession::start_async(const std::vector<types::Endpoint> &bootstrap_nodes) {
  return std::async(std::launch::async, [this, bootstrap_nodes]() {
    return this->start(bootstrap_nodes);
  });
}

void DHTSession::stop() {
  // Check if the session is running
  if (!running_) {
    return;
  }

  // Stop the receive loop
  running_ = false;

  // Close the socket
  socket_->close();

  // Wait for the receive thread to finish
  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }
}

std::vector<types::DHTNode>
DHTSession::find_nodes(const types::NodeID &target_id) {
  // Create a node lookup
  auto lookup = std::make_shared<NodeLookup>(node_id_, target_id, *routing_table_, *socket_,
                    *message_factory_, *this, NodeLookup::QueryType::FIND_NODE, lock_manager_);

  // Start the lookup
  return lookup->start();
}

std::future<std::vector<types::DHTNode>>
DHTSession::find_nodes_async(const types::NodeID &target_id) {
  return std::async(std::launch::async, [this, target_id]() {
    return this->find_nodes(target_id);
  });
}

std::vector<types::Endpoint>
DHTSession::find_peers(const types::InfoHash &infohash) {
  // Check if we already have peers for this infohash
  {
    auto guard = lock_manager_->get_lock_guard(
        peers_resource_id_, lock::LockManager::LockType::SHARED);
    auto it = peers_.find(infohash);
    if (it != peers_.end() && !it->second.empty()) {
      return it->second;
    }
  }

  // Convert the infohash to a node ID for the lookup
  types::NodeID target_id(
      std::vector<uint8_t>(infohash.bytes().begin(), infohash.bytes().end()));

  // Create a peer lookup
  auto lookup = std::make_shared<NodeLookup>(node_id_, target_id, *routing_table_, *socket_,
                    *message_factory_, *this, NodeLookup::QueryType::GET_PEERS, lock_manager_);

  // Set the peer callback
  lookup->set_peer_callback([this](const types::InfoHash& hash, const types::Endpoint& peer) {
      // Emit PeerDiscoveredEvent
      if (event_bus_) {
          network::Address address(peer.address(), peer.port());
          event_bus_->publish(bittorrent::PeerDiscoveredEvent(hash, address));
      }

      // Also add to our internal list
      {
          auto guard = lock_manager_->get_lock_guard(peers_resource_id_, lock::LockManager::LockType::EXCLUSIVE);
          peers_[hash].push_back(peer);
      }
  });

  // Start the lookup
  lookup->start();

  // Return the peers found during the lookup (already added to peers_ via callback)
  auto guard = lock_manager_->get_lock_guard(
      peers_resource_id_, lock::LockManager::LockType::SHARED);
  auto it = peers_.find(infohash);
  if (it != peers_.end()) {
      return it->second;
  }
  return {};
}

std::future<std::vector<types::Endpoint>>
DHTSession::find_peers_async(const types::InfoHash &infohash) {
  return std::async(std::launch::async,
                    [this, infohash]() { return this->find_peers(infohash); });
}

bool DHTSession::announce_peer(const types::InfoHash &infohash, uint16_t port) {
  // Convert the infohash to a node ID for the lookup
  types::NodeID target_id(
      std::vector<uint8_t>(infohash.bytes().begin(), infohash.bytes().end()));

  // Find nodes close to the infohash
  auto nodes = find_nodes(target_id);

  // Send announce_peer messages to the found nodes
  bool success = false;
  for (const auto &node : nodes) {
    // Check if we have a token for this node
    // In a real implementation, we would track tokens received from get_peers
    // responses For now, just use a random token
    types::DHTToken token = types::DHTToken::random();

    // Create an announce_peer message
    auto transaction_id = DHTMessageFactory::generate_transaction_id();
    auto message = message_factory_->create_announce_peer(
        transaction_id, node_id_, infohash, port, token);

    // Encode the message
    auto data = message->encode();

    // Send the message
    network::Address address(node.endpoint().address(), node.endpoint().port());
    if (socket_->send_to(data.data(), data.size(), address)) {
      success = true;
    }
  }

  return success;
}

std::future<bool>
DHTSession::announce_peer_async(const types::InfoHash &infohash,
                                uint16_t port) {
  return std::async(std::launch::async, [this, infohash, port]() {
    return this->announce_peer(infohash, port);
  });
}

const types::NodeID &DHTSession::node_id() const { return node_id_; }

const RoutingTable &DHTSession::routing_table() const {
  return *routing_table_;
}

bool DHTSession::is_running() const { return running_; }

void DHTSession::process_message(const std::vector<uint8_t> &data,
                                 const types::Endpoint &sender_endpoint) {
  // Parse the message
  auto message = message_factory_->create_from_data(data);
  if (!message) {
    return;
  }

  // If we're bootstrapping, pass the message to the bootstrap object
  if (bootstrap_ && !bootstrap_->is_complete()) {
    bootstrap_->process_message(message, sender_endpoint);
  }

  // Add the sender to the routing table if it's a valid node
  if (message->type() == DHTMessage::Type::PING ||
      message->type() == DHTMessage::Type::FIND_NODE ||
      message->type() == DHTMessage::Type::GET_PEERS ||
      message->type() == DHTMessage::Type::ANNOUNCE_PEER) {
    // For now, we'll just use the DHTPingMessage as an example
    // In a real implementation, we'd need to handle all message types
    auto ping_message = std::dynamic_pointer_cast<DHTPingMessage>(message);
    if (ping_message) {
      // Get the node ID from the message
      types::NodeID node_id = ping_message->node_id();

      // Create a node and add it to the routing table
      types::DHTNode node(node_id, sender_endpoint);
      routing_table_->add_node(node);
    }
  }



  if (message->type() == DHTMessage::Type::PING_RESPONSE ||
      message->type() == DHTMessage::Type::FIND_NODE_RESPONSE ||
      message->type() == DHTMessage::Type::GET_PEERS_RESPONSE ||
      message->type() == DHTMessage::Type::ANNOUNCE_PEER_RESPONSE) {
      
    std::shared_ptr<NodeLookup> lookup = nullptr;
    
    {
        auto guard = lock_manager_->get_lock_guard(
            lookups_resource_id_, lock::LockManager::LockType::SHARED, 5000);

        // Find the lookup with the matching transaction ID
        auto it = lookups_.find(message->transaction_id());
        if (it != lookups_.end()) {
          lookup = it->second;
        }
    }

    if (lookup) {
        // Process the response
        lookup->process_response(message, sender_endpoint);
        return;
    }
  }

  // Handle the message based on its type
  switch (message->type()) {
  case DHTMessage::Type::PING:
    handle_ping(message, sender_endpoint);
    break;
  case DHTMessage::Type::FIND_NODE:
    handle_find_node(message, sender_endpoint);
    break;
  case DHTMessage::Type::GET_PEERS:
    handle_get_peers(message, sender_endpoint);
    break;
  case DHTMessage::Type::ANNOUNCE_PEER:
    handle_announce_peer(message, sender_endpoint);
    break;
  default:
    // Ignore other message types
    break;
  }
}

void DHTSession::start_receive_loop() {
  // Start the receive thread
  receive_thread_ = std::thread([this]() {
    while (running_) {
      try {
        // Receive a message
        network::Buffer buffer;
        network::Address address;
        int bytes_received = socket_->receive_from(buffer, address);

        if (bytes_received <= 0) {
          continue; // No data received or error
        }



        // Convert to vector<uint8_t> and Endpoint
        std::vector<uint8_t> data(buffer.data(), buffer.data() + buffer.size());
        types::Endpoint sender_endpoint(address.to_string(), address.port());

        // Process the message
        process_message(data, sender_endpoint);
      } catch (const std::exception &e) {
        // Ignore exceptions
        std::cerr << "DHTSession: Exception in receive loop: " << e.what() << std::endl;
      }
    }
  });
}

void DHTSession::handle_ping(const std::shared_ptr<DHTMessage> &message,
                             const types::Endpoint &sender_endpoint) {
  // Create a ping response
  auto response = message_factory_->create_ping_response(
      message->transaction_id(), node_id_);

  // Encode the response
  auto data = response->encode();

  // Send the response
  network::Address address(sender_endpoint.address(), sender_endpoint.port());
  socket_->send_to(data.data(), data.size(), address);
}

void DHTSession::handle_find_node(const std::shared_ptr<DHTMessage> &message,
                                  const types::Endpoint &sender_endpoint) {
  // Cast to DHTFindNodeMessage
  auto find_node_message =
      std::dynamic_pointer_cast<DHTFindNodeMessage>(message);
  if (!find_node_message) {
    return;
  }

  // Extract the node ID from the message and add it to the routing table
  types::DHTNode node(find_node_message->node_id(), sender_endpoint);
  routing_table_->add_node(node);

  // Get the target ID
  types::NodeID target_id = find_node_message->target_id();

  // Find nodes close to the target ID
  auto nodes = routing_table_->get_closest_nodes(target_id, 8);

  // Create a find_node response
  auto response = message_factory_->create_find_node_response(
      find_node_message->transaction_id(), node_id_, nodes);

  // Encode the response
  auto data = response->encode();

  // Send the response
  network::Address address(sender_endpoint.address(), sender_endpoint.port());
  socket_->send_to(data.data(), data.size(), address);
}

void DHTSession::handle_get_peers(const std::shared_ptr<DHTMessage> &message,
                                  const types::Endpoint &sender_endpoint) {
  // Cast to DHTGetPeersMessage
  auto get_peers_message =
      std::dynamic_pointer_cast<DHTGetPeersMessage>(message);
  if (!get_peers_message) {
    return;
  }

  // Get the infohash
  const auto &info_hash = get_peers_message->info_hash();

  // Notify callback about discovered infohash (passive collection)
  if (on_infohash_discovered_) {
    on_infohash_discovered_(info_hash);
  }

  // Check if we have peers for this infohash
  std::vector<types::Endpoint> peers;
  {
    auto guard = lock_manager_->get_lock_guard(
        peers_resource_id_, lock::LockManager::LockType::SHARED);
    auto it = peers_.find(info_hash);
    if (it != peers_.end()) {
      peers = it->second;
    }
  }

  // Create a token for this sender using the token manager
  // The token is based on the sender's IP and a secret per BEP-0005
  types::DHTToken token = token_manager_->generate_token(sender_endpoint);

  // Find nodes close to the infohash
  types::NodeID target_id(
      std::vector<uint8_t>(info_hash.bytes().begin(), info_hash.bytes().end()));
  auto nodes = routing_table_->get_closest_nodes(target_id, 8);

  // Create a get_peers response
  auto response = message_factory_->create_get_peers_response(
      get_peers_message->transaction_id(), node_id_, token, nodes, peers);

  // Encode the response
  auto data = response->encode();

  // Send the response
  network::Address address(sender_endpoint.address(), sender_endpoint.port());
  socket_->send_to(data.data(), data.size(), address);
}

void DHTSession::handle_announce_peer(
    const std::shared_ptr<DHTMessage> &message,
    const types::Endpoint &sender_endpoint) {
  // Cast to DHTAnnouncePeerMessage
  auto announce_peer_message =
      std::dynamic_pointer_cast<DHTAnnouncePeerMessage>(message);
  if (!announce_peer_message) {
    return;
  }

  // Extract the node ID from the message and add it to the routing table
  types::DHTNode node(announce_peer_message->node_id(), sender_endpoint);
  routing_table_->add_node(node);

  // Verify the token using the token manager
  const auto &token = announce_peer_message->token();
  if (!token_manager_->verify_token(token, sender_endpoint)) {
    // Invalid token - reject the announce
    // TODO: Optionally send an error response
    return;
  }

  // Get the infohash and port
  types::InfoHash info_hash = announce_peer_message->info_hash();
  uint16_t port = announce_peer_message->port();

  // If implied_port is set, use the sender's port instead
  if (announce_peer_message->implied_port()) {
    port = sender_endpoint.port();
  }

  // Notify callback about discovered infohash (passive collection)
  if (on_infohash_discovered_) {
    on_infohash_discovered_(info_hash);
  }

  // Add the peer to our list
  types::Endpoint peer_endpoint(sender_endpoint.address(), port);
  {
    auto guard = lock_manager_->get_lock_guard(
        peers_resource_id_, lock::LockManager::LockType::EXCLUSIVE);
    peers_[info_hash].push_back(peer_endpoint);
  }

  // Create an announce_peer response
  auto response = message_factory_->create_announce_peer_response(
      announce_peer_message->transaction_id(), node_id_);

  // Encode the response
  auto data = response->encode();

  // Send the response
  network::Address address(sender_endpoint.address(), sender_endpoint.port());
  socket_->send_to(data.data(), data.size(), address);
}

void DHTSession::register_transaction(const std::string& transaction_id, std::shared_ptr<NodeLookup> lookup) {
    auto guard = lock_manager_->get_lock_guard(lookups_resource_id_, lock::LockManager::LockType::EXCLUSIVE, 5000);
    lookups_[transaction_id] = lookup;
}

void DHTSession::unregister_transaction(const std::string& transaction_id) {
    auto guard = lock_manager_->get_lock_guard(lookups_resource_id_, lock::LockManager::LockType::EXCLUSIVE, 5000);
    lookups_.erase(transaction_id);
}

void DHTSession::set_infohash_callback(std::function<void(const types::InfoHash&)> callback) {
    on_infohash_discovered_ = std::move(callback);
}

} // namespace bitscrape::dht
