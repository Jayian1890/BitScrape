#include "bitscrape/types/beacon_types.hpp"
#include <bitscrape/core/configuration.hpp>
#include <bitscrape/core/controller.hpp>

#include <bitscrape/beacon/beacon.hpp>
#include <bitscrape/beacon/console_sink.hpp>
#include <bitscrape/beacon/event_sink.hpp>
#include <bitscrape/beacon/file_sink.hpp>
#include <bitscrape/event/event_bus.hpp>
#include <bitscrape/event/event_processor.hpp>

#include <bitscrape/storage/database.hpp>
#include <bitscrape/storage/query_interface.hpp>
#include <bitscrape/types/dht_node.hpp>
#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/metadata_info.hpp>
#include <bitscrape/types/torrent_info.hpp>

// BitTorrent module
#include <bitscrape/bittorrent/bittorrent_event_processor.hpp>
#include <bitscrape/bittorrent/metadata_exchange.hpp>
#include <bitscrape/bittorrent/peer_manager.hpp>

// Tracker module
#include <bitscrape/tracker/tracker_manager.hpp>

// DHT module
#include <bitscrape/dht/dht_events.hpp>
#include <bitscrape/dht/dht_session.hpp>

// Lock module
#include <bitscrape/lock/lock_guard.hpp>
#include <bitscrape/lock/lock_manager_singleton.hpp>

#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>

namespace bitscrape::core {

class Controller::Impl {
public:
  Impl(const std::string &config_path)
      : config_(std::make_shared<Configuration>(config_path)),
        event_bus_(event::create_event_bus()),
        event_processor_(event::create_event_processor()),
        beacon_(std::make_shared<beacon::Beacon>()), is_running_(false),
        is_crawling_(false) {

    // Register resources with the LockManager
    auto lock_manager = lock::LockManagerSingleton::instance(beacon_);
    controller_state_resource_id_ = lock_manager->register_resource(
        "controller_state", lock::LockManager::LockPriority::HIGH);

    // Add beacon sinks
    beacon_->add_sink(std::make_unique<beacon::ConsoleSink>());

    // Storage manager creation is deferred until initialize() so tests can
    // tweak configuration (e.g., database.path) before the DB is created.
    storage_manager_ = nullptr; // Will be created in initialize()

    // Note: we still keep the beacon active for logging during initialization
  }

  ~Impl() {
    auto lock_manager = lock::LockManagerSingleton::instance();
    auto lock_guard = lock_manager->get_lock_guard(
        controller_state_resource_id_, lock::LockManager::LockType::SHARED);
    if (is_running_) {
      lock_guard.reset(); // Release the lock before calling stop()
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

      // Initialize storage manager (create it here if deferred)
      if (!storage_manager_) {
        std::string default_db_path = (std::filesystem::path(Configuration::get_default_base_dir()) / "bitscrape.db").string();
        std::string db_path = config_->get_path("database.path", default_db_path);
        if (db_path.empty()) {
          db_path = default_db_path;
          beacon_->info(std::string("Using default database path: ") + db_path,
                        types::BeaconCategory::GENERAL);
        }

        try {
          std::filesystem::path path(db_path);
          if (!path.parent_path().empty() &&
              !std::filesystem::exists(path.parent_path())) {
            std::filesystem::create_directories(path.parent_path());
          }
          storage_manager_ = storage::create_storage_manager(db_path, true);
        } catch (const std::exception &e) {
          beacon_->error("Failed to create database directory: ",
                         types::BeaconCategory::GENERAL, e.what());
          db_path = (std::filesystem::path(Configuration::get_default_base_dir()) / "bitscrape.db").string(); 
          beacon_->warning("Falling back to default directory: " + db_path,
                           types::BeaconCategory::GENERAL);
          storage_manager_ = storage::create_storage_manager(db_path, true);
        }
      }

      if (!storage_manager_->initialize()) {
        beacon_->error("Failed to initialize storage manager");
        return false;
      }

      // Initialize event processor
      event_processor_->start(*event_bus_);

      // Subscribe to events
      event_bus_->subscribe<types::Event>([this](const types::Event &event) {
        // Handle DHT node found events
        if (event.type() == types::Event::Type::DHT_NODE_FOUND) {
          handle_dht_node_discovered(event);
        }
        // Handle DHT infohash found events
        else if (event.type() == types::Event::Type::DHT_INFOHASH_FOUND) {
          handle_dht_infohash_discovered(event);
        }
        // Handle BitTorrent metadata received events. Some BitTorrent events
        // are tagged as USER_DEFINED, so we also match on the concrete type.
        else if (event.type() == types::Event::Type::BT_METADATA_RECEIVED ||
                 dynamic_cast<const bittorrent::MetadataReceivedEvent *>(
                     &event) != nullptr) {
          handle_metadata_downloaded(event);
        }
        // Handle error events
        else if (event.type() == types::Event::Type::SYSTEM_ERROR) {
          handle_error(event);
        }
      });

      return true;
    } catch (const std::exception &e) {
      beacon_->error("Failed to initialize controller: {}",
                     types::BeaconCategory::GENERAL, e.what());
      return false;
    }
  }

  std::future<bool> initialize_async() {
    return std::async(std::launch::async, [this]() { return initialize(); });
  }

  bool start() {
    beacon_->debug(std::string("start() called, is_running=") +
                       (is_running_ ? "true" : "false"),
                   types::BeaconCategory::GENERAL);
    auto lock_manager = lock::LockManagerSingleton::instance();
    auto lock_guard =
        lock_manager->get_lock_guard(controller_state_resource_id_);
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
      stop_discovery_threads_ = false;
      start_peer_refresh_thread();
      beacon_->info("Controller started successfully");
      return true;
    } catch (const std::exception &e) {
      beacon_->error("Failed to start controller: {}",
                     types::BeaconCategory::GENERAL, e.what());
      return false;
    }
  }

  void start_peer_refresh_thread() {
    peer_refresh_thread_ = std::thread([this]() {
      beacon_->info("Peer refresh thread started",
                    types::BeaconCategory::GENERAL);

      // Wait a bit after start before first refresh
      for (int i = 0; i < 30 && !stop_discovery_threads_; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }

      while (!stop_discovery_threads_) {
        try {
          refresh_peers();
        } catch (const std::exception &e) {
          beacon_->error("Error in peer refresh loop: " + std::string(e.what()));
        }

        // Sleep for 2 minutes or until stopped
        for (int i = 0; i < 120 && !stop_discovery_threads_; ++i) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      }

      beacon_->info("Peer refresh thread stopping",
                    types::BeaconCategory::GENERAL);
    });
  }

