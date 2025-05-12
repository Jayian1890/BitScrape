#include <bitscrape/core/controller.hpp>
#include <bitscrape/core/configuration.hpp>

#include <bitscrape/event/event_bus.hpp>
#include <bitscrape/event/event_processor.hpp>
#include <bitscrape/beacon/beacon.hpp>
#include <bitscrape/beacon/console_sink.hpp>
#include <bitscrape/beacon/file_sink.hpp>
#include <bitscrape/beacon/event_sink.hpp>

#include <bitscrape/types/dht_node.hpp>
#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/metadata_info.hpp>
#include <bitscrape/types/torrent_info.hpp>
#include <bitscrape/storage/query_interface.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <filesystem>

namespace bitscrape::core {

class Controller::Impl {
public:
    Impl(const std::string& config_path)
        : config_(std::make_shared<Configuration>(config_path)),
          event_bus_(event::create_event_bus()),
          event_processor_(event::create_event_processor()),
          beacon_(std::make_shared<beacon::Beacon>()),
          is_running_(false),
          is_crawling_(false) {

        // Add beacon sinks
        beacon_->add_sink(std::make_unique<beacon::ConsoleSink>());

        // Create a storage manager with a default path from config
        std::string db_path = config_->get_string("database.path", "data/bitscrape.db");

        // Always use disk-based storage
        if (db_path.empty()) {
            // Use a default path if none is provided
            db_path = "data/bitscrape.db";
            beacon_->info(std::string("Using default database path: ") + db_path,
                          types::BeaconCategory::GENERAL);
        }

        // Create parent directories if they don't exist
        try {
            std::filesystem::path path(db_path);
            if (!path.parent_path().empty() && !std::filesystem::exists(path.parent_path())) {
                std::filesystem::create_directories(path.parent_path());
            }
            storage_manager_ = storage::create_storage_manager(db_path, true); // Always persistent
        } catch (const std::exception& e) {
            beacon_->error("Failed to create database directory: ", types::BeaconCategory::GENERAL,
                           e.what());
            db_path = "bitscrape.db"; // Use current directory as fallback
            beacon_->warning("Falling back to current directory: " + db_path,
                             types::BeaconCategory::GENERAL);
            storage_manager_ = storage::create_storage_manager(db_path, true); // Still persistent
        }
    }

    ~Impl() {
        if (is_running_) {
            stop();
        }
    }

    bool initialize() {
        try {
            // Load configuration
            if (!config_->load()) {
                beacon_->error("Failed to load configuration");
                return false;
            }

            // Initialize storage manager
            if (!storage_manager_->initialize()) {
                beacon_->error("Failed to initialize storage manager");
                return false;
            }

            // Initialize event processor
            event_processor_->start(*event_bus_);

            // Subscribe to events
            event_bus_->subscribe<types::Event>([this](const types::Event& event) {
                // Handle DHT node found events
                if (event.type() == types::Event::Type::DHT_NODE_FOUND) {
                    handle_dht_node_discovered(event);
                }
                // Handle DHT infohash found events
                else if (event.type() == types::Event::Type::DHT_INFOHASH_FOUND) {
                    handle_dht_infohash_discovered(event);
                }
                // Handle BitTorrent metadata received events
                else if (event.type() == types::Event::Type::BT_METADATA_RECEIVED) {
                    handle_metadata_downloaded(event);
                }
                // Handle error events
                else if (event.type() == types::Event::Type::SYSTEM_ERROR) {
                    handle_error(event);
                }
            });

            return true;
        } catch (const std::exception& e) {
            beacon_->error("Failed to initialize controller: {}", types::BeaconCategory::GENERAL, e.what());
            return false;
        }
    }

    std::future<bool> initialize_async() {
        return std::async(std::launch::async, [this]() {
            return initialize();
        });
    }

    bool start() {
        if (is_running_) {
            beacon_->warning("Controller is already running");
            return true;
        }

        try {
            // Start components
            // TODO: Start DHT, BitTorrent, and Tracker components

            is_running_ = true;
            beacon_->debug("Controller started successfully");
            return true;
        } catch (const std::exception& e) {
            beacon_->error("Failed to start controller: {}", types::BeaconCategory::GENERAL, e.what());
            return false;
        }
    }

    std::future<bool> start_async() {
        return std::async(std::launch::async, [this]() {
            return start();
        });
    }

