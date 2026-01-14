#include "bitscrape/dht/dht_message_factory.hpp"

#include <future>
#include <iomanip>
#include <random>
#include <sstream>

#include "bitscrape/bencode/bencode_decoder.hpp"

namespace bitscrape::dht {

std::shared_ptr<DHTMessage>
DHTMessageFactory::create_from_data(const std::vector<uint8_t> &data) {
  // Decode the bencode data
  bencode::BencodeValue value;

  try {
    auto decoder = bencode::create_bencode_decoder();
    value = decoder->decode(data);
  } catch (const std::exception &e) {
    // Failed to decode the data
    return nullptr;
  }

  // Create a message from the bencode value
  return create_from_bencode(value);
}

std::future<std::shared_ptr<DHTMessage>>
DHTMessageFactory::create_from_data_async(const std::vector<uint8_t> &data) {
  return std::async(std::launch::async,
                    [this, data]() { return this->create_from_data(data); });
}

std::shared_ptr<DHTMessage>
DHTMessageFactory::create_from_bencode(const bencode::BencodeValue &value) {
  // Check if the value is a dictionary
  if (!value.is_dict()) {
    return nullptr;
  }

  // Get the transaction ID
  const bencode::BencodeValue *t_value = value.get("t");
  if (!t_value || !t_value->is_string()) {
    return nullptr;
  }
  std::string transaction_id = t_value->as_string();

  // Get the message type
  const bencode::BencodeValue *y_value = value.get("y");
  if (!y_value || !y_value->is_string()) {
    return nullptr;
  }
  std::string y = y_value->as_string();

  if (y == "q") {
    // Query message
    const bencode::BencodeValue *q_value = value.get("q");
    if (!q_value || !q_value->is_string()) {
      return nullptr;
    }
    std::string q = q_value->as_string();

    if (q == "ping") {
      return parse_ping(value, transaction_id, false);
    } else if (q == "find_node") {
      return parse_find_node(value, transaction_id, false);
    } else if (q == "get_peers") {
      return parse_get_peers(value, transaction_id, false);
    } else if (q == "announce_peer") {
      return parse_announce_peer(value, transaction_id, false);
    } else {
      // Unknown query type
      return nullptr;
    }
  } else if (y == "r") {
    // Response message
    // Determine the response type based on the contents
    const bencode::BencodeValue *r_value = value.get("r");
    if (r_value && r_value->is_dict()) {

      const bencode::BencodeValue *id_value = r_value->get("id");
      if (id_value && id_value->is_string()) {
        // This is at least a ping response
        // Check for additional fields to determine the exact type
        const bencode::BencodeValue *nodes_value = r_value->get("nodes");
        if (nodes_value && nodes_value->is_string()) {
          // This is a find_node or get_peers response
          const bencode::BencodeValue *values_value = r_value->get("values");
          const bencode::BencodeValue *token_value = r_value->get("token");
          if (values_value || token_value) {
            // This is a get_peers response
            return parse_get_peers(value, transaction_id, true);
          } else {
            // This is a find_node response
            return parse_find_node(value, transaction_id, true);
          }
        } else {
          // This is a ping or announce_peer response
          // Try to determine if it's an announce_peer response
          // For now, we'll just assume it's a ping response
          // In a real implementation, we'd need to track transaction IDs
          return parse_ping(value, transaction_id, true);
        }
      }
    }

    // Failed to determine the response type
    return nullptr;
  } else if (y == "e") {
    // Error message
    return parse_error(value, transaction_id);
  } else {
    // Unknown message type
    return nullptr;
  }
}

std::future<std::shared_ptr<DHTMessage>>
DHTMessageFactory::create_from_bencode_async(
    const bencode::BencodeValue &value) {
  return std::async(std::launch::async, [this, value]() {
    return this->create_from_bencode(value);
  });
}

std::shared_ptr<DHTPingMessage>
DHTMessageFactory::create_ping(const std::string &transaction_id,
                               const types::NodeID &node_id) {
  return std::make_shared<DHTPingMessage>(transaction_id, node_id);
}

std::shared_ptr<DHTPingMessage>
DHTMessageFactory::create_ping_response(const std::string &transaction_id,
                                        const types::NodeID &node_id) {
  return std::make_shared<DHTPingMessage>(transaction_id, node_id, true);
}

std::shared_ptr<DHTGetPeersMessage>
DHTMessageFactory::create_get_peers(const std::string &transaction_id,
                                    const types::NodeID &node_id,
                                    const types::InfoHash &info_hash) {
  return std::make_shared<DHTGetPeersMessage>(transaction_id, node_id,
                                              info_hash);
}

std::shared_ptr<DHTGetPeersMessage>
DHTMessageFactory::create_get_peers_response(
    const std::string &transaction_id, const types::NodeID &node_id,
    const types::DHTToken &token, const std::vector<types::DHTNode> &nodes,
    const std::vector<types::Endpoint> &values) {
  return std::make_shared<DHTGetPeersMessage>(transaction_id, node_id, token,
                                              nodes, values);
}

std::shared_ptr<DHTFindNodeMessage>
DHTMessageFactory::create_find_node(const std::string &transaction_id,
                                    const types::NodeID &node_id,
                                    const types::NodeID &target_id) {
  return std::make_shared<DHTFindNodeMessage>(transaction_id, node_id,
                                              target_id);
}

std::shared_ptr<DHTFindNodeMessage>
DHTMessageFactory::create_find_node_response(
    const std::string &transaction_id, const types::NodeID &node_id,
    const std::vector<types::DHTNode> &nodes) {
  return std::make_shared<DHTFindNodeMessage>(transaction_id, node_id, nodes);
}

std::shared_ptr<DHTAnnouncePeerMessage> DHTMessageFactory::create_announce_peer(
    const std::string &transaction_id, const types::NodeID &node_id,
    const types::InfoHash &info_hash, uint16_t port,
    const types::DHTToken &token, bool implied_port) {
  return std::make_shared<DHTAnnouncePeerMessage>(
      transaction_id, node_id, info_hash, port, token, implied_port);
}

std::shared_ptr<DHTAnnouncePeerMessage>
DHTMessageFactory::create_announce_peer_response(
    const std::string &transaction_id, const types::NodeID &node_id) {
  return std::make_shared<DHTAnnouncePeerMessage>(transaction_id, node_id);
}

std::string DHTMessageFactory::generate_transaction_id() {
  // Generate a random 2-byte transaction ID
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 255);