  void refresh_peers() {
    std::vector<types::InfoHash> hashes_to_refresh;

    {
      std::lock_guard<std::mutex> lock(maps_mutex_);
      // Collect targets that need metadata
      for (auto const &[hash_str, pm] : peer_managers_) {
        auto hash = pm->info_hash();
        auto metadata =
            storage_manager_->query_interface()->get_metadata(hash);
        if (!metadata) {
          hashes_to_refresh.push_back(hash);
        }
      }
    }

    // Clean up finished futures
    {
      std::lock_guard<std::mutex> lock(maps_mutex_);
      background_futures_.erase(
          std::remove_if(background_futures_.begin(), background_futures_.end(),
                         [](const std::future<void> &f) {
                           return f.wait_for(std::chrono::seconds(0)) ==
                                  std::future_status::ready;
                         }),
          background_futures_.end());
    }

    for (const auto &hash : hashes_to_refresh) {
      beacon_->debug(
          "Refreshing peers for infohash: " + hash.to_hex().substr(0, 8),
          types::BeaconCategory::GENERAL);

      // DHT lookup
      if (dht_session_) {
        auto dht_task = std::async(std::launch::async, [this, hash]() {
          try {
            dht_session_->find_peers(hash);
          } catch (...) {
          }
        });
        {
          std::lock_guard<std::mutex> lock(maps_mutex_);
          background_futures_.push_back(std::move(dht_task));
        }
      }

      // Tracker announcement
      std::string hash_str = hash.to_hex();
      bool found_managers = false;
      std::shared_ptr<tracker::TrackerManager> tm;
      std::shared_ptr<bittorrent::PeerManager> pm;

      {
        std::lock_guard<std::mutex> lock(maps_mutex_);
        if (tracker_managers_.count(hash_str) && peer_managers_.count(hash_str)) {
          tm = tracker_managers_[hash_str];
          pm = peer_managers_[hash_str];
          found_managers = true;
        }
      }

      if (found_managers) {
        std::string peer_id_str(pm->peer_id().begin(), pm->peer_id().end());
        uint16_t port =
            static_cast<uint16_t>(config_->get_int("bittorrent.port", 6881));

        // Trigger another announce to get more peers
        auto future = tm->announce_async(peer_id_str, port, 0, 0, 0, "");
        auto task = std::async(
            std::launch::async,
            [this, future = std::move(future), hash]() mutable {
              try {
                auto responses = future.get();
                for (const auto &[url, response] : responses) {
                  if (!response.has_error()) {
                    for (const auto &peer : response.peers()) {
                      event_bus_->publish(
                          bittorrent::PeerDiscoveredEvent(hash, peer));
                    }
                  }
                }
              } catch (...) {
              }
            });
        
        {
          std::lock_guard<std::mutex> lock(maps_mutex_);
          background_futures_.push_back(std::move(task));
        }
      }
    }
  }

  bool start_bittorrent_component() {
    try {
      // Create BitTorrent event processor
      bt_event_processor_ =
          std::make_unique<bittorrent::BitTorrentEventProcessor>();

      // Start the event processor
      bt_event_processor_->start(*event_bus_);

      beacon_->info("BitTorrent component started successfully",
                    types::BeaconCategory::BITTORRENT);
      return true;
    } catch (const std::exception &e) {
      beacon_->error("Failed to start BitTorrent component: " +
                         std::string(e.what()),
                     types::BeaconCategory::BITTORRENT);
      return false;
    }
  }

  bool start_tracker_component() {
    try {
      // Tracker component is initialized on-demand when infohashes are
      // discovered
      beacon_->info("Tracker component initialized successfully",
                    types::BeaconCategory::TRACKER);
      return true;
    } catch (const std::exception &e) {
      beacon_->error("Failed to start Tracker component: " +
                         std::string(e.what()),
                     types::BeaconCategory::TRACKER);
      return false;
    }
  }

  std::future<bool> start_async() {
    return std::async(std::launch::async, [this]() { return start(); });
  }

