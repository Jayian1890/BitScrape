#include <bitscrape/core/configuration.hpp>
// Keep bencode includes for compatibility with other modules
#include <bitscrape/bencode/bencode_value.hpp>
#include <bitscrape/bencode/bencode_encoder.hpp>
#include <bitscrape/bencode/bencode_decoder.hpp>
#include <bitscrape/lock/lock_manager_singleton.hpp>
#include <bitscrape/lock/lock_guard.hpp>
#include <bitscrape/lock/lock_exceptions.hpp>

#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <cstdlib>

namespace bitscrape::core {

class Configuration::Impl {
public:
    Impl(const std::string& config_path)
        : config_path_(config_path.empty() ? Configuration::get_default_config_path() : config_path)
    {
        // Register the configuration resource with the LockManager
        auto lock_manager = lock::LockManagerSingleton::instance();
        config_resource_id_ = lock_manager->register_resource("configuration", lock::LockManager::LockPriority::HIGH);
    }

    bool load()
    {
        // Acquire a lock on the configuration resource
        auto lock_manager = lock::LockManagerSingleton::instance();


        std::unique_ptr<lock::LockGuard> lock_guard;
        try {
            lock_guard = lock_manager->get_lock_guard(config_resource_id_);
        } catch (const std::exception& e) {
            std::cerr << "Failed to acquire configuration lock: " << e.what() << std::endl;
            std::cerr << lock_manager->dump_lock_state() << std::endl;
            throw;
        }

        try {
            // Check if file exists
            if (!std::filesystem::exists(config_path_))
            {
                // Release the lock before calling create_default_configuration
                lock_guard.reset();
                // Create default configuration
                create_default_configuration();
                return true;
            }

            // Open file
            std::ifstream file(config_path_);
            if (!file.is_open()) {
                std::cerr << "Failed to open configuration file: " << config_path_ << std::endl;
                return false;
            }

            // Read file content
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();

            // Clear current configuration
            config_.clear();

            // Parse JSON
            if (content.empty())
            {
                std::cerr << "Empty configuration file" << std::endl;
                return false;
            }

            // Find the opening brace
            size_t pos = content.find('{');
            if (pos == std::string::npos)
            {
                std::cerr << "Invalid JSON format: missing opening brace" << std::endl;
                return false;
            }

            // Parse JSON object
            pos++; // Move past the opening brace
            while (pos < content.size())
            {
                // Skip whitespace
                pos = content.find_first_not_of(" \t\n\r", pos);
                if (pos == std::string::npos || content[pos] == '}')
                {
                    break; // End of object
                }

                // Find key (must be in quotes)
                if (content[pos] != '"')
                {
                    std::cerr << "Invalid JSON format: expected key" << std::endl;
                    return false;
                }

                size_t key_start = pos + 1;
                size_t key_end = content.find('"', key_start);
                if (key_end == std::string::npos)
                {
                    std::cerr << "Invalid JSON format: unterminated key string" << std::endl;
                    return false;
                }

                std::string key = content.substr(key_start, key_end - key_start);
                pos = key_end + 1;

                // Find colon
                pos = content.find(':', pos);
                if (pos == std::string::npos)
                {
                    std::cerr << "Invalid JSON format: missing colon after key" << std::endl;
                    return false;
                }
                pos++;

                // Skip whitespace
                pos = content.find_first_not_of(" \t\n\r", pos);
                if (pos == std::string::npos)
                {
                    std::cerr << "Invalid JSON format: unexpected end of file" << std::endl;
                    return false;
                }

                // Parse value based on its type
                if (content[pos] == '"')
                {
                    // String value
                    size_t value_start = pos + 1;
                    size_t value_end = pos + 1;

                    // Find the closing quote, handling escaped quotes
                    bool escaped = false;
                    while (value_end < content.size())
                    {
                        if (content[value_end] == '\\')
                        {
                            escaped = !escaped;
                        }
                        else if (content[value_end] == '"' && !escaped)
                        {
                            break;
                        }
                        else
                        {
                            escaped = false;
                        }
                        value_end++;
                    }

                    if (value_end >= content.size())
                    {
                        std::cerr << "Invalid JSON format: unterminated string value" << std::endl;
                        return false;
                    }

                    std::string value = content.substr(value_start, value_end - value_start);
                    config_[key] = value;
                    pos = value_end + 1;
                }
                else if (content[pos] == '[')
                {
                    // Array value
                    size_t array_start = pos + 1;
                    size_t array_end = pos + 1;
                    int bracket_depth = 1;

                    // Find the closing bracket, handling nested arrays
                    while (array_end < content.size() && bracket_depth > 0)
                    {
                        if (content[array_end] == '[')
                        {
                            bracket_depth++;
                        }
                        else if (content[array_end] == ']')
                        {
                            bracket_depth--;
                        }
                        array_end++;
                    }

                    if (bracket_depth != 0)
                    {
                        std::cerr << "Invalid JSON format: unterminated array" << std::endl;
                        return false;
                    }

                    array_end--; // Move back to the closing bracket
                    std::string array_content = content.substr(array_start, array_end - array_start);

                    // Parse array elements
                    std::vector<std::string> elements;
                    size_t elem_pos = 0;

                    while (elem_pos < array_content.size())
                    {
                        // Skip whitespace
                        elem_pos = array_content.find_first_not_of(" \t\n\r,", elem_pos);
                        if (elem_pos == std::string::npos)
                        {
                            break;
                        }

                        if (array_content[elem_pos] == '"')
                        {
                            // String element
                            size_t elem_start = elem_pos + 1;
                            size_t elem_end = elem_start;

                            // Find the closing quote, handling escaped quotes
                            bool escaped = false;
                            while (elem_end < array_content.size())
                            {
                                if (array_content[elem_end] == '\\')
                                {
                                    escaped = !escaped;
                                }
                                else if (array_content[elem_end] == '"' && !escaped)
                                {
                                    break;
                                }
                                else
                                {
                                    escaped = false;
                                }
                                elem_end++;
                            }

                            if (elem_end >= array_content.size())
                            {
                                std::cerr << "Invalid JSON format: unterminated string in array" << std::endl;
                                return false;
                            }

                            std::string element = array_content.substr(elem_start, elem_end - elem_start);
                            elements.push_back(element);
                            elem_pos = elem_end + 1;
                        }
                        else if (isdigit(array_content[elem_pos]) || array_content[elem_pos] == '-')
                        {
                            // Number element
                            size_t elem_start = elem_pos;
                            size_t elem_end = array_content.find_first_of(",]\t\n\r ", elem_pos);
                            if (elem_end == std::string::npos)
                            {
                                elem_end = array_content.size();
                            }

                            std::string element = array_content.substr(elem_start, elem_end - elem_start);
                            elements.push_back(element);
                            elem_pos = elem_end;
                        }
                        else
                        {
                            // Skip invalid elements
                            elem_pos = array_content.find(',', elem_pos);
                            if (elem_pos == std::string::npos)
                            {
                                break;
                            }
                            elem_pos++;
                        }
                    }

                    // Store array as comma-separated string
                    std::string list_str;
                    for (size_t i = 0; i < elements.size(); ++i)
                    {
                        if (i > 0)
                        {
                            list_str += ",";
                        }
                        list_str += elements[i];
                    }

                    config_[key] = list_str;
                    pos = array_end + 1;
                }
                else if (isdigit(content[pos]) || content[pos] == '-')
                {
                    // Number value
                    size_t value_start = pos;
                    size_t value_end = content.find_first_of(",}\t\n\r ", pos);
                    if (value_end == std::string::npos)
                    {
                        std::cerr << "Invalid JSON format: unterminated number" << std::endl;
                        return false;
                    }

                    std::string value = content.substr(value_start, value_end - value_start);
                    config_[key] = value;
                    pos = value_end;
                }
                else if (content.substr(pos, 4) == "true")
                {
                    // Boolean true
                    config_[key] = "1";
                    pos += 4;
                }
                else if (content.substr(pos, 5) == "false")
                {
                    // Boolean false
                    config_[key] = "0";
                    pos += 5;
                }
                else if (content.substr(pos, 4) == "null")
                {
                    // Null value
                    config_[key] = "";
                    pos += 4;
                } else {
                    std::cerr << "Invalid JSON format: unknown value type" << std::endl;
                    return false;
                }

                // Find comma or closing brace
                pos = content.find_first_not_of(" \t\n\r", pos);
                if (pos == std::string::npos) {
                    std::cerr << "Invalid JSON format: unexpected end of file" << std::endl;
                    return false;
                }

                if (content[pos] == ',')
                {
                    pos++; // Move past the comma
                }
                else if (content[pos] == '}')
                {
                    break; // End of object
                } else {
                    std::cerr << "Invalid JSON format: expected comma or closing brace" << std::endl;
                    return false;
                }
            }

            apply_missing_defaults();
            // Release the lock before calling save to avoid acquiring the same lock twice in this thread
            lock_guard.reset();
            // Always rewrite on load so the file is pretty-printed and complete
            if (!save()) {
                std::cerr << "Warning: Failed to save configuration after loading defaults (may be read-only)" << std::endl;
            }
            return true;
        } catch (const lock::LockOperationException& e) {
            std::cerr << "Failed to load configuration (lock): " << e.what() << std::endl;
            std::cerr << lock_manager->dump_lock_state() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "Failed to load configuration: " << e.what() << std::endl;
            std::cerr << lock_manager->dump_lock_state() << std::endl;

            auto stack = lock_manager->get_lock_stack();
            if (!stack.empty()) {
                std::cerr << "Thread lock stack at failure: ";
                for (auto id : stack) {
                    try {
                        std::cerr << lock_manager->get_resource_name(id) << "(" << id << ") ";
                    } catch (...) {
                        std::cerr << id << " ";
                    }
                }
                std::cerr << std::endl;
            }

            return false;
        }
    }