  std::ostringstream oss;
  oss << static_cast<char>(dis(gen)) << static_cast<char>(dis(gen));

  return oss.str();
}

std::shared_ptr<DHTPingMessage>
DHTMessageFactory::parse_ping(const bencode::BencodeValue &value,
                              const std::string &transaction_id,
                              bool is_response) {
  // Get the arguments or response dictionary
  std::string dict_key = is_response ? "r" : "a";
  const bencode::BencodeValue *dict_value = value.get(dict_key);
  if (!dict_value || !dict_value->is_dict()) {
    return nullptr;
  }

  // Get the node ID
  const bencode::BencodeValue *id_value = dict_value->get("id");
  if (!id_value || !id_value->is_string()) {
    return nullptr;
  }
  auto id_str = id_value->as_string();

  // Create a NodeID from the string
  types::NodeID node_id;
  try {
    node_id = types::NodeID(std::vector<uint8_t>(id_str.begin(), id_str.end()));
  } catch (const std::exception &e) {
    // Failed to create the NodeID
    return nullptr;
  }

  // Create the ping message
  return std::make_shared<DHTPingMessage>(transaction_id, node_id, is_response);
}

std::shared_ptr<DHTGetPeersMessage>
DHTMessageFactory::parse_get_peers(const bencode::BencodeValue &value,
                                   const std::string &transaction_id,
                                   bool is_response) {
  // Get the arguments or response dictionary
  std::string dict_key = is_response ? "r" : "a";
  const bencode::BencodeValue *dict_value = value.get(dict_key);
  if (!dict_value || !dict_value->is_dict()) {
    return nullptr;
  }

  // Get the node ID
  const bencode::BencodeValue *id_value = dict_value->get("id");
  if (!id_value || !id_value->is_string()) {
    return nullptr;
  }
  auto id_str = id_value->as_string();

  // Create a NodeID from the string
  types::NodeID node_id;
  try {
    node_id = types::NodeID(std::vector<uint8_t>(id_str.begin(), id_str.end()));
  } catch (const std::exception &e) {
    // Failed to create the NodeID
    return nullptr;
  }

  if (!is_response) {
    // Query message - get the infohash
    const bencode::BencodeValue *info_hash_value = dict_value->get("info_hash");
    if (!info_hash_value || !info_hash_value->is_string()) {
      return nullptr;
    }
    auto info_hash_str = info_hash_value->as_string();

    // Create an InfoHash from the string
    types::InfoHash info_hash;
    try {
      info_hash = types::InfoHash(
          std::vector<uint8_t>(info_hash_str.begin(), info_hash_str.end()));
    } catch (const std::exception &e) {
      // Failed to create the InfoHash
      return nullptr;
    }

    // Create the get_peers query message
    return std::make_shared<DHTGetPeersMessage>(transaction_id, node_id,
                                                info_hash);
  } else {
    // Response message - get the token, nodes, and values
    const bencode::BencodeValue *token_value = dict_value->get("token");
    if (!token_value || !token_value->is_string()) {
      return nullptr;
    }
    auto token_str = token_value->as_string();

    // Create a DHTToken from the string
    types::DHTToken token(
        std::vector<uint8_t>(token_str.begin(), token_str.end()));

    // Get the nodes (optional)
    std::vector<types::DHTNode> nodes;
    const bencode::BencodeValue *nodes_value = dict_value->get("nodes");
    if (nodes_value && nodes_value->is_string()) {
      auto nodes_str = nodes_value->as_string();
      // Parse the nodes string (26 bytes per node: 20 bytes ID + 6 bytes
      // endpoint) This is a simplified implementation - in a real
      // implementation, we would need to parse the compact node info format For
      // now, just create a dummy node
      if (!nodes_str.empty()) {
        nodes.push_back(types::DHTNode(
            types::NodeID(), types::Endpoint(std::string("0.0.0.0"), 0)));
      }
    }

    // Get the values (optional)
    std::vector<types::Endpoint> values;
    const bencode::BencodeValue *values_value = dict_value->get("values");
    if (values_value && values_value->is_list()) {
      auto values_list = values_value->as_list();
      for (const auto &value : values_list) {
        if (value.is_string()) {
          auto value_str = value.as_string();
          // Parse the value string (6 bytes per peer: 4 bytes IP + 2 bytes
          // port) This is a simplified implementation - in a real
          // implementation, we would need to parse the compact peer info format
          // For now, just create a dummy endpoint
          values.push_back(types::Endpoint(std::string("0.0.0.0"), 0));
        }
      }
    }

    // Create the get_peers response message
    return std::make_shared<DHTGetPeersMessage>(transaction_id, node_id, token,
                                                nodes, values);
  }
}