    bool stop() {
        if (!is_running_) {
            beacon_->warning("Controller is not running");
            return true;
        }

        try {
            // Stop crawling if active
            if (is_crawling_) {
                stop_crawling();
            }

            // Stop components
            // TODO: Stop DHT, BitTorrent, and Tracker components

            // Stop event processor
            event_processor_->stop();

            // Close persistence
            storage_manager_->close();

            is_running_ = false;
            beacon_->info("Controller stopped successfully");
            return true;
        } catch (const std::exception& e) {
            beacon_->error("Failed to stop controller: {}", types::BeaconCategory::GENERAL, e.what());
            return false;
        }
    }

    std::future<bool> stop_async() {
        return std::async(std::launch::async, [this]() {
            return stop();
        });
    }

    bool start_crawling() {
        if (!is_running_) {
            beacon_->error("Controller is not running");
            return false;
        }

        if (is_crawling_) {
            beacon_->warning("Crawling is already active");
            return true;
        }

        try {
            // Start crawling
            // TODO: Start DHT crawling and metadata downloading

            is_crawling_ = true;
            beacon_->info("Crawling started successfully");
            return true;
        } catch (const std::exception& e) {
            beacon_->error("Failed to start crawling: {}", types::BeaconCategory::GENERAL, e.what());
            return false;
        }
    }

    std::future<bool> start_crawling_async() {
        return std::async(std::launch::async, [this]() {
            return start_crawling();
        });
    }

    bool stop_crawling() {
        if (!is_running_) {
            beacon_->error("Controller is not running");
            return false;
        }

        if (!is_crawling_) {
            beacon_->warning("Crawling is not active");
            return true;
        }

        try {
            // Stop crawling
            // TODO: Stop DHT crawling and metadata downloading

            is_crawling_ = false;
            beacon_->info("Crawling stopped successfully");
            return true;
        } catch (const std::exception& e) {
            beacon_->error("Failed to stop crawling: {}", types::BeaconCategory::GENERAL, e.what());
            return false;
        }
    }

    std::future<bool> stop_crawling_async() {
        return std::async(std::launch::async, [this]() {
            return stop_crawling();
        });
    }

    std::unordered_map<std::string, std::string> get_statistics() const {
        std::unordered_map<std::string, std::string> stats;

        // Get storage manager statistics
        auto storage_stats = storage_manager_->get_statistics();
        stats.insert(storage_stats.begin(), storage_stats.end());

        // Add controller statistics
        stats["controller.running"] = is_running_ ? "true" : "false";
        stats["controller.crawling"] = is_crawling_ ? "true" : "false";

        // Add node and infohash counts from storage
        auto query = storage_manager_->query_interface();
        stats["storage.node_count"] = std::to_string(query->count_nodes());
        stats["storage.infohash_count"] = std::to_string(query->count_infohashes());
        stats["storage.metadata_count"] = std::to_string(query->count_metadata());

        // TODO: Add DHT, BitTorrent, and Tracker statistics

        return stats;
    }

    std::vector<types::InfoHash> get_infohashes(size_t limit, size_t offset) const {
        auto query = storage_manager_->query_interface();
        storage::QueryInterface::InfoHashQueryOptions options;
        options.limit = limit;
        options.offset = offset;
        auto infohashes = query->get_infohashes(options);

        std::vector<types::InfoHash> result;
        for (const auto& model : infohashes) {
            result.push_back(model.info_hash);
        }

        return result;
    }

    std::vector<types::NodeID> get_nodes(size_t limit, size_t offset) const {
        auto query = storage_manager_->query_interface();
        storage::QueryInterface::NodeQueryOptions options;
        options.limit = limit;
        options.offset = offset;
        auto nodes = query->get_nodes(options);

        std::vector<types::NodeID> result;
        for (const auto& model : nodes) {
            result.push_back(model.node_id);
        }

        return result;
    }

    void handle_dht_node_discovered(const types::Event& event) {
        try {
            // For DHT_NODE_FOUND events, we expect the event to contain node information
            // in its data field, which we need to extract
            beacon_->debug("DHT node discovered event received", types::BeaconCategory::DHT);

            // Try to cast the event to a more specific type if available
            // Since we don't have a specific DHT node found event class in the codebase,
            // we'll need to use dynamic_cast to check if it's a custom event type

            // Check if the event has a DHTNode property
            if (auto* dht_event = dynamic_cast<const types::DHTNodeFoundEvent*>(&event)) {
                // Extract node information from the event
                const types::NodeID& node_id = dht_event->node().id();
                const types::Endpoint& endpoint = dht_event->node().endpoint();

                // Store the node in the database
                storage_manager_->store_node(node_id, endpoint, true);

                beacon_->info("Stored DHT node: " + node_id.to_hex().substr(0, 8) + "... at " +
                              endpoint.address() + ":" + std::to_string(endpoint.port()),
                              types::BeaconCategory::DHT);
            } else {
                // If we can't cast to a specific event type, log a warning
                beacon_->warning("Received DHT_NODE_FOUND event with unknown format",
                                 types::BeaconCategory::DHT);
            }
        } catch (const std::exception& e) {
            beacon_->error("Failed to handle DHT node discovery: " + std::string(e.what()),
                           types::BeaconCategory::DHT);
        }
    }

