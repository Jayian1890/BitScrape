#include <bitscrape/core/configuration.hpp>
// Keep bencode includes for compatibility with other modules
#include <bitscrape/bencode/bencode_value.hpp>
#include <bitscrape/bencode/bencode_encoder.hpp>
#include <bitscrape/bencode/bencode_decoder.hpp>

#include <fstream>
#include <sstream>
#include <mutex>
#include <filesystem>
#include <iostream>

namespace bitscrape::core {

class Configuration::Impl {
public:
    Impl(const std::string& config_path)
        : config_path_(config_path.empty() ? "bitscrape.json" : config_path) {
    }

    bool load() {
        std::unique_lock lock(mutex_);

        try {
            // Check if file exists
            if (!std::filesystem::exists(config_path_)) {
                lock.unlock();
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
            if (content.empty()) {
                std::cerr << "Empty configuration file" << std::endl;
                return false;
            }

            // Find the opening brace
            size_t pos = content.find('{');
            if (pos == std::string::npos) {
                std::cerr << "Invalid JSON format: missing opening brace" << std::endl;
                return false;
            }

            // Parse JSON object
            pos++; // Move past the opening brace
            while (pos < content.size()) {
                // Skip whitespace
                pos = content.find_first_not_of(" \t\n\r", pos);
                if (pos == std::string::npos || content[pos] == '}') {
                    break; // End of object
                }

                // Find key (must be in quotes)
                if (content[pos] != '"') {
                    std::cerr << "Invalid JSON format: expected key" << std::endl;
                    return false;
                }

                size_t key_start = pos + 1;
                size_t key_end = content.find('"', key_start);
                if (key_end == std::string::npos) {
                    std::cerr << "Invalid JSON format: unterminated key string" << std::endl;
                    return false;
                }

                std::string key = content.substr(key_start, key_end - key_start);
                pos = key_end + 1;

                // Find colon
                pos = content.find(':', pos);
                if (pos == std::string::npos) {
                    std::cerr << "Invalid JSON format: missing colon after key" << std::endl;
                    return false;
                }
                pos++;

                // Skip whitespace
                pos = content.find_first_not_of(" \t\n\r", pos);
                if (pos == std::string::npos) {
                    std::cerr << "Invalid JSON format: unexpected end of file" << std::endl;
                    return false;
                }

                // Parse value based on its type
                if (content[pos] == '"') {
                    // String value
                    size_t value_start = pos + 1;
                    size_t value_end = pos + 1;

                    // Find the closing quote, handling escaped quotes
                    bool escaped = false;
                    while (value_end < content.size()) {
                        if (content[value_end] == '\\') {
                            escaped = !escaped;
                        } else if (content[value_end] == '"' && !escaped) {
                            break;
                        } else {
                            escaped = false;
                        }
                        value_end++;
                    }

                    if (value_end >= content.size()) {
                        std::cerr << "Invalid JSON format: unterminated string value" << std::endl;
                        return false;
                    }

                    std::string value = content.substr(value_start, value_end - value_start);
                    config_[key] = value;
                    pos = value_end + 1;
                } else if (content[pos] == '[') {
                    // Array value
                    size_t array_start = pos + 1;
                    size_t array_end = pos + 1;
                    int bracket_depth = 1;

                    // Find the closing bracket, handling nested arrays
                    while (array_end < content.size() && bracket_depth > 0) {
                        if (content[array_end] == '[') {
                            bracket_depth++;
                        } else if (content[array_end] == ']') {
                            bracket_depth--;
                        }
                        array_end++;
                    }

                    if (bracket_depth != 0) {
                        std::cerr << "Invalid JSON format: unterminated array" << std::endl;
                        return false;
                    }

                    array_end--; // Move back to the closing bracket
                    std::string array_content = content.substr(array_start, array_end - array_start);

                    // Parse array elements
                    std::vector<std::string> elements;
                    size_t elem_pos = 0;

                    while (elem_pos < array_content.size()) {
                        // Skip whitespace
                        elem_pos = array_content.find_first_not_of(" \t\n\r,", elem_pos);
                        if (elem_pos == std::string::npos) {
                            break;
                        }

                        if (array_content[elem_pos] == '"') {
                            // String element
                            size_t elem_start = elem_pos + 1;
                            size_t elem_end = elem_start;

                            // Find the closing quote, handling escaped quotes
                            bool escaped = false;
                            while (elem_end < array_content.size()) {
                                if (array_content[elem_end] == '\\') {
                                    escaped = !escaped;
                                } else if (array_content[elem_end] == '"' && !escaped) {
                                    break;
                                } else {
                                    escaped = false;
                                }
                                elem_end++;
                            }

                            if (elem_end >= array_content.size()) {
                                std::cerr << "Invalid JSON format: unterminated string in array" << std::endl;
                                return false;
                            }

                            std::string element = array_content.substr(elem_start, elem_end - elem_start);
                            elements.push_back(element);
                            elem_pos = elem_end + 1;
                        } else if (isdigit(array_content[elem_pos]) || array_content[elem_pos] == '-') {
                            // Number element
                            size_t elem_start = elem_pos;
                            size_t elem_end = array_content.find_first_of(",]\t\n\r ", elem_pos);
                            if (elem_end == std::string::npos) {
                                elem_end = array_content.size();
                            }

                            std::string element = array_content.substr(elem_start, elem_end - elem_start);
                            elements.push_back(element);
                            elem_pos = elem_end;
                        } else {
                            // Skip invalid elements
                            elem_pos = array_content.find(',', elem_pos);
                            if (elem_pos == std::string::npos) {
                                break;
                            }
                            elem_pos++;
                        }
                    }

                    // Store array as comma-separated string
                    std::string list_str;
                    for (size_t i = 0; i < elements.size(); ++i) {
                        if (i > 0) {
                            list_str += ",";
                        }
                        list_str += elements[i];
                    }

                    config_[key] = list_str;
                    pos = array_end + 1;
                } else if (isdigit(content[pos]) || content[pos] == '-') {
                    // Number value
                    size_t value_start = pos;
                    size_t value_end = content.find_first_of(",}\t\n\r ", pos);
                    if (value_end == std::string::npos) {
                        std::cerr << "Invalid JSON format: unterminated number" << std::endl;
                        return false;
                    }

                    std::string value = content.substr(value_start, value_end - value_start);
                    config_[key] = value;
                    pos = value_end;
                } else if (content.substr(pos, 4) == "true") {
                    // Boolean true
                    config_[key] = "1";
                    pos += 4;
                } else if (content.substr(pos, 5) == "false") {
                    // Boolean false
                    config_[key] = "0";
                    pos += 5;
                } else if (content.substr(pos, 4) == "null") {
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

                if (content[pos] == ',') {
                    pos++; // Move past the comma
                } else if (content[pos] == '}') {
                    break; // End of object
                } else {
                    std::cerr << "Invalid JSON format: expected comma or closing brace" << std::endl;
                    return false;
                }
            }

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to load configuration: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> load_async() {
        return std::async(std::launch::async, [this]() {
            return load();
        });
    }

    bool save() {
        std::lock_guard<std::mutex> lock(mutex_);

        try {
            // Create JSON string
            std::string json = "{\
";

            bool first = true;
            for (const auto& [key, value] : config_) {
                if (!first) {
                    json += ",\
";
                }
                first = false;

                json += "  \"" + key + "\": ";

                // Check if value is a list (comma-separated string)
                if (value.find(',') != std::string::npos && !value.empty()) {
                    // Parse list
                    std::vector<std::string> items;
                    std::stringstream ss(value);
                    std::string item;

                    while (std::getline(ss, item, ',')) {
                        items.push_back(item);
                    }

                    // Create JSON array
                    json += "[";

                    for (size_t i = 0; i < items.size(); ++i) {
                        if (i > 0) {
                            json += ", ";
                        }

                        // Try to parse as integer
                        try {
                            int int_value = std::stoi(items[i]);
                            json += items[i]; // Add as number
                        } catch (const std::exception&) {
                            // Not an integer, add as string
                            json += "\"" + items[i] + "\"";
                        }
                    }

                    json += "]";
                } else {
                    // Try to parse as integer
                    try {
                        int int_value = std::stoi(value);
                        json += value; // Add as number
                    } catch (const std::exception&) {
                        // Check if it's a boolean
                        if (value == "1" || value == "true" || value == "yes") {
                            json += "true";
                        } else if (value == "0" || value == "false" || value == "no") {
                            json += "false";
                        } else if (value.empty()) {
                            json += "null";
                        } else {
                            // Not a number or boolean, add as string
                            json += "\"" + value + "\"";
                        }
                    }
                }
            }

            json += "\
}";

            // Write to file
            std::ofstream file(config_path_);
            if (!file.is_open()) {
                std::cerr << "Failed to open configuration file for writing: " << config_path_ << std::endl;
                return false;
            }

            file << json;

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
        std::lock_guard<std::mutex> lock(mutex_);
        config_path_ = path;
    }

    std::string get_config_path() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_path_;
    }

    void set_string(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_[key] = value;
    }

    std::string get_string(const std::string& key, const std::string& default_value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = config_.find(key);
        return (it != config_.end()) ? it->second : default_value;
    }

    void set_int(const std::string& key, int value) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_[key] = std::to_string(value);
    }

    int get_int(const std::string& key, int default_value) const {
        std::lock_guard<std::mutex> lock(mutex_);
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
        std::lock_guard<std::mutex> lock(mutex_);
        config_[key] = value ? "1" : "0";
    }

    bool get_bool(const std::string& key, bool default_value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = config_.find(key);
        if (it != config_.end()) {
            return (it->second == "1" || it->second == "true" || it->second == "yes");
        }
        return default_value;
    }

    void set_string_list(const std::string& key, const std::vector<std::string>& values) {
        std::lock_guard<std::mutex> lock(mutex_);
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
        std::lock_guard<std::mutex> lock(mutex_);
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
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> endpoint_strings;
        for (const auto& endpoint : endpoints) {
            endpoint_strings.push_back(endpoint.to_string());
        }
        set_string_list(key, endpoint_strings);
    }

    std::vector<types::Endpoint> get_endpoint_list(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<types::Endpoint> result;
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
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.find(key) != config_.end();
    }

    bool remove_key(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = config_.find(key);
        if (it != config_.end()) {
            config_.erase(it);
            return true;
        }
        return false;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.clear();
    }

    std::vector<std::string> get_keys() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> keys;
        for (const auto& [key, _] : config_) {
            keys.push_back(key);
        }
        return keys;
    }

    std::unordered_map<std::string, std::string> get_all() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

private:
    void create_default_configuration() {
        // Set default values
        config_["database.path"] = "bitscrape.db";
        config_["dht.bootstrap_nodes"] = "router.bittorrent.com:6881,dht.transmissionbt.com:6881,router.utorrent.com:6881";
        config_["dht.port"] = "6881";
        config_["dht.node_id"] = ""; // Will be generated randomly if empty
        config_["dht.max_nodes"] = "1000";
        config_["dht.ping_interval"] = "300"; // 5 minutes
        config_["bittorrent.max_connections"] = "50";
        config_["bittorrent.connection_timeout"] = "10"; // 10 seconds
        config_["bittorrent.download_timeout"] = "30"; // 30 seconds
        config_["tracker.announce_interval"] = "1800"; // 30 minutes
        config_["tracker.max_trackers"] = "20";
        config_["log.level"] = "info";
        config_["log.file"] = "bitscrape.log";
        config_["log.max_size"] = "10485760"; // 10 MB
        config_["log.max_files"] = "5";
        config_["web.auto_start"] = "1"; // Auto-start web interface by default
        config_["web.port"] = "8080"; // Default web interface port
        config_["web.static_dir"] = "public"; // Default static files directory

        save();
    }

    std::string config_path_;
    std::unordered_map<std::string, std::string> config_;
    mutable std::mutex mutex_;
};

// Configuration implementation

Configuration::Configuration(const std::string& config_path)
    : impl_(std::make_unique<Impl>(config_path)) {
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