std::shared_ptr<DHTFindNodeMessage>
DHTMessageFactory::parse_find_node(const bencode::BencodeValue &value,
                                   const std::string &transaction_id,
                                   bool is_response) {
  // Get the arguments or response dictionary
  std::string dict_key = is_response ? "r" : "a";
  const bencode::BencodeValue *dict_value = value.get(dict_key);
  if (!dict_value || !dict_value->is_dict()) {
    return nullptr;
  }

  // Get the node ID
  const bencode::BencodeValue *id_value = dict_value->get("id");
  if (!id_value || !id_value->is_string()) {
    return nullptr;
  }
  auto id_str = id_value->as_string();

  // Create a NodeID from the string
  types::NodeID node_id;
  try {
    node_id = types::NodeID(std::vector<uint8_t>(id_str.begin(), id_str.end()));
  } catch (const std::exception &e) {
    // Failed to create the NodeID
    return nullptr;
  }

  if (!is_response) {
    // Query message - get the target ID
    const bencode::BencodeValue *target_value = dict_value->get("target");
    if (!target_value || !target_value->is_string()) {
      return nullptr;
    }
    auto target_str = target_value->as_string();

    // Create a NodeID from the string
    types::NodeID target_id;
    try {
      target_id = types::NodeID(
          std::vector<uint8_t>(target_str.begin(), target_str.end()));
    } catch (const std::exception &e) {
      // Failed to create the NodeID
      return nullptr;
    }

    // Create the find_node query message
    return std::make_shared<DHTFindNodeMessage>(transaction_id, node_id,
                                                target_id);
  } else {
    // Response message - get the nodes
    std::vector<types::DHTNode> nodes;
    const bencode::BencodeValue *nodes_value = dict_value->get("nodes");
    if (nodes_value && nodes_value->is_string()) {
      auto nodes_str = nodes_value->as_string();
      // Parse the nodes string (26 bytes per node: 20 bytes ID + 6 bytes
      // endpoint) This is a simplified implementation - in a real
      // implementation, we would need to parse the compact node info format For
      // now, just create a dummy node
      if (!nodes_str.empty()) {
        nodes.push_back(types::DHTNode(
            types::NodeID(), types::Endpoint(std::string("0.0.0.0"), 0)));
      }
    }

    // Create the find_node response message
    return std::make_shared<DHTFindNodeMessage>(transaction_id, node_id, nodes);
  }
}