    std::future<bool> load_async()
    {
        return std::async(std::launch::async, [this]()
        {
            return load();
        });
    }

    bool save() {
        // Acquire a lock on the configuration resource
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_);

        try
        {
            auto format_scalar = [](const std::string& value) {
                if (value.empty()) {
                    return std::string("null");
                }

                // Booleans
                if (value == "true" || value == "yes") {
                    return std::string("true");
                }
                if (value == "false" || value == "no") {
                    return std::string("false");
                }

                // Strictly numeric check (integer)
                bool is_numeric = !value.empty();
                size_t start = (value[0] == '-') ? 1 : 0;
                if (start == value.size()) is_numeric = false;
                for (size_t i = start; i < value.size(); ++i) {
                    if (!std::isdigit(static_cast<unsigned char>(value[i]))) {
                        is_numeric = false;
                        break;
                    }
                }

                if (is_numeric) {
                    // One final check: if it was a boolean "1" or "0" in the map, 
                    // it might have come from a bool setter. But we actually store 
                    // booleans as "1" or "0" string representations.
                    // To stay consistent with JSON bools:
                    if (value == "1") return std::string("true");
                    if (value == "0") return std::string("false");
                    return value;
                }

                // Everything else is a string and needs quotes
                return std::string("\"") + value + "\"";
            };

            auto format_array = [&](const std::string& value, const std::string& indent) {
                std::vector<std::string> items;
                std::stringstream ss(value);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    items.push_back(item);
                }

                std::string out = "[\n";
                for (size_t i = 0; i < items.size(); ++i) {
                    out += indent + "  " + format_scalar(items[i]);
                    if (i + 1 < items.size()) {
                        out += ",";
                    }
                    out += "\n";
                }
                out += indent + "]";
                return out;
            };