    void handle_dht_infohash_discovered(const types::Event& event) {
        try {
            // For DHT_INFOHASH_FOUND events, we expect the event to contain infohash information
            // in its data field, which we need to extract
            beacon_->debug("DHT infohash discovered event received", types::BeaconCategory::DHT);

            // Try to cast the event to a more specific type if available
            // Since we don't have a specific DHT infohash found event class in the codebase,
            // we'll need to use dynamic_cast to check if it's a custom event type

            // Check if the event has an InfoHash property
            if (auto* dht_event = dynamic_cast<const types::DHTInfoHashFoundEvent*>(&event)) {
                // Extract infohash information from the event
                const types::InfoHash& info_hash = dht_event->info_hash();

                // Store the infohash in the database
                storage_manager_->store_infohash(info_hash);

                beacon_->info("Stored DHT infohash: " + info_hash.to_hex().substr(0, 16) + "...",
                              types::BeaconCategory::DHT);
            } else {
                // If we can't cast to a specific event type, log a warning
                beacon_->warning("Received DHT_INFOHASH_FOUND event with unknown format",
                                 types::BeaconCategory::DHT);
            }
        } catch (const std::exception& e) {
            beacon_->error("Failed to handle DHT infohash discovery: " + std::string(e.what()),
                           types::BeaconCategory::DHT);
        }
    }

    void handle_metadata_downloaded(const types::Event& event) {
        try {
            // For BT_METADATA_RECEIVED events, we expect the event to contain metadata information
            // in its data field, which we need to extract
            beacon_->debug("BitTorrent metadata received event received", types::BeaconCategory::BITTORRENT);

            // Try to cast the event to a more specific type if available
            if (auto* bt_event = dynamic_cast<const bittorrent::MetadataReceivedEvent*>(&event)) {
                // Extract metadata information from the event
                const types::InfoHash& info_hash = bt_event->info_hash();
                const types::MetadataInfo& metadata = bt_event->metadata();

                // Store the metadata in the database
                if (storage_manager_->store_metadata(info_hash, metadata)) {
                    beacon_->info(
                        "Stored metadata for infohash: " + info_hash.to_hex().substr(0, 16) + "...",
                        types::BeaconCategory::BITTORRENT);

                    // Extract and store file information if available
                    const auto& files = metadata.files();
                    if (!files.empty()) {
                        beacon_->info(
                            "Storing " + std::to_string(files.size()) + " files for infohash: " +
                            info_hash.to_hex().substr(0, 16) + "...",
                            types::BeaconCategory::BITTORRENT);

                        // In a real implementation, we would store each file
                        // For now, we'll just log the file information
                        for (const auto& file : files) {
                            beacon_->debug(
                                "File: " + file.first + ", Size: " + std::to_string(file.second) +
                                " bytes",
                                types::BeaconCategory::BITTORRENT);
                        }
                    } else {
                        beacon_->info("No file information available for infohash: " +
                                      info_hash.to_hex().substr(0, 16) + "...",
                                      types::BeaconCategory::BITTORRENT);
                    }
                } else {
                    beacon_->error("Failed to store metadata for infohash: " + info_hash.to_hex(),
                                   types::BeaconCategory::BITTORRENT);
                }
            } else {
                // If we can't cast to a specific event type, log a warning
                beacon_->warning("Received BT_METADATA_RECEIVED event with unknown format",
                                 types::BeaconCategory::BITTORRENT);
            }
        } catch (const std::exception& e) {
            beacon_->error("Failed to handle metadata download: " + std::string(e.what()),
                           types::BeaconCategory::BITTORRENT);
        }
    }