std::shared_ptr<DHTAnnouncePeerMessage>
DHTMessageFactory::parse_announce_peer(const bencode::BencodeValue &value,
                                       const std::string &transaction_id,
                                       bool is_response) {
  // Get the arguments or response dictionary
  std::string dict_key = is_response ? "r" : "a";
  const bencode::BencodeValue *dict_value = value.get(dict_key);
  if (!dict_value || !dict_value->is_dict()) {
    return nullptr;
  }

  // Get the node ID
  const bencode::BencodeValue *id_value = dict_value->get("id");
  if (!id_value || !id_value->is_string()) {
    return nullptr;
  }
  auto id_str = id_value->as_string();

  // Create a NodeID from the string
  types::NodeID node_id;
  try {
    node_id = types::NodeID(std::vector<uint8_t>(id_str.begin(), id_str.end()));
  } catch (const std::exception &e) {
    // Failed to create the NodeID
    return nullptr;
  }

  if (!is_response) {
    // Query message - get the infohash, port, token, and implied_port
    const bencode::BencodeValue *info_hash_value = dict_value->get("info_hash");
    if (!info_hash_value || !info_hash_value->is_string()) {
      return nullptr;
    }
    auto info_hash_str = info_hash_value->as_string();

    // Create an InfoHash from the string
    types::InfoHash info_hash;
    try {
      info_hash = types::InfoHash(
          std::vector<uint8_t>(info_hash_str.begin(), info_hash_str.end()));
    } catch (const std::exception &e) {
      // Failed to create the InfoHash
      return nullptr;
    }

    // Get the port
    const bencode::BencodeValue *port_value = dict_value->get("port");
    if (!port_value || !port_value->is_integer()) {
      return nullptr;
    }
    uint16_t port = static_cast<uint16_t>(port_value->as_integer());

    // Get the token
    const bencode::BencodeValue *token_value = dict_value->get("token");
    if (!token_value || !token_value->is_string()) {
      return nullptr;
    }
    auto token_str = token_value->as_string();

    // Create a DHTToken from the string
    types::DHTToken token(
        std::vector<uint8_t>(token_str.begin(), token_str.end()));

    // Get the implied_port (optional)
    bool implied_port = false;
    const bencode::BencodeValue *implied_port_value =
        dict_value->get("implied_port");
    if (implied_port_value && implied_port_value->is_integer()) {
      implied_port = implied_port_value->as_integer() != 0;
    }

    // Create the announce_peer query message
    return std::make_shared<DHTAnnouncePeerMessage>(
        transaction_id, node_id, info_hash, port, token, implied_port);
  } else {
    // Response message - just need the node ID
    return std::make_shared<DHTAnnouncePeerMessage>(transaction_id, node_id);
  }
}

std::shared_ptr<DHTMessage>
DHTMessageFactory::parse_error(const bencode::BencodeValue &value,
                               const std::string &transaction_id) {
  // Get the error array
  const bencode::BencodeValue *e_value = value.get("e");
  if (!e_value || !e_value->is_list()) {
    return nullptr;
  }
  auto e_list = e_value->as_list();
  if (e_list.size() < 2 || !e_list[0].is_integer() || !e_list[1].is_string()) {
    return nullptr;
  }

  // Get the error code and message
  int error_code = e_list[0].as_integer();
  (void)error_code;
  std::string error_message = e_list[1].as_string();

  // For now, just return a ping message as a placeholder
  // In a real implementation, we'd create a proper error message class
  return std::make_shared<DHTPingMessage>(transaction_id, types::NodeID());
}

} // namespace bitscrape::dht