            std::vector<std::string> key_order = {
                "database.path",
                "dht.bootstrap_nodes",
                "dht.bootstrap_infohash",
                "dht.bootstrap_trackers",
                "dht.port",
                "dht.node_id",
                "dht.max_nodes",
                "dht.ping_interval",
                "bittorrent.max_connections",
                "bittorrent.connection_timeout",
                "bittorrent.download_timeout",
                "tracker.announce_interval",
                "tracker.max_trackers",
                "log.level",
                "log.file",
                "log.max_size",
                "log.max_files",
                "web.auto_start",
                "web.port",
                "web.static_dir"
            };

            std::unordered_set<std::string> emitted;

            auto emit_keys = [&](std::ostream& os, const std::vector<std::string>& keys, bool& first) {
                for (const auto& key : keys) {
                    auto it = config_.find(key);
                    if (it == config_.end()) {
                        continue;
                    }
                    const auto& value = it->second;
                    if (!first) {
                        os << ",\n";
                    }
                    first = false;

                    os << "  \"" << key << "\": ";

                    if (value.find(',') != std::string::npos && !value.empty()) {
                        os << format_array(value, "  ");
                    } else {
                        os << format_scalar(value);
                    }

                    emitted.insert(key);
                }
            };

            std::ostringstream out;
            out << "{\n";
            bool first = true;

