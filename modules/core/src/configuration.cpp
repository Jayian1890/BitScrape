#include <bitscrape/core/configuration.hpp>
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
        : config_path_(config_path.empty() ? "bitscrape.conf" : config_path) {
    }

    bool load() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            // Check if file exists
            if (!std::filesystem::exists(config_path_)) {
                // Create default configuration
                create_default_configuration();
                return true;
            }
            
            // Open file
            std::ifstream file(config_path_, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Failed to open configuration file: " << config_path_ << std::endl;
                return false;
            }
            
            // Read file content
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            
            // Decode bencode
            bencode::BencodeDecoder decoder;
            auto value = decoder.decode(std::vector<uint8_t>(content.begin(), content.end()));
            
            if (!value.is_dictionary()) {
                std::cerr << "Invalid configuration format" << std::endl;
                return false;
            }
            
            // Clear current configuration
            config_.clear();
            
            // Parse configuration
            auto dict = value.dictionary();
            for (const auto& [key, value] : dict) {
                if (value.is_string()) {
                    config_[key] = value.string();
                } else if (value.is_integer()) {
                    config_[key] = std::to_string(value.integer());
                } else if (value.is_list()) {
                    // Handle lists
                    auto list = value.list();
                    std::vector<std::string> values;
                    
                    for (const auto& item : list) {
                        if (item.is_string()) {
                            values.push_back(item.string());
                        } else if (item.is_integer()) {
                            values.push_back(std::to_string(item.integer()));
                        }
                    }
                    
                    // Store list as comma-separated string
                    std::string list_str;
                    for (size_t i = 0; i < values.size(); ++i) {
                        if (i > 0) {
                            list_str += ",";
                        }
                        list_str += values[i];
                    }
                    
                    config_[key] = list_str;
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
            // Create bencode dictionary
            bencode::BencodeValue dict = bencode::BencodeValue::create_dictionary();
            
            // Add configuration values
            for (const auto& [key, value] : config_) {
                // Check if value is a list (comma-separated string)
                if (value.find(',') != std::string::npos) {
                    // Parse list
                    std::vector<std::string> items;
                    std::stringstream ss(value);
                    std::string item;
                    
                    while (std::getline(ss, item, ',')) {
                        items.push_back(item);
                    }
                    
                    // Create bencode list
                    bencode::BencodeValue list = bencode::BencodeValue::create_list();
                    
                    for (const auto& item : items) {
                        // Try to parse as integer
                        try {
                            int int_value = std::stoi(item);
                            list.add_to_list(bencode::BencodeValue(int_value));
                        } catch (const std::exception&) {
                            // Not an integer, add as string
                            list.add_to_list(bencode::BencodeValue(item));
                        }
                    }
                    
                    dict.add_to_dictionary(key, list);
                } else {
                    // Try to parse as integer
                    try {
                        int int_value = std::stoi(value);
                        dict.add_to_dictionary(key, bencode::BencodeValue(int_value));
                    } catch (const std::exception&) {
                        // Not an integer, add as string
                        dict.add_to_dictionary(key, bencode::BencodeValue(value));
                    }
                }
            }
            
            // Encode bencode
            bencode::BencodeEncoder encoder;
            auto encoded = encoder.encode(dict);
            
            // Write to file
            std::ofstream file(config_path_, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Failed to open configuration file for writing: " << config_path_ << std::endl;
                return false;
            }
            
            file.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
            
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
                result.push_back(types::Endpoint::from_string(str));
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
        
        // Save default configuration
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
