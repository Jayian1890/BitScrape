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
        std::string db_path = config_->get_string("database.path", "bitscrape.db");
        storage_manager_ = storage::create_storage_manager(db_path);
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

            beacon_->info("Controller initialized successfully");
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
            beacon_->info("Controller started successfully");
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

            // In a real implementation, we would extract the node ID and endpoint from the event
            // For now, we'll just log that we received the event
            beacon_->debug("DHT node discovered event received", types::BeaconCategory::DHT);

            // TODO: Extract node information from the event and store it
            // Example (pseudocode):
            // types::NodeID node_id = extract_node_id_from_event(event);
            // types::Endpoint endpoint = extract_endpoint_from_event(event);
            // storage_manager_->store_node(node_id, endpoint, true);
        } catch (const std::exception& e) {
            beacon_->error("Failed to handle DHT node discovery: {}", types::BeaconCategory::DHT, e.what());
        }
    }

    void handle_dht_infohash_discovered(const types::Event& event) {
        try {
            // For DHT_INFOHASH_FOUND events, we expect the event to contain infohash information
            // in its data field, which we need to extract

            // In a real implementation, we would extract the infohash from the event
            // For now, we'll just log that we received the event
            beacon_->debug("DHT infohash discovered event received", types::BeaconCategory::DHT);

            // TODO: Extract infohash from the event and store it
            // Example (pseudocode):
            // types::InfoHash info_hash = extract_infohash_from_event(event);
            // storage_manager_->store_infohash(info_hash);
        } catch (const std::exception& e) {
            beacon_->error("Failed to handle DHT infohash discovery: {}", types::BeaconCategory::DHT, e.what());
        }
    }

    void handle_metadata_downloaded(const types::Event& event) {
        try {
            // For BT_METADATA_RECEIVED events, we expect the event to contain metadata information
            // in its data field, which we need to extract

            // In a real implementation, we would extract the metadata from the event
            // For now, we'll just log that we received the event
            beacon_->debug("BitTorrent metadata received event received", types::BeaconCategory::BITTORRENT);

            // TODO: Extract metadata from the event and store it
            // Example (pseudocode):
            // types::InfoHash info_hash = extract_infohash_from_event(event);
            // types::MetadataInfo metadata = extract_metadata_from_event(event);
            // storage_manager_->store_metadata(info_hash, metadata);

            // TODO: Extract and store file information if available
            // Example (pseudocode):
            // if (has_file_info(event)) {
            //     for (const auto& file : extract_files_from_event(event)) {
            //         storage_manager_->store_file(info_hash, file.path, file.size);
            //     }
            // }
        } catch (const std::exception& e) {
            beacon_->error("Failed to handle metadata download: {}", types::BeaconCategory::BITTORRENT, e.what());
        }
    }

    void handle_error(const types::Event& event) {
        try {
            // For SYSTEM_ERROR events, we expect the event to contain error information
            // in its data field, which we need to extract

            // In a real implementation, we would extract the error message from the event
            // For now, we'll just log that we received the event
            beacon_->error("System error event received", types::BeaconCategory::GENERAL);

            // TODO: Extract error information from the event and log it
            // Example (pseudocode):
            // std::string error_message = extract_error_message_from_event(event);
            // types::BeaconCategory category = extract_category_from_event(event);
            // beacon_->error("Error occurred: {}", category, error_message);
        } catch (const std::exception& e) {
            beacon_->error("Failed to handle error event: {}", types::BeaconCategory::GENERAL, e.what());
        }
    }

    // Member variables
    std::shared_ptr<Configuration> config_;
    std::shared_ptr<storage::StorageManager> storage_manager_;
    std::shared_ptr<event::EventBus> event_bus_;
    std::shared_ptr<event::EventProcessor> event_processor_;
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

std::shared_ptr<event::EventBus> Controller::get_event_bus() const {
    return impl_->event_bus_;
}

storage::StorageManager& Controller::get_storage_manager() const {
    return *impl_->storage_manager_;
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