            emit_keys(out, key_order, first);

            // Emit any remaining keys (deterministic order)
            if (emitted.size() != config_.size()) {
                std::vector<std::string> rest;
                rest.reserve(config_.size());
                for (const auto& [key, _] : config_) {
                    if (emitted.find(key) == emitted.end()) {
                        rest.push_back(key);
                    }
                }
                std::sort(rest.begin(), rest.end());
                emit_keys(out, rest, first);
            }

            out << "\n}\n";

            // Ensure the directory exists
            std::filesystem::path p(config_path_);
            if (!p.parent_path().empty() && !std::filesystem::exists(p.parent_path())) {
                std::filesystem::create_directories(p.parent_path());
            }

            std::ofstream file(config_path_);
            if (!file.is_open()) {
                std::cerr << "Failed to open configuration file for writing: " << config_path_ << std::endl;
                return false;
            }

            file << out.str();

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to save configuration: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> save_async() {
        return std::async(std::launch::async, [this]() {
            return save();
        });
    }

    void set_config_path(const std::string& path) {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_);
        config_path_ = path;
    }

    std::string get_config_path() const {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_, lock::LockManager::LockType::SHARED);
        return config_path_;
    }

    void set_string(const std::string& key, const std::string& value) {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_);
        config_[key] = value;
    }

    std::string get_string(const std::string& key, const std::string& default_value) const {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_, lock::LockManager::LockType::SHARED);
        auto it = config_.find(key);
        return (it != config_.end()) ? it->second : default_value;
    }

    void set_int(const std::string& key, int value) {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_);
        config_[key] = std::to_string(value);
    }

    int get_int(const std::string& key, int default_value) const {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_, lock::LockManager::LockType::SHARED);
        auto it = config_.find(key);
        if (it != config_.end()) {
            try {
                return std::stoi(it->second);
            } catch (const std::exception&) {
                return default_value;
            }
        }
        return default_value;
    }

    void set_bool(const std::string& key, bool value) {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_);
        config_[key] = value ? "1" : "0";
    }

    bool get_bool(const std::string& key, bool default_value) const {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_, lock::LockManager::LockType::SHARED);
        auto it = config_.find(key);
        if (it != config_.end()) {
            return (it->second == "1" || it->second == "true" || it->second == "yes");
        }
        return default_value;
    }

    void set_string_list(const std::string& key, const std::vector<std::string>& values) {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_);
        std::string list_str;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                list_str += ",";
            }
            list_str += values[i];
        }
        config_[key] = list_str;
    }

    std::vector<std::string> get_string_list(const std::string& key) const {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_, lock::LockManager::LockType::SHARED);
        std::vector<std::string> result;
        auto it = config_.find(key);
        if (it != config_.end()) {
            std::stringstream ss(it->second);
            std::string item;
            while (std::getline(ss, item, ',')) {
                result.push_back(item);
            }
        }
        return result;
    }

    void set_endpoint_list(const std::string& key, const std::vector<types::Endpoint>& endpoints) {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_);
        std::vector<std::string> endpoint_strings;
        for (const auto& endpoint : endpoints) {
            endpoint_strings.push_back(endpoint.to_string());
        }
        // Release the lock before calling set_string_list which will acquire its own lock
        lock_guard.reset();
        set_string_list(key, endpoint_strings);
    }

    std::vector<types::Endpoint> get_endpoint_list(const std::string& key) const {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_, lock::LockManager::LockType::SHARED);
        std::vector<types::Endpoint> result;
        // Release the lock before calling get_string_list which will acquire its own lock
        lock_guard.reset();
        auto strings = get_string_list(key);
        for (const auto& str : strings) {
            try {
                // Parse endpoint from string (format: "address:port")
                size_t pos = str.find(':');
                if (pos != std::string::npos) {
                    std::string address = str.substr(0, pos);
                    uint16_t port = std::stoi(str.substr(pos + 1));
                    result.push_back(types::Endpoint(address, port));
                }
            } catch (const std::exception&) {
                // Skip invalid endpoints
            }
        }
        return result;
    }

    bool has_key(const std::string& key) const {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_, lock::LockManager::LockType::SHARED);
        return config_.find(key) != config_.end();
    }

    bool remove_key(const std::string& key) {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_);
        auto it = config_.find(key);
        if (it != config_.end()) {
            config_.erase(it);
            return true;
        }
        return false;
    }

    void clear() {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_);
        config_.clear();
    }

    std::vector<std::string> get_keys() const {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_, lock::LockManager::LockType::SHARED);
        std::vector<std::string> keys;
        for (const auto& [key, _] : config_) {
            keys.push_back(key);
        }
        return keys;
    }

    std::unordered_map<std::string, std::string> get_all() const {
        auto lock_manager = lock::LockManagerSingleton::instance();
        auto lock_guard = lock_manager->get_lock_guard(config_resource_id_, lock::LockManager::LockType::SHARED);
        return config_;
    }

