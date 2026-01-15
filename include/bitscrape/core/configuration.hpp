#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <future>
#include <memory>

#include <bitscrape/types/endpoint.hpp>

namespace bitscrape::core {

/**
 * @brief The Configuration class manages application settings.
 *
 * It provides methods for loading, saving, and accessing configuration values.
 */
class Configuration {
public:
    /**
     * @brief Construct a new Configuration object
     *
     * @param config_path Path to the configuration file
     */
    explicit Configuration(const std::string& config_path = "");

    /**
     * @brief Get the default configuration file path (~/.config/bitscrape/settings.json)
     *
     * @return std::string Default path to the configuration file
     */
    static std::string get_default_config_path();

    /**
     * @brief Get the default base directory for configuration and data (~/.config/bitscrape/)
     *
     * @return std::string Default base directory
     */
    static std::string get_default_base_dir();

    /**
     * @brief Destroy the Configuration object
     */
    ~Configuration();

    /**
     * @brief Load configuration from file
     *
     * @return true if loading was successful
     * @return false if loading failed
     */
    bool load();

    /**
     * @brief Load configuration from file asynchronously
     *
     * @return std::future<bool> Future that will contain the result of loading
     */
    std::future<bool> load_async();

    /**
     * @brief Save configuration to file
     *
     * @return true if saving was successful
     * @return false if saving failed
     */
    bool save();

    /**
     * @brief Save configuration to file asynchronously
     *
     * @return std::future<bool> Future that will contain the result of saving
     */
    std::future<bool> save_async();

    /**
     * @brief Set the configuration file path
     *
     * @param path Path to the configuration file
     */
    void set_config_path(const std::string& path);

    /**
     * @brief Get the configuration file path
     *
     * @return std::string Path to the configuration file
     */
    std::string get_config_path() const;

    /**
     * @brief Set a string value in the configuration
     *
     * @param key Configuration key
     * @param value Configuration value
     */
    void set_string(const std::string& key, const std::string& value);

    /**
     * @brief Get a string value from the configuration
     *
     * @param key Configuration key
     * @param default_value Default value to return if key is not found
     * @return std::string Configuration value
     */
    std::string get_string(const std::string& key, const std::string& default_value = "") const;

    /**
     * @brief Get a path value from the configuration (expands ~)
     *
     * @param key Configuration key
     * @param default_value Default value to return if key is not found
     * @return std::string Expanded path
     */
    std::string get_path(const std::string& key, const std::string& default_value = "") const;

    /**
     * @brief Set an integer value in the configuration
     *
     * @param key Configuration key
     * @param value Configuration value
     */
    void set_int(const std::string& key, int value);

    /**
     * @brief Get an integer value from the configuration
     *
     * @param key Configuration key
     * @param default_value Default value to return if key is not found
     * @return int Configuration value
     */
    int get_int(const std::string& key, int default_value = 0) const;

    /**
     * @brief Set a boolean value in the configuration
     *
     * @param key Configuration key
     * @param value Configuration value
     */
    void set_bool(const std::string& key, bool value);

    /**
     * @brief Get a boolean value from the configuration
     *
     * @param key Configuration key
     * @param default_value Default value to return if key is not found
     * @return bool Configuration value
     */
    bool get_bool(const std::string& key, bool default_value = false) const;

    /**
     * @brief Set a list of strings in the configuration
     *
     * @param key Configuration key
     * @param values Configuration values
     */
    void set_string_list(const std::string& key, const std::vector<std::string>& values);

    /**
     * @brief Get a list of strings from the configuration
     *
     * @param key Configuration key
     * @return std::vector<std::string> Configuration values
     */
    std::vector<std::string> get_string_list(const std::string& key) const;

    /**
     * @brief Set a list of endpoints in the configuration
     *
     * @param key Configuration key
     * @param endpoints Configuration values
     */
    void set_endpoint_list(const std::string& key, const std::vector<types::Endpoint>& endpoints);

    /**
     * @brief Get a list of endpoints from the configuration
     *
     * @param key Configuration key
     * @return std::vector<types::Endpoint> Configuration values
     */
    std::vector<types::Endpoint> get_endpoint_list(const std::string& key) const;

    /**
     * @brief Check if a key exists in the configuration
     *
     * @param key Configuration key
     * @return true if key exists
     * @return false if key does not exist
     */
    bool has_key(const std::string& key) const;

    /**
     * @brief Remove a key from the configuration
     *
     * @param key Configuration key
     * @return true if key was removed
     * @return false if key did not exist
     */
    bool remove_key(const std::string& key);

    /**
     * @brief Clear all configuration values
     */
    void clear();

    /**
     * @brief Get all configuration keys
     *
     * @return std::vector<std::string> List of configuration keys
     */
    std::vector<std::string> get_keys() const;

    /**
     * @brief Get all configuration values as a map
     *
     * @return std::unordered_map<std::string, std::string> Map of configuration values
     */
    std::unordered_map<std::string, std::string> get_all() const;

private:
    // Private implementation
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace bitscrape::core
