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

            // Register event handlers
            // TODO: Register event handlers for DHT, BitTorrent, and Tracker events

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
        // TODO: Implement DHT node discovery handling
    }

    void handle_dht_infohash_discovered(const types::Event& event) {
        // TODO: Implement DHT infohash discovery handling
    }

    void handle_metadata_downloaded(const types::Event& event) {
        // TODO: Implement metadata download handling
    }

    void handle_error(const types::Event& event) {
        // TODO: Implement error handling
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