  bool stop() {
    beacon_->debug(std::string("stop() called, is_running=") +
                       (is_running_ ? "true" : "false"),
                   types::BeaconCategory::GENERAL);
    auto lock_manager = lock::LockManagerSingleton::instance();
    auto lock_guard =
        lock_manager->get_lock_guard(controller_state_resource_id_);
    if (!is_running_) {
      beacon_->warning("Controller is not running");
      return true;
    }

    try {
      stop_discovery_threads_ = true;
      if (peer_refresh_thread_.joinable()) {
        peer_refresh_thread_.join();
      }

      // Stop crawling if active
      if (is_crawling_) {
        lock_guard.reset(); // Release the lock before calling stop_crawling()
        stop_crawling();
        lock_guard = lock_manager->get_lock_guard(
            controller_state_resource_id_); // Re-acquire the lock
      }

      // Stop components
      stop_tracker_component();
      stop_bittorrent_component();
      stop_dht_component();

      // Stop event processor
      event_processor_->stop();

      // Close persistence
      if (storage_manager_) {
        beacon_->debug(std::string("Closing storage manager, use_count=") +
                           std::to_string(storage_manager_.use_count()),
                       types::BeaconCategory::GENERAL);
        storage_manager_->close();
      } else {
        beacon_->debug("No storage manager to close",
                       types::BeaconCategory::GENERAL);
      }

      is_running_ = false;
      beacon_->info("Controller stopped successfully");
      return true;
    } catch (const std::exception &e) {
      beacon_->error("Failed to stop controller: {}",
                     types::BeaconCategory::GENERAL, e.what());
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
    } catch (const std::exception &e) {
      beacon_->error("Failed to stop BitTorrent component: " +
                         std::string(e.what()),
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
    } catch (const std::exception &e) {
      beacon_->error("Failed to stop Tracker component: " +
                         std::string(e.what()),
                     types::BeaconCategory::TRACKER);
    }
  }

  bool start_dht_component() {
    try {
      // Get DHT configuration
      std::string node_id_str = config_->get_string("dht.node_id", "");
      uint16_t dht_port =
          static_cast<uint16_t>(config_->get_int("dht.port", 6881));

      // Create a node ID (random if not specified)
      types::NodeID node_id;
      if (!node_id_str.empty()) {
        node_id = types::NodeID(node_id_str);
      }

      // Create the DHT session with the lock manager
      auto lock_manager = lock::LockManagerSingleton::instance();
      dht_session_ = std::make_unique<dht::DHTSession>(
          node_id, dht_port, *event_bus_, lock_manager);

      // Set up callback for passive infohash discovery
      dht_session_->set_infohash_callback([this](const types::InfoHash& infohash) {
        // Create and dispatch a DHT_INFOHASH_FOUND event
        dht::DHTInfoHashFoundEvent event(infohash);
        event_bus_->publish(event);
        
        // Also log it directly for now (optional, duplicate of what handler does)
        beacon_->info("Discovered infohash: " + infohash.to_hex().substr(0, 16) + "...",
                     types::BeaconCategory::DHT);
      });

      // Get bootstrap nodes from configuration
      std::string bootstrap_nodes_str =
          config_->get_string("dht.bootstrap_nodes",
                              "router.bittorrent.com:6881,dht.transmissionbt."
                              "com:6881,router.utorrent.com:6881");

      // Optionally derive bootstrap peers from a tracker announce for a known
      // infohash
      std::string bootstrap_infohash =
          config_->get_string("dht.bootstrap_infohash", "");
      std::string bootstrap_trackers =
          config_->get_string("dht.bootstrap_trackers", "");

      // Parse bootstrap nodes
      std::vector<types::Endpoint> bootstrap_nodes;
      std::stringstream ss(bootstrap_nodes_str);
      std::string node;
      while (std::getline(ss, node, ',')) {
        size_t pos = node.find(':');
        if (pos != std::string::npos) {
          std::string host = node.substr(0, pos);
          uint16_t port =
              static_cast<uint16_t>(std::stoi(node.substr(pos + 1)));
          try {
            // Use the hostname-resolving constructor with AddressType::IPv4
            bootstrap_nodes.emplace_back(host, port,
                                         types::Endpoint::AddressType::IPv4);
            beacon_->debug("Added bootstrap node: " + host + ":" +
                               std::to_string(port),
                           types::BeaconCategory::DHT);
          } catch (const std::exception &e) {
            beacon_->warning("Failed to resolve bootstrap node: " + host + ":" +
                                 std::to_string(port) + " - " + e.what(),
                             types::BeaconCategory::DHT);
            // Continue with other bootstrap nodes even if one fails
          }
        }
      }

      // If a bootstrap infohash is provided, try seeding via tracker peers
      if (!bootstrap_infohash.empty() && !bootstrap_trackers.empty()) {
        try {
          auto info_hash = types::InfoHash::from_hex(bootstrap_infohash);
          tracker::TrackerManager tm(info_hash);

          std::stringstream tss(bootstrap_trackers);
          std::string tracker_url;
          while (std::getline(tss, tracker_url, ',')) {
            if (!tracker_url.empty()) {
              tm.add_tracker(tracker_url);
            }
          }

          // Build a simple random 20-byte peer ID
          std::array<uint8_t, 20> peer_id_bytes{};
          std::uniform_int_distribution<int> dist(0, 255);
          std::random_device rd;
          std::mt19937 gen(rd());
          for (auto &b : peer_id_bytes) {
            b = static_cast<uint8_t>(dist(gen));
          }
          std::string peer_id(peer_id_bytes.begin(), peer_id_bytes.end());

          auto responses = tm.announce(peer_id, dht_port, 0, 0, 0, "started");
          std::unordered_set<std::string> seen;
          for (const auto &[url, resp] : responses) {
            (void)url;
            for (const auto &peer_addr : resp.peers()) {
              if (!peer_addr.is_valid()) {
                continue;
              }
              auto endpoint_key = peer_addr.to_string();
              if (seen.insert(endpoint_key).second) {
                try {
                  bootstrap_nodes.emplace_back(
                      peer_addr.to_string(), peer_addr.port(),
                      types::Endpoint::AddressType::IPv4);
                } catch (const std::exception &) {
                  // Ignore peers that fail resolution
                }
              }
            }
          }

          if (!bootstrap_nodes.empty()) {
            beacon_->info("Derived DHT bootstrap peers from tracker announce",
                          types::BeaconCategory::DHT);
          }
        } catch (const std::exception &e) {
          beacon_->warning("Tracker-derived bootstrap failed: " +
                               std::string(e.what()),
                           types::BeaconCategory::DHT);
        }
      }

      // Start the DHT session
      if (!dht_session_->start(bootstrap_nodes)) {
        beacon_->error("Failed to start DHT session",
                       types::BeaconCategory::DHT);
        return false;
      }

      beacon_->info("DHT component started successfully",
                    types::BeaconCategory::DHT);
      return true;
    } catch (const std::exception &e) {
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

      beacon_->info("DHT component stopped successfully",
                    types::BeaconCategory::DHT);
    } catch (const std::exception &e) {
      beacon_->error("Failed to stop DHT component: " + std::string(e.what()),
                     types::BeaconCategory::DHT);
    }
  }

  std::future<bool> stop_async() {
    return std::async(std::launch::async, [this]() { return stop(); });
  }

  bool start_crawling() {
    auto lock_manager = lock::LockManagerSingleton::instance();
    auto lock_guard =
        lock_manager->get_lock_guard(controller_state_resource_id_);
    if (!is_running_) {
      beacon_->error("Controller is not running");
      return false;
    }

    if (is_crawling_) {
      beacon_->warning("Crawling is already active");
      return true;
    }

    try {
      // Start by loading existing infohashes from the database (including bootstrap hash)
      std::string bootstrap_infohash_hex = config_->get_string("dht.bootstrap_infohash", "");
      if (!bootstrap_infohash_hex.empty()) {
        try {
          auto bootstrap_hash = types::InfoHash::from_hex(bootstrap_infohash_hex);
          storage_manager_->store_infohash(bootstrap_hash);
          create_peer_manager_for_infohash(bootstrap_hash);
          beacon_->info("Added bootstrap infohash to crawl: " + bootstrap_infohash_hex, 
                        types::BeaconCategory::GENERAL);
        } catch (...) {
          beacon_->warning("Invalid bootstrap infohash hex: " + bootstrap_infohash_hex,
                           types::BeaconCategory::GENERAL);
        }
      }

      auto query = storage_manager_->query_interface();
      storage::QueryInterface::InfoHashQueryOptions options;
      options.limit = 100;
      std::vector<storage::InfoHashModel> infohash_models =
          query->get_infohashes(options);

      beacon_->info("Loaded " + std::to_string(infohash_models.size()) +
                        " existing infohashes from database",
                    types::BeaconCategory::GENERAL);

      // Create peer managers for existing infohashes
      for (const auto &model : infohash_models) {
        create_peer_manager_for_infohash(model.info_hash);
      }

      // If we don't have enough infohashes, optionally generate some random
      // ones for testing This is disabled by default in production - enable
      // with crawler.generate_test_infohashes=true
      bool generate_test_hashes =
          config_->get_bool("crawler.generate_test_infohashes", false);
      if (generate_test_hashes && infohash_models.size() < 5) {
        int num_to_generate = 5 - infohash_models.size();
        beacon_->info("Generating " + std::to_string(num_to_generate) +
                          " random infohashes for testing "
                          "(crawler.generate_test_infohashes=true)",
                      types::BeaconCategory::GENERAL);

        for (int i = 0; i < num_to_generate; i++) {
          // Generate random infohash
          std::random_device rd;
          std::mt19937 gen(rd());
          std::uniform_int_distribution<uint8_t> dist(0, 255);

          std::vector<uint8_t> hash_bytes(20);
          for (auto &byte : hash_bytes) {
            byte = dist(gen);
          }

          types::InfoHash random_hash(hash_bytes);

          // Store the infohash
          storage_manager_->store_infohash(random_hash);

          // Create a peer manager for this infohash
          create_peer_manager_for_infohash(random_hash);

          beacon_->debug("Generated random infohash: " +
                             random_hash.to_hex().substr(0, 16) + "...",
                         types::BeaconCategory::DHT);
        }
      }

      // Start DHT crawling if the DHT component is running
      if (dht_session_ && dht_session_->is_running()) {
        beacon_->info("Starting DHT crawling", types::BeaconCategory::DHT);

        // Start a background thread for DHT crawling - assign to member for
        // join
        dht_crawling_thread_ = std::thread([this]() {
          try {
            // Continue crawling until stopped
            while (is_crawling_ && is_running_) {
              // Perform random node lookups to discover new nodes
              perform_random_node_lookups();

              // Sleep for a while before the next round of lookups
              std::this_thread::sleep_for(std::chrono::seconds(30));

              if (!is_crawling_ || !is_running_) {
                break; // Exit the loop if crawling or controller has stopped
              }
            }
          } catch (const std::exception &e) {
            beacon_->error("Error in DHT crawling thread: " +
                               std::string(e.what()),
                           types::BeaconCategory::DHT);
          }
        });
      } else {
        beacon_->warning(
            "DHT component is not running, crawling will be limited",
            types::BeaconCategory::DHT);
      }

      // Start a background thread to periodically check for new infohashes
      infohash_check_thread_ = std::thread([this]() {
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
            std::vector<storage::InfoHashModel> latest_infohashes =
                query->get_infohashes(options);

            // Create peer managers for any new infohashes
            for (const auto &model : latest_infohashes) {
              create_peer_manager_for_infohash(model.info_hash);
            }

            bool random_discovery_enabled = config_->get_bool(
                "crawler.random_discovery",
                config_->get_string("dht.bootstrap_infohash", "").empty());
            if (random_discovery_enabled && (rand() % 5 == 0)) {
              // 20% chance
              // Generate random infohash
              std::random_device rd;
              std::mt19937 gen(rd());
              std::uniform_int_distribution<uint8_t> dist(0, 255);

              std::vector<uint8_t> hash_bytes(20);
              for (auto &byte : hash_bytes) {
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
        } catch (const std::exception &e) {
          beacon_->error("Error in crawling background thread: " +
                             std::string(e.what()),
                         types::BeaconCategory::GENERAL);
        }
      });

      is_crawling_ = true;
      beacon_->info("Crawling started successfully");
      return true;
    } catch (const std::exception &e) {
      beacon_->error("Failed to start crawling: {}",
                     types::BeaconCategory::GENERAL, e.what());
      return false;
    }
  }

  std::future<bool> start_crawling_async() {
    return std::async(std::launch::async,
                      [this]() { return start_crawling(); });
  }

  bool stop_crawling() {
    auto lock_manager = lock::LockManagerSingleton::instance();
    auto lock_guard =
        lock_manager->get_lock_guard(controller_state_resource_id_);
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

      // Join background threads if they are joinable
      if (dht_crawling_thread_.joinable()) {
        beacon_->info("Waiting for DHT crawling thread to finish",
                      types::BeaconCategory::DHT);
        dht_crawling_thread_.join();
      }
      if (infohash_check_thread_.joinable()) {
        beacon_->info("Waiting for infohash check thread to finish",
                      types::BeaconCategory::GENERAL);
        infohash_check_thread_.join();
      }

      // Stop DHT crawling if the DHT component is running
      if (dht_session_ && dht_session_->is_running()) {
        beacon_->info("Stopping DHT crawling", types::BeaconCategory::DHT);
        // No specific action needed here, just logging
      }
      // Stop all active peer managers to stop metadata downloading
      beacon_->info("Stopping " + std::to_string(peer_managers_.size()) +
                        " active peer managers",
                    types::BeaconCategory::BITTORRENT);

      {
        std::lock_guard<std::mutex> lock(maps_mutex_);
        for (const auto &[info_hash, peer_manager] : peer_managers_) {
          try {
            peer_manager->stop();
          } catch (const std::exception &e) {
            beacon_->warning("Failed to stop peer manager for infohash " +
                                 info_hash + ": " + e.what(),
                             types::BeaconCategory::BITTORRENT);
          }
        }
        peer_managers_.clear();
        metadata_exchanges_.clear();
      }

      // Cancel any pending tracker announcements
      beacon_->info("Stopping " + std::to_string(tracker_managers_.size()) +
                        " active tracker managers",
                    types::BeaconCategory::TRACKER);

      {
        std::lock_guard<std::mutex> lock(maps_mutex_);
        for (const auto &[info_hash, tracker_manager] : tracker_managers_) {
          try {
            // Send a stopped announce to all trackers
            std::string peer_id_str(20, '0');
            uint16_t port =
                static_cast<uint16_t>(config_->get_int("bittorrent.port", 6881));

            tracker_manager->announce_async(peer_id_str, port,
                                            0, // uploaded
                                            0, // downloaded
                                            0, // left
                                            "stopped");
          } catch (const std::exception &e) {
            beacon_->warning(
                "Failed to cancel tracker announcements for infohash " +
                    info_hash + ": " + e.what(),
                types::BeaconCategory::TRACKER);
          }
        }
        tracker_managers_.clear();
      }

      beacon_->info("Crawling stopped successfully");
      return true;
    } catch (const std::exception &e) {
      beacon_->error("Failed to stop crawling: {}",
                     types::BeaconCategory::GENERAL, e.what());
      return false;
    }
  }

  std::future<bool> stop_crawling_async() {
    return std::async(std::launch::async, [this]() { return stop_crawling(); });
  }

  std::unordered_map<std::string, std::string> get_statistics() const {
    std::unordered_map<std::string, std::string> stats;

    // Get storage manager statistics
    auto storage_stats = storage_manager_->get_statistics();
    stats.insert(storage_stats.begin(), storage_stats.end());

    // Add controller statistics
    auto lock_manager = lock::LockManagerSingleton::instance();
    auto lock_guard = lock_manager->get_lock_guard(
        controller_state_resource_id_, lock::LockManager::LockType::SHARED);
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
      stats["dht.node_count"] =
          std::to_string(dht_session_->routing_table().size());
    } else {
      stats["dht.running"] = "false";
      stats["dht.node_count"] = "0";
    }

    // Add BitTorrent statistics
    {
      std::lock_guard<std::mutex> lock(maps_mutex_);
      stats["bittorrent.peer_manager_count"] =
          std::to_string(peer_managers_.size());
      stats["bittorrent.metadata_exchange_count"] =
          std::to_string(metadata_exchanges_.size());

      // Count total connected peers across all peer managers
      size_t total_connected_peers = 0;
      for (const auto &[info_hash, peer_manager] : peer_managers_) {
        total_connected_peers += peer_manager->connected_peers().size();
      }
      stats["bittorrent.connected_peer_count"] =
          std::to_string(total_connected_peers);
    }

    // Add Tracker statistics
    {
      std::lock_guard<std::mutex> lock(maps_mutex_);
      stats["tracker.manager_count"] = std::to_string(tracker_managers_.size());

      // Count total trackers across all tracker managers
      size_t total_trackers = 0;
      for (const auto &[info_hash, tracker_manager] : tracker_managers_) {
        total_trackers += tracker_manager->tracker_urls().size();
      }
      stats["tracker.url_count"] = std::to_string(total_trackers);
    }

    return stats;
  }