private:
    void apply_missing_defaults() {
        std::string base_dir = Configuration::get_default_base_dir();
        const std::unordered_map<std::string, std::string> defaults = {
            {"database.path", (std::filesystem::path(base_dir) / "bitscrape.db").string()},
            {"dht.bootstrap_nodes", "dht.aelitis.com:6881,router.utorrent.com:6881,router.bittorrent.com:6881"},
            {"dht.port", "6881"},
            {"dht.node_id", ""},
            {"dht.max_nodes", "1000"},
            {"dht.ping_interval", "300"},
            {"dht.bootstrap_infohash", "d2474e86c95b19b8bcfdb92bc12c9d44667cfa36"},
            {"dht.bootstrap_trackers", "udp://tracker.opentrackr.org:1337/announce,udp://tracker.torrent.eu.org:451/announce"},
            {"bittorrent.max_connections", "50"},
            {"bittorrent.connection_timeout", "10"},
            {"bittorrent.download_timeout", "30"},
            {"tracker.announce_interval", "1800"},
            {"tracker.max_trackers", "20"},
            {"tracker.default_trackers", "udp://tracker.opentrackr.org:1337/announce,udp://tracker.torrent.eu.org:451/announce"},
            {"log.level", "debug"},
            {"log.file", (std::filesystem::path(base_dir) / "bitscrape.log").string()},
            {"log.max_size", "10485760"},
            {"log.max_files", "5"},
            {"web.auto_start", "true"},
            {"web.port", "8080"},
            {"web.static_dir", "public"},
            {"crawler.random_discovery", "true"}
        };

        for (const auto& [key, value] : defaults) {
            if (config_.find(key) == config_.end()) {
                config_[key] = value;
            }
        }
    }

    void create_default_configuration() {
        std::string base_dir = Configuration::get_default_base_dir();
        // Set default values
        config_["database.path"] = (std::filesystem::path(base_dir) / "bitscrape.db").string();
        config_["dht.bootstrap_nodes"] = "dht.aelitis.com:6881,router.utorrent.com:6881,router.bittorrent.com:6881"; // Default public routers
        config_["dht.port"] = "6881";
        config_["dht.node_id"] = ""; // Will be generated randomly if empty
        config_["dht.max_nodes"] = "1000";
        config_["dht.ping_interval"] = "300"; // 5 minutes
        config_["dht.bootstrap_infohash"] = "d2474e86c95b19b8bcfdb92bc12c9d44667cfa36"; // OpenOffice 3.3 swarm
        config_["dht.bootstrap_trackers"] = "udp://tracker.opentrackr.org:1337/announce,udp://tracker.torrent.eu.org:451/announce";
        config_["bittorrent.max_connections"] = "50";
        config_["bittorrent.connection_timeout"] = "10"; // 10 seconds
        config_["bittorrent.download_timeout"] = "30"; // 30 seconds
        config_["tracker.announce_interval"] = "1800"; // 30 minutes
        config_["tracker.max_trackers"] = "20";
        config_["log.level"] = "debug";
        config_["log.file"] = (std::filesystem::path(base_dir) / "bitscrape.log").string();
        config_["log.max_size"] = "10485760"; // 10 MB
        config_["log.max_files"] = "5";
        config_["web.auto_start"] = "true"; // Auto-start web interface by default
        config_["web.port"] = "8080"; // Default web interface port
        config_["web.static_dir"] = "public"; // Default static files directory

        save();
    }

    std::string config_path_;
    std::unordered_map<std::string, std::string> config_;
    uint64_t config_resource_id_; // Resource ID for the configuration
};

