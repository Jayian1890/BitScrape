#pragma once

#include <memory>
#include <string>
#include <vector>
#include <future>
#include <functional>
#include <unordered_map>

#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/node_id.hpp>
#include <bitscrape/types/event_types.hpp>
#include <bitscrape/event/event_bus.hpp>
#include <bitscrape/event/event_processor.hpp>
#include <bitscrape/beacon/beacon.hpp>
#include <bitscrape/storage/storage_manager.hpp>

namespace bitscrape::core {

// Forward declarations
class Configuration;

/**
 * @brief The Controller class is the central component of the application.
 *
 * It manages the application lifecycle, coordinates between different modules,
 * and provides a high-level API for controlling the application.
 */
class Controller {
public:
    struct SanityCheckResult {
        std::string module;
        bool ok;
        std::string message;
    };

    /**
     * @brief Construct a new Controller object
     *
     * @param config_path Path to the configuration file
     */
    explicit Controller(const std::string& config_path = "");

    /**
     * @brief Destroy the Controller object
     */
    ~Controller();

    /**
     * @brief Initialize the controller and all its components
     *
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();

    /**
     * @brief Initialize the controller and all its components asynchronously
     *
     * @return std::future<bool> Future that will contain the result of initialization
     */
    std::future<bool> initialize_async();

    /**
     * @brief Start the controller and all its components
     *
     * @return true if start was successful
     * @return false if start failed
     */
    bool start();

    /**
     * @brief Start the controller and all its components asynchronously
     *
     * @return std::future<bool> Future that will contain the result of start
     */
    std::future<bool> start_async();

    /**
     * @brief Stop the controller and all its components
     *
     * @return true if stop was successful
     * @return false if stop failed
     */
    bool stop();

    /**
     * @brief Stop the controller and all its components asynchronously
     *
     * @return std::future<bool> Future that will contain the result of stop
     */
    std::future<bool> stop_async();

    /**
     * @brief Get the configuration object
     *
     * @return std::shared_ptr<Configuration> The configuration object
     */
    std::shared_ptr<Configuration> get_configuration() const;

    /**
     * @brief Get the storage manager object
     *
     * @return storage::StorageManager& Reference to the storage manager object
     */
    storage::StorageManager& get_storage_manager() const;

    /**
     * @brief Get the event bus
     *
     * @return std::shared_ptr<event::EventBus> The event bus
     */
    std::shared_ptr<event::EventBus> get_event_bus() const;

    /**
     * @brief Get the beacon
     *
     * @return std::shared_ptr<beacon::Beacon> The beacon
     */
    std::shared_ptr<beacon::Beacon> get_beacon() const;

    /**
     * @brief Start crawling the DHT network
     *
     * @return true if crawling was started successfully
     * @return false if crawling failed to start
     */
    bool start_crawling();

    /**
     * @brief Start crawling the DHT network asynchronously
     *
     * @return std::future<bool> Future that will contain the result of starting crawling
     */
    std::future<bool> start_crawling_async();

    /**
     * @brief Stop crawling the DHT network
     *
     * @return true if crawling was stopped successfully
     * @return false if crawling failed to stop
     */
    bool stop_crawling();

    /**
     * @brief Stop crawling the DHT network asynchronously
     *
     * @return std::future<bool> Future that will contain the result of stopping crawling
     */
    std::future<bool> stop_crawling_async();

    /**
     * @brief Get statistics about the crawling process
     *
     * @return std::unordered_map<std::string, std::string> Map of statistics
     */
    std::unordered_map<std::string, std::string> get_statistics() const;

    /**
     * @brief Run lightweight runtime sanity checks for each module
     *
     * @return std::vector<SanityCheckResult> List of module check results
     */
    std::vector<SanityCheckResult> run_sanity_checks();

    /**
     * @brief Get discovered infohashes
     *
     * @param limit Maximum number of infohashes to return
     * @param offset Offset to start from
     * @return std::vector<types::InfoHash> Vector of infohashes
     */
    std::vector<types::InfoHash> get_infohashes(size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Get discovered nodes
     *
     * @param limit Maximum number of nodes to return
     * @param offset Offset to start from
     * @return std::vector<types::NodeID> Vector of node IDs
     */
    std::vector<types::NodeID> get_nodes(size_t limit = 100, size_t offset = 0) const;

    /**
     * @brief Receive a raw Event and dispatch it to the appropriate handler.
     *
     * This method is primarily provided for testing and integration: it allows
     * callers to deliver an Event directly to the Controller for processing.
     */
    void receive_event(const types::Event& event);

private:
    // Event handlers
    void handle_dht_node_discovered(const types::Event& event);
    void handle_dht_infohash_discovered(const types::Event& event);
    void handle_metadata_downloaded(const types::Event& event);
    void handle_error(const types::Event& event);

    // DHT crawling methods
    void perform_random_node_lookups();
    void perform_infohash_lookups();

    // Private implementation
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace bitscrape::core
