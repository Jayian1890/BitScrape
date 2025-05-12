#include "bitscrape/dht/dht_message_factory.hpp"

#include <random>
#include <sstream>
#include <iomanip>
#include <future>

#include "bitscrape/bencode/bencode_decoder.hpp"

namespace bitscrape::dht {

std::shared_ptr<DHTMessage> DHTMessageFactory::create_from_data(const std::vector<uint8_t>& data) {
    // Decode the bencode data
    bencode::BencodeValue value;

    try {
        auto decoder = bencode::create_bencode_decoder();
        value = decoder->decode(data);
    } catch (const std::exception& e) {
        // Failed to decode the data
        return nullptr;
    }

    // Create a message from the bencode value
    return create_from_bencode(value);
}

std::future<std::shared_ptr<DHTMessage>> DHTMessageFactory::create_from_data_async(const std::vector<uint8_t>& data) {
    return std::async(std::launch::async, [this, data]() {
        return this->create_from_data(data);
    });
}

std::shared_ptr<DHTMessage> DHTMessageFactory::create_from_bencode(const bencode::BencodeValue& value) {
    // Check if the value is a dictionary
    if (!value.is_dict()) {
        return nullptr;
    }

    // Get the transaction ID
    const bencode::BencodeValue* t_value = value.get("t");
    if (!t_value || !t_value->is_string()) {
        return nullptr;
    }
    std::string transaction_id = t_value->as_string();

    // Get the message type
    const bencode::BencodeValue* y_value = value.get("y");
    if (!y_value || !y_value->is_string()) {
        return nullptr;
    }
    std::string y = y_value->as_string();

    if (y == "q") {
        // Query message
        const bencode::BencodeValue* q_value = value.get("q");
        if (!q_value || !q_value->is_string()) {
            return nullptr;
        }
        std::string q = q_value->as_string();

        if (q == "ping") {
            return parse_ping(value, transaction_id, false);
        } else if (q == "find_node") {
            // TODO: Implement find_node parsing
            return nullptr;
        } else if (q == "get_peers") {
            // TODO: Implement get_peers parsing
            return nullptr;
        } else if (q == "announce_peer") {
            // TODO: Implement announce_peer parsing
            return nullptr;
        } else {
            // Unknown query type
            return nullptr;
        }
    } else if (y == "r") {
        // Response message
        // Determine the response type based on the contents
        const bencode::BencodeValue* r_value = value.get("r");
        if (r_value && r_value->is_dict()) {

            const bencode::BencodeValue* id_value = r_value->get("id");
            if (id_value && id_value->is_string()) {
                // This is at least a ping response
                // Check for additional fields to determine the exact type
                const bencode::BencodeValue* nodes_value = r_value->get("nodes");
                if (nodes_value && nodes_value->is_string()) {
                    // This is a find_node or get_peers response
                    const bencode::BencodeValue* values_value = r_value->get("values");
                    const bencode::BencodeValue* token_value = r_value->get("token");
                    if (values_value || token_value) {
                        // This is a get_peers response
                        // TODO: Implement get_peers response parsing
                        return nullptr;
                    } else {
                        // This is a find_node response
                        // TODO: Implement find_node response parsing
                        return nullptr;
                    }
                } else {
                    // This is a ping or announce_peer response
                    // Since we can't distinguish between them, assume it's a ping response
                    return parse_ping(value, transaction_id, true);
                }
            }
        }

        // Failed to determine the response type
        return nullptr;
    } else if (y == "e") {
        // Error message
        // TODO: Implement error message parsing
        return nullptr;
    } else {
        // Unknown message type
        return nullptr;
    }
}

std::future<std::shared_ptr<DHTMessage>> DHTMessageFactory::create_from_bencode_async(const bencode::BencodeValue& value) {
    return std::async(std::launch::async, [this, value]() {
        return this->create_from_bencode(value);
    });
}

std::shared_ptr<DHTPingMessage> DHTMessageFactory::create_ping(const std::string& transaction_id, const types::NodeID& node_id) {
    return std::make_shared<DHTPingMessage>(transaction_id, node_id);
}

std::shared_ptr<DHTPingMessage> DHTMessageFactory::create_ping_response(const std::string& transaction_id, const types::NodeID& node_id) {
    return std::make_shared<DHTPingMessage>(transaction_id, node_id, true);
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

std::shared_ptr<DHTPingMessage> DHTMessageFactory::parse_ping(const bencode::BencodeValue& value,
                                                             const std::string& transaction_id,
                                                             bool is_response) {
    // Get the arguments or response dictionary
    std::string dict_key = is_response ? "r" : "a";
    const bencode::BencodeValue* dict_value = value.get(dict_key);
    if (!dict_value || !dict_value->is_dict()) {
        return nullptr;
    }

    // Get the node ID
    const bencode::BencodeValue* id_value = dict_value->get("id");
    if (!id_value || !id_value->is_string()) {
        return nullptr;
    }
    auto id_str = id_value->as_string();

    // Create a NodeID from the string
    types::NodeID node_id;
    try {
        node_id = types::NodeID(std::vector<uint8_t>(id_str.begin(), id_str.end()));
    } catch (const std::exception& e) {
        // Failed to create the NodeID
        return nullptr;
    }

    // Create the ping message
    return std::make_shared<DHTPingMessage>(transaction_id, node_id, is_response);
}

} // namespace bitscrape::dht