  std::vector<Controller::SanityCheckResult> run_sanity_checks() {
    std::vector<Controller::SanityCheckResult> checks;
    checks.reserve(8);

    auto add = [&](std::string module, bool ok, std::string message) {
      checks.push_back({std::move(module), ok, std::move(message)});
    };

    auto lock_manager = lock::LockManagerSingleton::instance();
    auto lock_guard = lock_manager->get_lock_guard(
        controller_state_resource_id_, lock::LockManager::LockType::SHARED);

    try {
      if (!config_) {
        add("configuration", false, "Configuration not initialized");
      } else {
        std::ostringstream msg;
        const auto path = config_->get_config_path();
        const bool exists = !path.empty() && std::filesystem::exists(path);
        msg << "path=" << (path.empty() ? "<unset>" : path);

        if (!exists) {
          add("configuration", false, msg.str() + " (missing)");
        } else {
          msg << " db.path="
              << config_->get_string("database.path", "data/bitscrape.db");
          add("configuration", true, msg.str());
        }
      }
    } catch (const std::exception &e) {
      add("configuration", false, e.what());
    }

    try {
      if (!storage_manager_) {
        add("storage", false, "Storage manager not created");
      } else {
        auto db = storage_manager_->database();
        if (!db) {
          add("storage", false, "Database handle unavailable");
        } else if (!db->is_initialized()) {
          std::ostringstream msg;
          msg << "db=" << db->path() << " not initialized";
          add("storage", false, msg.str());
        } else {
          const auto result = db->execute("SELECT 1");
          const bool has_rows = result.has_rows();
          std::ostringstream msg;
          msg << "db=" << db->path()
              << " select1=" << (has_rows ? "ok" : "empty");
          add("storage", has_rows, msg.str());
        }
      }
    } catch (const std::exception &e) {
      add("storage", false, e.what());
    }

    try {
      const bool bus_ok = event_bus_ != nullptr;
      const bool processor_ok =
          event_processor_ && event_processor_->is_running();
      std::ostringstream msg;
      msg << "bus=" << (bus_ok ? "ok" : "null")
          << " processor=" << (processor_ok ? "running" : "stopped");
      add("event", bus_ok && processor_ok, msg.str());
    } catch (const std::exception &e) {
      add("event", false, e.what());
    }

    try {
      if (!bt_event_processor_) {
        add("bittorrent", false, "Event processor not created");
      } else {
        const bool running = bt_event_processor_->is_running();
        std::ostringstream msg;
        msg << "running=" << (running ? "yes" : "no")
            << " peer_managers=" << peer_managers_.size();
        add("bittorrent", running, msg.str());
      }
    } catch (const std::exception &e) {
      add("bittorrent", false, e.what());
    }

    try {
      std::ostringstream msg;
      msg << "managers=" << tracker_managers_.size();
      add("tracker", true, msg.str());
    } catch (const std::exception &e) {
      add("tracker", false, e.what());
    }

    try {
      if (!dht_session_) {
        add("dht", false, "DHT session not created");
      } else {
        const bool running = dht_session_->is_running();
        std::ostringstream msg;
        msg << "running=" << (running ? "yes" : "no")
            << " nodes=" << dht_session_->routing_table().size();
        add("dht", running, msg.str());
      }
    } catch (const std::exception &e) {
      add("dht", false, e.what());
    }

    return checks;
  }