    void handle_error(const types::Event& event) {
        try {
            // For SYSTEM_ERROR events, we expect the event to contain error information
            beacon_->debug("System error event received", types::BeaconCategory::GENERAL);

            // Try to cast the event to a more specific type if available
            if (auto* beacon_event = dynamic_cast<const types::BeaconEvent*>(&event)) {
                // Extract error information from the beacon event
                const std::string& message = beacon_event->message();
                types::BeaconCategory category = beacon_event->category();
                types::BeaconSeverity severity = beacon_event->severity();

                // Log the error with the appropriate category and severity
                if (severity == types::BeaconSeverity::ERROR) {
                    beacon_->error("Error occurred: " + message, category);
                } else if (severity == types::BeaconSeverity::WARNING) {
                    beacon_->warning("Warning occurred: " + message, category);
                } else {
                    beacon_->info("Event received: " + message, category);
                }

                // Log the source location for debugging
                const auto& location = beacon_event->location();
                beacon_->debug("Source: " + std::string(location.file_name()) + ":" +
                               std::to_string(location.line()) + " in " +
                               std::string(location.function_name()),
                               category);
            } else if (auto* tracker_error = dynamic_cast<const tracker::TrackerErrorEvent*>(&
                event)) {
                // Extract error information from tracker error event
                const std::string& error_message = tracker_error->error_message();

                // Log the tracker error
                beacon_->error("Tracker error: " + error_message, types::BeaconCategory::TRACKER);
            } else if (auto* bencode_error = dynamic_cast<const bencode::BencodeErrorEvent*>(&
                event)) {
                // Extract error information from bencode error event
                const std::string& error_message = bencode_error->error_message();

                // Log the bencode error
                beacon_->error("Bencode error: " + error_message, types::BeaconCategory::BENCODE);
            } else {
                // If we can't cast to a specific event type, log a generic error
                beacon_->error("Unknown error event received: " + event.to_string(),
                               types::BeaconCategory::GENERAL);
            }
        } catch (const std::exception& e) {
            beacon_->error("Failed to handle error event: " + std::string(e.what()),
                           types::BeaconCategory::GENERAL);
        }
    }

    // Member variables
    std::shared_ptr<Configuration> config_;
    std::shared_ptr<storage::StorageManager> storage_manager_;
    std::unique_ptr<event::EventBus> event_bus_;
    std::unique_ptr<event::EventProcessor> event_processor_;
    std::shared_ptr<beacon::Beacon> beacon_;
    std::atomic<bool> is_running_;
    std::atomic<bool> is_crawling_;
};

// Controller implementation

Controller::Controller(const std::string& config_path)
    : impl_(std::make_unique<Impl>(config_path)) {
}

Controller::~Controller() = default;

bool Controller::initialize() {
    return impl_->initialize();
}

std::future<bool> Controller::initialize_async() {
    return impl_->initialize_async();
}

bool Controller::start() {
    return impl_->start();
}

std::future<bool> Controller::start_async() {
    return impl_->start_async();
}

bool Controller::stop() {
    return impl_->stop();
}

std::future<bool> Controller::stop_async() {
    return impl_->stop_async();
}

std::shared_ptr<Configuration> Controller::get_configuration() const {
    return impl_->config_;
}

std::shared_ptr<storage::StorageManager> Controller::get_storage_manager() const {
    return impl_->storage_manager_;
}

std::shared_ptr<event::EventBus> Controller::get_event_bus() const {
    // We can't return the unique_ptr directly, so we return a nullptr
    // This method should be updated to return a reference instead
    return nullptr;
}

std::shared_ptr<beacon::Beacon> Controller::get_beacon() const {
    return impl_->beacon_;
}

bool Controller::start_crawling() {
    return impl_->start_crawling();
}

std::future<bool> Controller::start_crawling_async() {
    return impl_->start_crawling_async();
}

bool Controller::stop_crawling() {
    return impl_->stop_crawling();
}

std::future<bool> Controller::stop_crawling_async() {
    return impl_->stop_crawling_async();
}

std::unordered_map<std::string, std::string> Controller::get_statistics() const {
    return impl_->get_statistics();
}

std::vector<types::InfoHash> Controller::get_infohashes(size_t limit, size_t offset) const {
    return impl_->get_infohashes(limit, offset);
}

std::vector<types::NodeID> Controller::get_nodes(size_t limit, size_t offset) const {
    return impl_->get_nodes(limit, offset);
}

void Controller::handle_dht_node_discovered(const types::Event& event) {
    impl_->handle_dht_node_discovered(event);
}

void Controller::handle_dht_infohash_discovered(const types::Event& event) {
    impl_->handle_dht_infohash_discovered(event);
}

void Controller::handle_metadata_downloaded(const types::Event& event) {
    impl_->handle_metadata_downloaded(event);
}

void Controller::handle_error(const types::Event& event) {
    impl_->handle_error(event);
}

} // namespace bitscrape::core