// Configuration implementation

Configuration::Configuration(const std::string& config_path)
    : impl_(std::make_unique<Impl>(config_path)) {
}

std::string expand_path(const std::string& path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }

    const char* home = std::getenv("HOME");
    if (!home) {
        return path;
    }

    if (path.size() == 1) {
        return home;
    }

    if (path[1] == '/' || path[1] == '\\') {
        return std::string(home) + path.substr(1);
    }

    return path;
}

std::string Configuration::get_default_base_dir() {
    const char* home = std::getenv("HOME");
    std::filesystem::path base_dir;
    if (home) {
        base_dir = std::filesystem::path(home) / ".config" / "bitscrape";
    } else {
        base_dir = std::filesystem::current_path() / ".config" / "bitscrape";
    }
    return base_dir.string();
}

std::string Configuration::get_default_config_path() {
    return (std::filesystem::path(get_default_base_dir()) / "settings.json").string();
}

Configuration::~Configuration() = default;

bool Configuration::load() {
    return impl_->load();
}

std::future<bool> Configuration::load_async() {
    return impl_->load_async();
}

bool Configuration::save() {
    return impl_->save();
}

std::future<bool> Configuration::save_async() {
    return impl_->save_async();
}

void Configuration::set_config_path(const std::string& path) {
    impl_->set_config_path(path);
}

std::string Configuration::get_config_path() const {
    return impl_->get_config_path();
}

void Configuration::set_string(const std::string& key, const std::string& value) {
    impl_->set_string(key, value);
}

std::string Configuration::get_string(const std::string& key, const std::string& default_value) const {
    return impl_->get_string(key, default_value);
}

std::string Configuration::get_path(const std::string& key, const std::string& default_value) const {
    return expand_path(get_string(key, default_value));
}

void Configuration::set_int(const std::string& key, int value) {
    impl_->set_int(key, value);
}

int Configuration::get_int(const std::string& key, int default_value) const {
    return impl_->get_int(key, default_value);
}

void Configuration::set_bool(const std::string& key, bool value) {
    impl_->set_bool(key, value);
}

bool Configuration::get_bool(const std::string& key, bool default_value) const {
    return impl_->get_bool(key, default_value);
}

void Configuration::set_string_list(const std::string& key, const std::vector<std::string>& values) {
    impl_->set_string_list(key, values);
}

std::vector<std::string> Configuration::get_string_list(const std::string& key) const {
    return impl_->get_string_list(key);
}

void Configuration::set_endpoint_list(const std::string& key, const std::vector<types::Endpoint>& endpoints) {
    impl_->set_endpoint_list(key, endpoints);
}

std::vector<types::Endpoint> Configuration::get_endpoint_list(const std::string& key) const {
    return impl_->get_endpoint_list(key);
}

bool Configuration::has_key(const std::string& key) const {
    return impl_->has_key(key);
}

bool Configuration::remove_key(const std::string& key) {
    return impl_->remove_key(key);
}

void Configuration::clear() {
    impl_->clear();
}

std::vector<std::string> Configuration::get_keys() const {
    return impl_->get_keys();
}

std::unordered_map<std::string, std::string> Configuration::get_all() const {
    return impl_->get_all();
}

} // namespace bitscrape::core