  std::vector<types::InfoHash> get_infohashes(size_t limit,
                                              size_t offset) const {
    auto query = storage_manager_->query_interface();
    storage::QueryInterface::InfoHashQueryOptions options;
    options.limit = limit;
    options.offset = offset;
    auto infohashes = query->get_infohashes(options);

    std::vector<types::InfoHash> result;
    for (const auto &model : infohashes) {
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
    for (const auto &model : nodes) {
      result.push_back(model.node_id);
    }

    return result;
  }

  void handle_dht_node_discovered(const types::Event &event) {
    (void)event;
    try {
      // For DHT_NODE_FOUND events, we expect the event to contain node
      // information in its data field, which we need to extract
      beacon_->debug("DHT node discovered event received",
                     types::BeaconCategory::DHT);

      // In a real implementation, we would extract the node information from
      // the event and store it in the database. For now, we'll just log a
      // message.
      beacon_->info("Received DHT node discovery event",
                    types::BeaconCategory::DHT);

      // Generate a random node ID and endpoint for testing
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<uint8_t> dist(0, 255);

      std::vector<uint8_t> id_bytes(20);
      for (auto &byte : id_bytes) {
        byte = dist(gen);
      }

      types::NodeID node_id(id_bytes);
      types::Endpoint endpoint(std::string("192.168.1.1"), 6881);

      // Store the node in the database
      storage_manager_->store_node(node_id, endpoint, true);

      beacon_->info("Stored DHT node: " + node_id.to_hex().substr(0, 8) +
                        "... at " + endpoint.address() + ":" +
                        std::to_string(endpoint.port()),
                    types::BeaconCategory::DHT);
    } catch (const std::exception &e) {
      beacon_->error("Failed to handle DHT node discovery: " +
                         std::string(e.what()),
                     types::BeaconCategory::DHT);
    }
  }

  void handle_dht_infohash_discovered(const types::Event &event) {
    (void)event;
    try {
      // For DHT_INFOHASH_FOUND events, we expect the event to contain infohash
      // information in its data field, which we need to extract
      beacon_->debug("DHT infohash found event received",
                     types::BeaconCategory::DHT);

      const types::InfoHash* info_hash_ptr = nullptr;

      // Try to cast the event to a more specific type
      if (auto *dht_event =
              dynamic_cast<const dht::DHTInfoHashFoundEvent *>(&event)) {
        info_hash_ptr = &dht_event->info_hash();
      }

      if (!info_hash_ptr) {
        beacon_->error("Received DHT_INFOHASH_FOUND event with unknown format",
                       types::BeaconCategory::DHT);
        return;
      }

      const types::InfoHash &info_hash = *info_hash_ptr;

      // Store the infohash in the database
      storage_manager_->store_infohash(info_hash);

      beacon_->info("Stored discovered DHT infohash: " + info_hash.to_hex().substr(0, 16) +
                        "...",
                    types::BeaconCategory::DHT);

      // If crawling is active, create a PeerManager and MetadataExchange for
      // this infohash
      if (is_crawling_ && is_running_) {
        create_peer_manager_for_infohash(info_hash);
      }
    } catch (const std::exception &e) {
      beacon_->error("Failed to handle DHT infohash discovery: " +
                         std::string(e.what()),
                     types::BeaconCategory::DHT);
    }
  }

  void create_peer_manager_for_infohash(const types::InfoHash &info_hash) {
    try {
      // Check if we already have a peer manager for this infohash
      std::string info_hash_str = info_hash.to_hex();
      {
        std::lock_guard<std::mutex> lock(maps_mutex_);
        if (peer_managers_.find(info_hash_str) != peer_managers_.end()) {
          return; // Already have a peer manager for this infohash
        }
      }

      // Generate a random peer ID
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<uint8_t> dist(0, 255);

      std::vector<uint8_t> peer_id(20);
      for (auto &byte : peer_id) {
        byte = dist(gen);
      }

      // Create a peer manager for this infohash
      int max_connections = config_->get_int("bittorrent.max_connections", 50);
      auto peer_manager = std::make_shared<bittorrent::PeerManager>(
          info_hash, peer_id, max_connections);

      // Start the peer manager
      if (peer_manager->start()) {
        // Add the peer manager to our map
        {
          std::lock_guard<std::mutex> lock(maps_mutex_);
          peer_managers_[info_hash_str] = peer_manager;
        }

        // Create a metadata exchange for this infohash
        auto metadata_exchange = std::make_shared<bittorrent::MetadataExchange>(
            peer_manager->protocol());
        metadata_exchange->initialize();

        // Add the metadata exchange to our map
        {
          std::lock_guard<std::mutex> lock(maps_mutex_);
          metadata_exchanges_[info_hash_str] = metadata_exchange;
        }

        // Add the peer manager and metadata exchange to the BitTorrent event
        // processor
        if (bt_event_processor_) {
          bt_event_processor_->add_peer_manager(info_hash, peer_manager);
          bt_event_processor_->add_metadata_exchange(info_hash,
                                                     metadata_exchange);
        }

        // Create a tracker manager for this infohash
        auto tracker_manager =
            std::make_shared<tracker::TrackerManager>(info_hash);

        // Set timeouts from configuration
        int connection_timeout =
            config_->get_int("bittorrent.connection_timeout", 10) *
            1000; // Convert to ms
        int request_timeout =
            config_->get_int("bittorrent.download_timeout", 30) * 1000;
        // Convert to ms
        tracker_manager->set_connection_timeout(connection_timeout);
        tracker_manager->set_request_timeout(request_timeout);

        // Add the tracker manager to our map
        {
          std::lock_guard<std::mutex> lock(maps_mutex_);
          tracker_managers_[info_hash_str] = tracker_manager;
        }

        beacon_->info("Created peer manager, metadata exchange, and tracker "
                      "manager for infohash: " +
                          info_hash.to_hex().substr(0, 16) + "...",
                      types::BeaconCategory::BITTORRENT);

        // Add some tracker URLs from configuration
        std::string tracker_urls = config_->get_string(
            "tracker.urls", "udp://tracker.opentrackr.org:1337,udp://"
                            "tracker.openbittorrent.com:6969,udp://"
                            "tracker.coppersurfer.tk:6969");

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
        uint16_t port =
            static_cast<uint16_t>(config_->get_int("bittorrent.port", 6881));

        auto future = tracker_manager->announce_async(peer_id_str, port,
                                        0, // uploaded
                                        0, // downloaded
                                        0, // left
                                        "started");

        // Process tracker responses asynchronously
        std::thread([this, future = std::move(future), info_hash]() mutable {
          try {
            auto responses = future.get();
            for (const auto &[url, response] : responses) {
              if (!response.has_error()) {
                for (const auto &peer : response.peers()) {
                  // Emit PeerDiscoveredEvent
                  event_bus_->publish(
                      bittorrent::PeerDiscoveredEvent(info_hash, peer));
                }
              }
            }
          } catch (...) {
            // Ignore errors in background processing
          }
        }).detach();
      } else {
        beacon_->error("Failed to start peer manager for infohash: " +
                           info_hash.to_hex(),
                       types::BeaconCategory::BITTORRENT);
      }
    } catch (const std::exception &e) {
      beacon_->error("Failed to create peer manager for infohash: " +
                         info_hash.to_hex() + ": " + std::string(e.what()),
                     types::BeaconCategory::BITTORRENT);
    }
  }

  void handle_metadata_downloaded(const types::Event &event) {
    try {
      // For BT_METADATA_RECEIVED events, we expect the event to contain
      // metadata information in its data field, which we need to extract
      beacon_->debug("BitTorrent metadata received event received",
                     types::BeaconCategory::BITTORRENT);

      // Try to cast the event to a more specific type if available
      if (auto *bt_event =
              dynamic_cast<const bittorrent::MetadataReceivedEvent *>(&event)) {
        // Extract metadata information from the event
        const types::InfoHash &info_hash = bt_event->info_hash();
        const types::MetadataInfo &metadata = bt_event->metadata();

        // Store the metadata in the database
        if (storage_manager_->store_metadata(info_hash, metadata)) {
          beacon_->info("Stored metadata for infohash: " +
                            info_hash.to_hex().substr(0, 16) + "...",
                        types::BeaconCategory::BITTORRENT);

          // Extract and store file information if available
          const auto &files = metadata.files();
          if (!files.empty()) {
            beacon_->info("Storing " + std::to_string(files.size()) +
                              " files for infohash: " +
                              info_hash.to_hex().substr(0, 16) + "...",
                          types::BeaconCategory::BITTORRENT);

            // Log file details for debugging
            for (const auto &file : files) {
              beacon_->debug("File: " + file.first + ", Size: " +
                                 std::to_string(file.second) + " bytes",
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

          beacon_->info("Stored torrent info for infohash: " +
                            info_hash.to_hex().substr(0, 16) + "...",
                        types::BeaconCategory::BITTORRENT);

          // If we have tracker URLs in the torrent_info, add them to the
          // tracker manager
          std::string info_hash_str = info_hash.to_hex();
          std::shared_ptr<tracker::TrackerManager> tracker_manager;
          
          {
            std::lock_guard<std::mutex> lock(maps_mutex_);
            if (tracker_managers_.find(info_hash_str) !=
                tracker_managers_.end()) {
              tracker_manager = tracker_managers_[info_hash_str];
            }
          }

          if (tracker_manager) {
            // Add the main announce URL if available
            if (!torrent_info.announce().empty()) {
              tracker_manager->add_tracker(torrent_info.announce());
              beacon_->debug("Added tracker URL from torrent info: " +
                                 torrent_info.announce(),
                             types::BeaconCategory::TRACKER);
            }

            // Add announce list URLs if available
            for (const auto &announce_url : torrent_info.announce_list()) {
              tracker_manager->add_tracker(announce_url);
              beacon_->debug(
                  "Added tracker URL from torrent info announce list: " +
                      announce_url,
                  types::BeaconCategory::TRACKER);
            }

            // Re-announce to trackers with the updated metadata
            if (!torrent_info.announce().empty() ||
                !torrent_info.announce_list().empty()) {
              // Generate a random peer ID
              std::random_device rd;
              std::mt19937 gen(rd());
              std::uniform_int_distribution<uint8_t> dist(0, 255);

              std::vector<uint8_t> peer_id(20);
              for (auto &byte : peer_id) {
                byte = dist(gen);
              }

              std::string peer_id_str(peer_id.begin(), peer_id.end());
              uint16_t port = static_cast<uint16_t>(
                  config_->get_int("bittorrent.port", 6881));

              tracker_manager->announce_async(
                  peer_id_str, port,
                  0,                     // uploaded
                  0,                     // downloaded
                  metadata.total_size(), // left (total size)
                  "started");

              beacon_->info("Re-announced to trackers with updated metadata",
                            types::BeaconCategory::TRACKER);
            }
          }
        } else {
          beacon_->error("Failed to store metadata for infohash: " +
                             info_hash.to_hex(),
                         types::BeaconCategory::BITTORRENT);
        }
      } else {
        // If we can't cast to a specific event type, log a warning
        beacon_->warning(
            "Received BT_METADATA_RECEIVED event with unknown format",
            types::BeaconCategory::BITTORRENT);
      }
    } catch (const std::exception &e) {
      beacon_->error("Failed to handle metadata download: " +
                         std::string(e.what()),
                     types::BeaconCategory::BITTORRENT);
    }
  }

  void handle_error(const types::Event &event) {
    try {
      // For SYSTEM_ERROR events, we expect the event to contain error
      // information
      beacon_->debug("System error event received",
                     types::BeaconCategory::GENERAL);

      // Try to cast the event to a more specific type if available
      if (auto *beacon_event =
              dynamic_cast<const types::BeaconEvent *>(&event)) {
        // Extract error information from the beacon event
        const std::string &message = beacon_event->message();
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
        const auto &location = beacon_event->location();
        beacon_->debug("Source: " + std::string(location.file_name()) + ":" +
                           std::to_string(location.line()) + " in " +
                           std::string(location.function_name()),
                       category);
      } else {
        // For other error events, just log the event type
        beacon_->error("Error event received: " + event.to_string(),
                       types::BeaconCategory::GENERAL);
      }
    } catch (const std::exception &e) {
      beacon_->error("Failed to handle error event: " + std::string(e.what()),
                     types::BeaconCategory::GENERAL);
    }
  }

  void perform_random_node_lookups() {
    if (!dht_session_ || !dht_session_->is_running()) {
      return;
    }

    try {
      // Number of random lookups to perform
      const size_t NUM_LOOKUPS = 5;

      beacon_->info("Performing " + std::to_string(NUM_LOOKUPS) +
                        " random node lookups",
                    types::BeaconCategory::DHT);

      // Perform multiple random lookups to discover new nodes
      for (size_t i = 0; i < NUM_LOOKUPS; ++i) {
        // Generate a random node ID to look up
        types::NodeID random_id = types::NodeID::random();

        // Perform the lookup
        beacon_->debug("Looking up random node ID: " +
                           random_id.to_hex().substr(0, 16) + "...",
                       types::BeaconCategory::DHT);

        // Use async version to avoid blocking
        auto future = dht_session_->find_nodes_async(random_id);

        // Wait for the result
        auto nodes = future.get();

        // Store the discovered nodes in the database
        for (const auto &node : nodes) {
          storage_manager_->store_node(node.id(), node.endpoint(), true, node.last_rtt_ms());

          // Publish a DHT_NODE_FOUND event
          // Create a custom event derived from Event for DHT_NODE_FOUND
          // In a real implementation, we would create a proper event class
          // For now, we'll just log the node discovery
          beacon_->info("Discovered node: " + node.id().to_hex().substr(0, 16) +
                            "...",
                        types::BeaconCategory::DHT);
        }

        beacon_->info("Found " + std::to_string(nodes.size()) +
                          " nodes in random lookup",
                      types::BeaconCategory::DHT);

        // Sleep briefly between lookups to avoid overwhelming the network
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    } catch (const std::exception &e) {
      beacon_->error("Error performing random node lookups: " +
                         std::string(e.what()),
                     types::BeaconCategory::DHT);
    }
  }

  // Member variables
  std::shared_ptr<Configuration> config_;
  std::shared_ptr<storage::StorageManager> storage_manager_;
  std::unique_ptr<event::EventBus> event_bus_;
  std::unique_ptr<event::EventProcessor> event_processor_;
  std::shared_ptr<beacon::Beacon> beacon_;
  bool is_running_;
  bool is_crawling_;
  std::atomic<bool> stop_discovery_threads_;
  std::thread peer_refresh_thread_;
  mutable std::mutex maps_mutex_;
  std::vector<std::future<void>> background_futures_;
  uint64_t
      controller_state_resource_id_; // Resource ID for the controller state

  // BitTorrent components
  std::unique_ptr<bittorrent::BitTorrentEventProcessor> bt_event_processor_;
  std::unordered_map<std::string, std::shared_ptr<bittorrent::PeerManager>>
      peer_managers_;
  std::unordered_map<std::string, std::shared_ptr<bittorrent::MetadataExchange>>
      metadata_exchanges_;

  // Tracker component
  std::unordered_map<std::string, std::shared_ptr<tracker::TrackerManager>>
      tracker_managers_;

  // DHT component
  std::unique_ptr<dht::DHTSession> dht_session_;

  // Background crawling threads (managed for proper shutdown)
  std::thread dht_crawling_thread_;
  std::thread infohash_check_thread_;
};

// Controller implementation

Controller::Controller(const std::string &config_path)
    : impl_(std::make_unique<Impl>(config_path)) {}

Controller::~Controller() {}

bool Controller::initialize() { return impl_->initialize(); }

std::future<bool> Controller::initialize_async() {
  return impl_->initialize_async();
}

bool Controller::start() { return impl_->start(); }

std::future<bool> Controller::start_async() { return impl_->start_async(); }

bool Controller::stop() { return impl_->stop(); }

std::future<bool> Controller::stop_async() { return impl_->stop_async(); }

std::shared_ptr<Configuration> Controller::get_configuration() const {
  return impl_->config_;
}

storage::StorageManager &Controller::get_storage_manager() const {
  return *impl_->storage_manager_;
}

std::shared_ptr<event::EventBus> Controller::get_event_bus() const {
  // Create an aliasing shared_ptr that keeps a no-op owning reference to the
  // internal Impl object (so the returned shared_ptr remains valid while the
  // Controller is alive) and aliases to the internal EventBus pointer.
  return std::shared_ptr<event::EventBus>(
      std::shared_ptr<Impl>(impl_.get(), [](Impl *) {}),
      impl_->event_bus_.get());
}

std::shared_ptr<beacon::Beacon> Controller::get_beacon() const {
  return impl_->beacon_;
}

bool Controller::start_crawling() { return impl_->start_crawling(); }

std::future<bool> Controller::start_crawling_async() {
  return impl_->start_crawling_async();
}

bool Controller::stop_crawling() { return impl_->stop_crawling(); }

std::future<bool> Controller::stop_crawling_async() {
  return impl_->stop_crawling_async();
}

std::unordered_map<std::string, std::string>
Controller::get_statistics() const {
  return impl_->get_statistics();
}

std::vector<types::InfoHash> Controller::get_infohashes(size_t limit,
                                                        size_t offset) const {
  return impl_->get_infohashes(limit, offset);
}

std::vector<types::NodeID> Controller::get_nodes(size_t limit,
                                                 size_t offset) const {
  return impl_->get_nodes(limit, offset);
}

void Controller::receive_event(const types::Event &event) {
  // Mirror the dispatch logic used in the internal subscription so tests can
  // deliver events directly to the controller.
  if (event.type() == types::Event::Type::DHT_NODE_FOUND) {
    impl_->handle_dht_node_discovered(event);
  } else if (event.type() == types::Event::Type::DHT_INFOHASH_FOUND) {
    impl_->handle_dht_infohash_discovered(event);
  } else if (event.type() == types::Event::Type::BT_METADATA_RECEIVED ||
             dynamic_cast<const bittorrent::MetadataReceivedEvent *>(&event) !=
                 nullptr) {
    impl_->handle_metadata_downloaded(event);
  } else if (event.type() == types::Event::Type::SYSTEM_ERROR) {
    impl_->handle_error(event);
  } else {
    // Default to the generic handlers when unsure
    try {
      impl_->handle_error(event);
    } catch (...) {
      // swallow for test convenience
    }
  }
}

void Controller::handle_dht_node_discovered(const types::Event &event) {
  impl_->handle_dht_node_discovered(event);
}

void Controller::handle_dht_infohash_discovered(const types::Event &event) {
  impl_->handle_dht_infohash_discovered(event);
}

void Controller::handle_metadata_downloaded(const types::Event &event) {
  impl_->handle_metadata_downloaded(event);
}

void Controller::handle_error(const types::Event &event) {
  impl_->handle_error(event);
}

std::vector<Controller::SanityCheckResult> Controller::run_sanity_checks() {
  return impl_->run_sanity_checks();
}

} // namespace bitscrape::core
