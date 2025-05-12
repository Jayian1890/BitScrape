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

// BitTorrent module
#include <bitscrape/bittorrent/peer_manager.hpp>
#include <bitscrape/bittorrent/bittorrent_event_processor.hpp>
#include <bitscrape/bittorrent/metadata_exchange.hpp>

// Tracker module
#include <bitscrape/tracker/tracker_manager.hpp>

// DHT module
#include <bitscrape/dht/dht_session.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <filesystem>
#include <random>

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
            // Initialize and start BitTorrent component
            if (!start_bittorrent_component()) {
                beacon_->error("Failed to start BitTorrent component");
                return false;
            }

            // Initialize and start Tracker component
            if (!start_tracker_component()) {
                beacon_->error("Failed to start Tracker component");
                stop_bittorrent_component(); // Clean up already started components
                return false;
            }

            // Initialize and start DHT component
            if (!start_dht_component()) {
                beacon_->error("Failed to start DHT component");
                stop_tracker_component();
                stop_bittorrent_component(); // Clean up already started components
                return false;
            }

            is_running_ = true;
            beacon_->info("Controller started successfully");
            return true;
        } catch (const std::exception& e) {
            beacon_->error("Failed to start controller: {}", types::BeaconCategory::GENERAL, e.what());
            return false;
        }
    }

    bool start_bittorrent_component() {
        try {
            // Create BitTorrent event processor
            bt_event_processor_ = std::make_unique<bittorrent::BitTorrentEventProcessor>();

            // Start the event processor
            bt_event_processor_->start(*event_bus_);

            beacon_->info("BitTorrent component started successfully",
                          types::BeaconCategory::BITTORRENT);
            return true;
        } catch (const std::exception& e) {
            beacon_->error("Failed to start BitTorrent component: " + std::string(e.what()),
                           types::BeaconCategory::BITTORRENT);
            return false;
        }
    }

    bool start_tracker_component() {
        try {
            // Tracker component is initialized on-demand when infohashes are discovered
            beacon_->info("Tracker component initialized successfully",
                          types::BeaconCategory::TRACKER);
            return true;
        } catch (const std::exception& e) {
            beacon_->error("Failed to start Tracker component: " + std::string(e.what()),
                           types::BeaconCategory::TRACKER);
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
            stop_tracker_component();
            stop_bittorrent_component();
            stop_dht_component();

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

    void stop_bittorrent_component() {
        try {
            // Stop BitTorrent event processor
            if (bt_event_processor_) {
                bt_event_processor_->stop();
            }

            // Clear peer managers and metadata exchanges
            peer_managers_.clear();
            metadata_exchanges_.clear();

            beacon_->info("BitTorrent component stopped successfully",
                          types::BeaconCategory::BITTORRENT);
        } catch (const std::exception& e) {
            beacon_->error("Failed to stop BitTorrent component: " + std::string(e.what()),
                           types::BeaconCategory::BITTORRENT);
        }
        bt_event_processor_.reset();
    }

    void stop_tracker_component() {
        try {
            // Clear tracker managers
            tracker_managers_.clear();

            beacon_->info("Tracker component stopped successfully",
                          types::BeaconCategory::TRACKER);
        } catch (const std::exception& e) {
            beacon_->error("Failed to stop Tracker component: " + std::string(e.what()),
                           types::BeaconCategory::TRACKER);
        }
    }

    bool start_dht_component() {
        try {
            // Get DHT configuration
            std::string node_id_str = config_->get_string("dht.node_id", "");
            uint16_t dht_port = static_cast<uint16_t>(config_->get_int("dht.port", 6881));

            // Create a node ID (random if not specified)
            types::NodeID node_id;
            if (!node_id_str.empty()) {
                node_id = types::NodeID(node_id_str);
            }

            // Create the DHT session
            dht_session_ = std::make_unique<dht::DHTSession>(node_id, dht_port, *event_bus_);

            // Get bootstrap nodes from configuration
            std::string bootstrap_nodes_str = config_->get_string("dht.bootstrap_nodes",
                "router.bittorrent.com:6881,dht.transmissionbt.com:6881,router.utorrent.com:6881");

            // Parse bootstrap nodes
            std::vector<types::Endpoint> bootstrap_nodes;
            std::stringstream ss(bootstrap_nodes_str);
            std::string node;
            while (std::getline(ss, node, ',')) {
                size_t pos = node.find(':');
                if (pos != std::string::npos) {
                    std::string host = node.substr(0, pos);
                    uint16_t port = static_cast<uint16_t>(std::stoi(node.substr(pos + 1)));
                    try {
                        // Use the hostname-resolving constructor with AddressType::IPv4
                        bootstrap_nodes.
                            emplace_back(host, port, types::Endpoint::AddressType::IPv4);
                        beacon_->debug("Added bootstrap node: " + host + ":" + std::to_string(port),
                                       types::BeaconCategory::DHT);
                    } catch (const std::exception& e) {
                        beacon_->warning("Failed to resolve bootstrap node: " + host + ":" +
                                         std::to_string(port) + " - " + e.what(),
                                         types::BeaconCategory::DHT);
                        // Continue with other bootstrap nodes even if one fails
                    }
                }
            }

            // Start the DHT session
            if (!dht_session_->start(bootstrap_nodes)) {
                beacon_->error("Failed to start DHT session", types::BeaconCategory::DHT);
                return false;
            }

            beacon_->info("DHT component started successfully", types::BeaconCategory::DHT);
            return true;
        } catch (const std::exception& e) {
            beacon_->error("Failed to start DHT component: " + std::string(e.what()),
                           types::BeaconCategory::DHT);
            return false;
        }
    }

    void stop_dht_component() {
        try {
            // Stop the DHT session
            if (dht_session_) {
                dht_session_->stop();
                dht_session_.reset();
            }

            beacon_->info("DHT component stopped successfully", types::BeaconCategory::DHT);
        } catch (const std::exception& e) {
            beacon_->error("Failed to stop DHT component: " + std::string(e.what()),
                           types::BeaconCategory::DHT);
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
            // Start BitTorrent crawling
            // Start by loading existing infohashes from the database
            auto query = storage_manager_->query_interface();
            storage::QueryInterface::InfoHashQueryOptions options;
            options.limit = 100;
            std::vector<storage::InfoHashModel> infohash_models = query->get_infohashes(options);

            beacon_->info("Loaded " + std::to_string(infohash_models.size()) +
                          " existing infohashes from database", types::BeaconCategory::GENERAL);

            // Create peer managers for existing infohashes
            for (const auto& model : infohash_models) {
                create_peer_manager_for_infohash(model.info_hash);
            }

            // If we don't have enough infohashes, generate some random ones for testing
            if (infohash_models.size() < 5) {
                int num_to_generate = 5 - infohash_models.size();
                beacon_->info("Generating " + std::to_string(num_to_generate) +
                              " random infohashes for testing", types::BeaconCategory::GENERAL);

                for (int i = 0; i < num_to_generate; i++) {
                    // Generate random infohash
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<uint8_t> dist(0, 255);

                    std::vector<uint8_t> hash_bytes(20);
                    for (auto& byte : hash_bytes) {
                        byte = dist(gen);
                    }

                    types::InfoHash random_hash(hash_bytes);

                    // Store the infohash
                    storage_manager_->store_infohash(random_hash);

                    // Create a peer manager for this infohash
                    create_peer_manager_for_infohash(random_hash);

                    beacon_->debug(
                        "Generated random infohash: " + random_hash.to_hex().substr(0, 16) + "...",
                        types::BeaconCategory::DHT);
                }
            }

            // Start DHT crawling if the DHT component is running
            if (dht_session_ && dht_session_->is_running()) {
                beacon_->info("Starting DHT crawling", types::BeaconCategory::DHT);

                // TODO: Implement DHT crawling logic here
                // For now, we'll just use the existing infohashes
            } else {
                beacon_->warning("DHT component is not running, crawling will be limited",
                                 types::BeaconCategory::DHT);
            }

            // Start a background thread to periodically check for new infohashes
            std::thread([this]() {
                try {
                    while (is_crawling_ && is_running_) {
                        // Sleep for a while before checking for new infohashes
                        std::this_thread::sleep_for(std::chrono::seconds(60));

                        if (!is_crawling_ || !is_running_) {
                            break; // Exit the loop if crawling or controller has stopped
                        }

                        // Get the latest infohashes from the database
                        auto query = storage_manager_->query_interface();
                        storage::QueryInterface::InfoHashQueryOptions options;
                        options.limit = 20;
                        std::vector<storage::InfoHashModel> latest_infohashes = query->
                            get_infohashes(options);

                        // Create peer managers for any new infohashes
                        for (const auto& model : latest_infohashes) {
                            create_peer_manager_for_infohash(model.info_hash);
                        }

                        // Generate a new random infohash occasionally
                        if (rand() % 5 == 0) {
                            // 20% chance
                            // Generate random infohash
                            std::random_device rd;
                            std::mt19937 gen(rd());
                            std::uniform_int_distribution<uint8_t> dist(0, 255);

                            std::vector<uint8_t> hash_bytes(20);
                            for (auto& byte : hash_bytes) {
                                byte = dist(gen);
                            }

                            types::InfoHash random_hash(hash_bytes);

                            // Store the infohash
                            storage_manager_->store_infohash(random_hash);

                            // Create a peer manager for this infohash
                            create_peer_manager_for_infohash(random_hash);

                            beacon_->debug("Generated new random infohash: " +
                                           random_hash.to_hex().substr(0, 16) + "...",
                                           types::BeaconCategory::DHT);
                        }
                    }
                } catch (const std::exception& e) {
                    beacon_->error("Error in crawling background thread: " + std::string(e.what()),
                                   types::BeaconCategory::GENERAL);
                }
            }).detach(); // Detach the thread so it runs independently

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
            // Set the crawling flag to false to stop the background thread
            is_crawling_ = false;

            // Give the background thread a moment to notice the flag change
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Stop DHT crawling if the DHT component is running
            if (dht_session_ && dht_session_->is_running()) {
                beacon_->info("Stopping DHT crawling", types::BeaconCategory::DHT);
                // No specific action needed here, just logging
            }

            // Stop all active peer managers to stop metadata downloading
            beacon_->info(
                "Stopping " + std::to_string(peer_managers_.size()) + " active peer managers",
                types::BeaconCategory::BITTORRENT);

            for (const auto& [info_hash, peer_manager] : peer_managers_) {
                try {
                    peer_manager->stop();
                } catch (const std::exception& e) {
                    beacon_->warning(
                        "Failed to stop peer manager for infohash " + info_hash + ": " + e.what(),
                        types::BeaconCategory::BITTORRENT);
                }
            }

            // Cancel any pending tracker announcements
            beacon_->info(
                "Stopping " + std::to_string(tracker_managers_.size()) + " active tracker managers",
                types::BeaconCategory::TRACKER);

            for (const auto& [info_hash, tracker_manager] : tracker_managers_) {
                try {
                    // Send a stopped announce to all trackers
                    std::string peer_id_str(20, '0');
                    uint16_t port = static_cast<uint16_t>(config_->
                        get_int("bittorrent.port", 6881));

                    tracker_manager->announce_async(
                        peer_id_str,
                        port,
                        0, // uploaded
                        0, // downloaded
                        0, // left
                        "stopped"
                        );
                } catch (const std::exception& e) {
                    beacon_->warning(
                        "Failed to cancel tracker announcements for infohash " + info_hash + ": " +
                        e.what(),
                        types::BeaconCategory::TRACKER);
                }
            }

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

        // Add DHT statistics
        if (dht_session_ && dht_session_->is_running()) {
            stats["dht.running"] = "true";
            stats["dht.node_count"] = std::to_string(dht_session_->routing_table().size());
        } else {
            stats["dht.running"] = "false";
            stats["dht.node_count"] = "0";
        }

        // Add BitTorrent statistics
        stats["bittorrent.peer_manager_count"] = std::to_string(peer_managers_.size());
        stats["bittorrent.metadata_exchange_count"] = std::to_string(metadata_exchanges_.size());

        // Count total connected peers across all peer managers
        size_t total_connected_peers = 0;
        for (const auto& [info_hash, peer_manager] : peer_managers_) {
            total_connected_peers += peer_manager->connected_peers().size();
        }
        stats["bittorrent.connected_peer_count"] = std::to_string(total_connected_peers);

        // Add Tracker statistics
        stats["tracker.manager_count"] = std::to_string(tracker_managers_.size());

        // Count total trackers across all tracker managers
        size_t total_trackers = 0;
        for (const auto& [info_hash, tracker_manager] : tracker_managers_) {
            total_trackers += tracker_manager->tracker_urls().size();
        }
        stats["tracker.url_count"] = std::to_string(total_trackers);

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

            // In a real implementation, we would extract the node information from the event
            // and store it in the database. For now, we'll just log a message.
            beacon_->info("Received DHT node discovery event", types::BeaconCategory::DHT);

            // Generate a random node ID and endpoint for testing
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint8_t> dist(0, 255);

            std::vector<uint8_t> id_bytes(20);
            for (auto& byte : id_bytes) {
                byte = dist(gen);
            }

            types::NodeID node_id(id_bytes);
            types::Endpoint endpoint(std::string("192.168.1.1"), 6881);

            // Store the node in the database
            storage_manager_->store_node(node_id, endpoint, true);

            beacon_->info("Stored DHT node: " + node_id.to_hex().substr(0, 8) + "... at " +
                          endpoint.address() + ":" + std::to_string(endpoint.port()),
                          types::BeaconCategory::DHT);
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

            // In a real implementation, we would extract the infohash from the event
            // For now, we'll generate a random infohash for testing
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint8_t> dist(0, 255);

            std::vector<uint8_t> hash_bytes(20);
            for (auto& byte : hash_bytes) {
                byte = dist(gen);
            }

            types::InfoHash info_hash(hash_bytes);

            // Store the infohash in the database
            storage_manager_->store_infohash(info_hash);

            beacon_->info("Stored DHT infohash: " + info_hash.to_hex().substr(0, 16) + "...",
                          types::BeaconCategory::DHT);

            // If crawling is active, create a PeerManager and MetadataExchange for this infohash
            if (is_crawling_ && is_running_) {
                create_peer_manager_for_infohash(info_hash);
            }
        } catch (const std::exception& e) {
            beacon_->error("Failed to handle DHT infohash discovery: " + std::string(e.what()),
                           types::BeaconCategory::DHT);
        }
    }

    void create_peer_manager_for_infohash(const types::InfoHash& info_hash) {
        try {
            // Check if we already have a peer manager for this infohash
            std::string info_hash_str = info_hash.to_hex();
            if (peer_managers_.find(info_hash_str) != peer_managers_.end()) {
                return; // Already have a peer manager for this infohash
            }

            // Generate a random peer ID
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint8_t> dist(0, 255);

            std::vector<uint8_t> peer_id(20);
            for (auto& byte : peer_id) {
                byte = dist(gen);
            }

            // Create a peer manager for this infohash
            int max_connections = config_->get_int("bittorrent.max_connections", 50);
            auto peer_manager = std::make_shared<bittorrent::PeerManager>(
                info_hash, peer_id, max_connections);

            // Start the peer manager
            if (peer_manager->start()) {
                // Add the peer manager to our map
                peer_managers_[info_hash_str] = peer_manager;

                // Create a metadata exchange for this infohash
                auto metadata_exchange = std::make_shared<bittorrent::MetadataExchange>(
                    peer_manager->protocol());
                metadata_exchange->initialize();

                // Add the metadata exchange to our map
                metadata_exchanges_[info_hash_str] = metadata_exchange;

                // Add the peer manager and metadata exchange to the BitTorrent event processor
                if (bt_event_processor_) {
                    bt_event_processor_->add_peer_manager(info_hash, peer_manager);
                    bt_event_processor_->add_metadata_exchange(info_hash, metadata_exchange);
                }

                // Create a tracker manager for this infohash
                auto tracker_manager = std::make_shared<tracker::TrackerManager>(info_hash);

                // Set timeouts from configuration
                int connection_timeout =
                    config_->get_int("bittorrent.connection_timeout", 10) * 1000; // Convert to ms
                int request_timeout = config_->get_int("bittorrent.download_timeout", 30) * 1000;
                // Convert to ms
                tracker_manager->set_connection_timeout(connection_timeout);
                tracker_manager->set_request_timeout(request_timeout);

                // Add the tracker manager to our map
                tracker_managers_[info_hash_str] = tracker_manager;

                beacon_->info(
                    "Created peer manager, metadata exchange, and tracker manager for infohash: " +
                    info_hash.to_hex().substr(0, 16) + "...",
                    types::BeaconCategory::BITTORRENT);

                // Add some tracker URLs from configuration
                std::string tracker_urls = config_->get_string("tracker.urls",
                                                               "udp://tracker.opentrackr.org:1337,udp://tracker.openbittorrent.com:6969,udp://tracker.coppersurfer.tk:6969");

                size_t pos = 0;
                std::string token;
                while ((pos = tracker_urls.find(',')) != std::string::npos) {
                    token = tracker_urls.substr(0, pos);
                    tracker_manager->add_tracker(token);
                    tracker_urls.erase(0, pos + 1);
                }

                // Add the last tracker URL
                if (!tracker_urls.empty()) {
                    tracker_manager->add_tracker(tracker_urls);
                }

                // Announce to trackers
                std::string peer_id_str(peer_id.begin(), peer_id.end());
                uint16_t port = static_cast<uint16_t>(config_->get_int("bittorrent.port", 6881));

                tracker_manager->announce_async(
                    peer_id_str,
                    port,
                    0, // uploaded
                    0, // downloaded
                    0, // left
                    "started"
                    );
            } else {
                beacon_->error("Failed to start peer manager for infohash: " + info_hash.to_hex(),
                               types::BeaconCategory::BITTORRENT);
            }
        } catch (const std::exception& e) {
            beacon_->error("Failed to create peer manager for infohash: " + info_hash.to_hex() +
                           ": " + std::string(e.what()),
                           types::BeaconCategory::BITTORRENT);
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

                        // Log file details for debugging
                        for (const auto& file : files) {
                            beacon_->debug(
                                "File: " + file.first + ", Size: " + std::to_string(file.second) +
                                " bytes",
                                types::BeaconCategory::BITTORRENT);
                        }
                    } else {
                        // If no files are present, this might be a single-file torrent
                        if (metadata.total_size() > 0) {
                            beacon_->info(
                                "Single file torrent: " + metadata.name() + ", Size: " +
                                std::to_string(metadata.total_size()) + " bytes",
                                types::BeaconCategory::BITTORRENT);
                        } else {
                            beacon_->info("No file information available for infohash: " +
                                          info_hash.to_hex().substr(0, 16) + "...",
                                          types::BeaconCategory::BITTORRENT);
                        }
                    }

                    // Create a TorrentInfo object from the metadata
                    types::TorrentInfo torrent_info(info_hash, metadata);

                    // Store the torrent in the database
                    storage_manager_->store_torrent(info_hash, torrent_info);

                    beacon_->info(
                        "Stored torrent info for infohash: " + info_hash.to_hex().substr(0, 16) +
                        "...",
                        types::BeaconCategory::BITTORRENT);

                    // If we have tracker URLs in the torrent_info, add them to the tracker manager
                    std::string info_hash_str = info_hash.to_hex();
                    if (tracker_managers_.find(info_hash_str) != tracker_managers_.end()) {
                        auto& tracker_manager = tracker_managers_[info_hash_str];

                        // Add the main announce URL if available
                        if (!torrent_info.announce().empty()) {
                            tracker_manager->add_tracker(torrent_info.announce());
                            beacon_->debug(
                                "Added tracker URL from torrent info: " + torrent_info.announce(),
                                types::BeaconCategory::TRACKER);
                        }

                        // Add announce list URLs if available
                        for (const auto& announce_url : torrent_info.announce_list()) {
                            tracker_manager->add_tracker(announce_url);
                            beacon_->debug(
                                "Added tracker URL from torrent info announce list: " +
                                announce_url,
                                types::BeaconCategory::TRACKER);
                        }

                        // Re-announce to trackers with the updated metadata
                        if (!torrent_info.announce().empty() || !torrent_info.announce_list().
                            empty()) {
                            // Generate a random peer ID
                            std::random_device rd;
                            std::mt19937 gen(rd());
                            std::uniform_int_distribution<uint8_t> dist(0, 255);

                            std::vector<uint8_t> peer_id(20);
                            for (auto& byte : peer_id) {
                                byte = dist(gen);
                            }

                            std::string peer_id_str(peer_id.begin(), peer_id.end());
                            uint16_t port = static_cast<uint16_t>(config_->get_int(
                                "bittorrent.port", 6881));

                            tracker_manager->announce_async(
                                peer_id_str,
                                port,
                                0,                     // uploaded
                                0,                     // downloaded
                                metadata.total_size(), // left (total size)
                                "started"
                                );

                            beacon_->info("Re-announced to trackers with updated metadata",
                                          types::BeaconCategory::TRACKER);
                        }
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
            } else {
                // For other error events, just log the event type
                beacon_->error("Error event received: " + event.to_string(),
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

    // BitTorrent components
    std::unique_ptr<bittorrent::BitTorrentEventProcessor> bt_event_processor_;
    std::unordered_map<std::string, std::shared_ptr<bittorrent::PeerManager>> peer_managers_;
    std::unordered_map<std::string, std::shared_ptr<bittorrent::MetadataExchange>>
    metadata_exchanges_;

    // Tracker component
    std::unordered_map<std::string, std::shared_ptr<tracker::TrackerManager>> tracker_managers_;

    // DHT component
    std::unique_ptr<dht::DHTSession> dht_session_;
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
