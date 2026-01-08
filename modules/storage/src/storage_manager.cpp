#include <bitscrape/storage/storage_manager.hpp>
#include <bitscrape/storage/database.hpp>
#include <bitscrape/storage/query_interface.hpp>

#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace bitscrape::storage {

class StorageManager::Impl {
public:
    friend class StorageManager;
    Impl(const std::string& db_path, bool persistent)
        : db_path_(db_path.empty() ? "data/default.db" : db_path),
                    database_(std::make_shared<Database>(
                            db_path.empty() ? "data/default.db" : db_path, persistent)),
          query_interface_(std::make_shared<QueryInterface>(database_)),
          initialized_(false) {
        // If original path was empty, we're using the default path
        if (db_path.empty()) {
            std::cerr << "Using default database path: " << db_path_ << std::endl;
        }
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (initialized_) {
            return true;
        }

        try {
            // Initialize database
            if (!database_->initialize()) {
                std::cerr << "Failed to initialize database" << std::endl;
                return false;
            }



            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize storage manager: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> initialize_async() {
        return std::async(std::launch::async, [this]() {
            return initialize();
        });
    }

    bool close() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            return true;
        }

        try {
            bool success = true;

            // Close database
            if (!database_->close()) {
                std::cerr << "Failed to close database" << std::endl;
                success = false;
                // Continue with cleanup even if close fails
            }

            initialized_ = false;
            return success;
        } catch (const std::exception& e) {
            std::cerr << "Failed to close storage manager: " << e.what() << std::endl;

            // Try to clean up anyway
            try {
                initialized_ = false;
            } catch (...) {
                // Ignore any exceptions during cleanup
            }

            return false;
        }
    }

    std::future<bool> close_async() {
        return std::async(std::launch::async, [this]() {
            return close();
        });
    }

    bool store_node(const types::NodeID& node_id, const types::Endpoint& endpoint, bool is_responsive) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Check if node exists
            auto existing_node = query_interface_->get_node(node_id);

            if (existing_node) {
                // Update existing node
                auto now = std::chrono::system_clock::now();

                return database_->execute_update(
                    "UPDATE nodes SET last_seen = ?, is_responsive = ? WHERE node_id = ?",
                    {
                        time_point_to_string(now),
                        is_responsive ? "1" : "0",
                        node_id.to_hex()
                    }
                );
            } else {
                // Insert new node
                auto now = std::chrono::system_clock::now();

                return database_->execute_update(
                    "INSERT INTO nodes (node_id, ip, port, first_seen, last_seen, is_responsive) VALUES (?, ?, ?, ?, ?, ?)",
                    {
                        node_id.to_hex(),
                        endpoint.address(),
                        std::to_string(endpoint.port()),
                        time_point_to_string(now),
                        time_point_to_string(now),
                        is_responsive ? "1" : "0"
                    }
                );
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to store node: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> store_node_async(const types::NodeID& node_id, const types::Endpoint& endpoint, bool is_responsive) {
        return std::async(std::launch::async, [this, node_id, endpoint, is_responsive]() {
            return store_node(node_id, endpoint, is_responsive);
        });
    }

    bool update_node_responsiveness(const types::NodeID& node_id, bool is_responsive) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            auto now = std::chrono::system_clock::now();

            return database_->execute_update(
                "UPDATE nodes SET last_seen = ?, is_responsive = ? WHERE node_id = ?",
                {
                    time_point_to_string(now),
                    is_responsive ? "1" : "0",
                    node_id.to_hex()
                }
            );
        } catch (const std::exception& e) {
            std::cerr << "Failed to update node responsiveness: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> update_node_responsiveness_async(const types::NodeID& node_id, bool is_responsive) {
        return std::async(std::launch::async, [this, node_id, is_responsive]() {
            return update_node_responsiveness(node_id, is_responsive);
        });
    }

    bool increment_node_ping_count(const types::NodeID& node_id) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            auto now = std::chrono::system_clock::now();

            return database_->execute_update(
                "UPDATE nodes SET last_seen = ?, ping_count = ping_count + 1 WHERE node_id = ?",
                {
                    time_point_to_string(now),
                    node_id.to_hex()
                }
            );
        } catch (const std::exception& e) {
            std::cerr << "Failed to increment node ping count: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> increment_node_ping_count_async(const types::NodeID& node_id) {
        return std::async(std::launch::async, [this, node_id]() {
            return increment_node_ping_count(node_id);
        });
    }

    bool increment_node_query_count(const types::NodeID& node_id) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            auto now = std::chrono::system_clock::now();

            return database_->execute_update(
                "UPDATE nodes SET last_seen = ?, query_count = query_count + 1 WHERE node_id = ?",
                {
                    time_point_to_string(now),
                    node_id.to_hex()
                }
            );
        } catch (const std::exception& e) {
            std::cerr << "Failed to increment node query count: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> increment_node_query_count_async(const types::NodeID& node_id) {
        return std::async(std::launch::async, [this, node_id]() {
            return increment_node_query_count(node_id);
        });
    }

    bool increment_node_response_count(const types::NodeID& node_id) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            auto now = std::chrono::system_clock::now();

            return database_->execute_update(
                "UPDATE nodes SET last_seen = ?, response_count = response_count + 1 WHERE node_id = ?",
                {
                    time_point_to_string(now),
                    node_id.to_hex()
                }
            );
        } catch (const std::exception& e) {
            std::cerr << "Failed to increment node response count: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> increment_node_response_count_async(const types::NodeID& node_id) {
        return std::async(std::launch::async, [this, node_id]() {
            return increment_node_response_count(node_id);
        });
    }

    bool store_infohash(const types::InfoHash& info_hash) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Check if infohash exists
            auto existing_infohash = query_interface_->get_infohash(info_hash);

            if (existing_infohash) {
                // Update existing infohash
                auto now = std::chrono::system_clock::now();

                return database_->execute_update(
                    "UPDATE infohashes SET last_seen = ? WHERE info_hash = ?",
                    {
                        time_point_to_string(now),
                        info_hash.to_hex()
                    }
                );
            } else {
                // Insert new infohash
                auto now = std::chrono::system_clock::now();

                return database_->execute_update(
                    "INSERT INTO infohashes (info_hash, first_seen, last_seen) VALUES (?, ?, ?)",
                    {
                        info_hash.to_hex(),
                        time_point_to_string(now),
                        time_point_to_string(now)
                    }
                );
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to store infohash: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> store_infohash_async(const types::InfoHash& info_hash) {
        return std::async(std::launch::async, [this, info_hash]() {
            return store_infohash(info_hash);
        });
    }

    bool increment_infohash_announce_count(const types::InfoHash& info_hash) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Ensure infohash exists
            if (!store_infohash(info_hash)) {
                return false;
            }

            auto now = std::chrono::system_clock::now();

            return database_->execute_update(
                "UPDATE infohashes SET last_seen = ?, announce_count = announce_count + 1 WHERE info_hash = ?",
                {
                    time_point_to_string(now),
                    info_hash.to_hex()
                }
            );
        } catch (const std::exception& e) {
            std::cerr << "Failed to increment infohash announce count: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> increment_infohash_announce_count_async(const types::InfoHash& info_hash) {
        return std::async(std::launch::async, [this, info_hash]() {
            return increment_infohash_announce_count(info_hash);
        });
    }

    bool increment_infohash_peer_count(const types::InfoHash& info_hash) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Ensure infohash exists
            if (!store_infohash(info_hash)) {
                return false;
            }

            auto now = std::chrono::system_clock::now();

            return database_->execute_update(
                "UPDATE infohashes SET last_seen = ?, peer_count = peer_count + 1 WHERE info_hash = ?",
                {
                    time_point_to_string(now),
                    info_hash.to_hex()
                }
            );
        } catch (const std::exception& e) {
            std::cerr << "Failed to increment infohash peer count: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> increment_infohash_peer_count_async(const types::InfoHash& info_hash) {
        return std::async(std::launch::async, [this, info_hash]() {
            return increment_infohash_peer_count(info_hash);
        });
    }

    bool store_metadata(const types::InfoHash& info_hash, const types::MetadataInfo& metadata) {
        std::lock_guard<std::mutex> lock(mutex_);

        (void)metadata;

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Ensure infohash exists
            if (!store_infohash(info_hash)) {
                return false;
            }

            // Update infohash to indicate metadata is available
            if (!database_->execute_update(
                "UPDATE infohashes SET has_metadata = 1 WHERE info_hash = ?",
                {info_hash.to_hex()}
            )) {
                return false;
            }

            // Check if metadata exists
            auto existing_metadata = query_interface_->get_metadata(info_hash);

            if (existing_metadata) {
                // Update existing metadata
                auto now = std::chrono::system_clock::now();

                // In a real implementation, we would serialize the metadata to a blob
                // For now, we'll just update the download time
                return database_->execute_update(
                    "UPDATE metadata SET download_time = ? WHERE info_hash = ?",
                    {
                        time_point_to_string(now),
                        info_hash.to_hex()
                    }
                );
            } else {
                // Insert new metadata
                auto now = std::chrono::system_clock::now();

                // In a real implementation, we would extract more information from the metadata
                // For now, we'll just insert placeholder values
                return database_->execute_update(
                    "INSERT INTO metadata (info_hash, download_time, name, total_size, piece_count, file_count, raw_metadata) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?)",
                    {
                        info_hash.to_hex(),
                        time_point_to_string(now),
                        "Unknown",  // name
                        "0",         // total_size
                        "0",         // piece_count
                        "0",         // file_count
                        ""           // raw_metadata
                    }
                );
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to store metadata: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> store_metadata_async(const types::InfoHash& info_hash, const types::MetadataInfo& metadata) {
        return std::async(std::launch::async, [this, info_hash, metadata]() {
            return store_metadata(info_hash, metadata);
        });
    }

    bool store_torrent(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Begin transaction
            if (!database_->begin_transaction()) {
                return false;
            }

            // Store metadata
            if (!store_metadata(info_hash, torrent_info.metadata())) {
                database_->rollback_transaction();
                return false;
            }

            // Store files
            for (const auto& file : torrent_info.metadata().files()) {
                if (!database_->execute_update(
                    "INSERT INTO files (info_hash, path, size) VALUES (?, ?, ?)",
                    {
                        info_hash.to_hex(),
                        file.first,  // path
                        std::to_string(file.second)  // size
                    }
                )) {
                    database_->rollback_transaction();
                    return false;
                }
            }

            // Store trackers
            // First, add the main announce URL if it exists
            if (!torrent_info.announce().empty()) {
                if (!database_->execute_update(
                    "INSERT INTO trackers (info_hash, url) VALUES (?, ?)",
                    {
                        info_hash.to_hex(),
                        torrent_info.announce()
                    }
                )) {
                    database_->rollback_transaction();
                    return false;
                }
            }

            // Then add all trackers from the announce list
            for (const auto& tracker : torrent_info.announce_list()) {
                if (!database_->execute_update(
                    "INSERT INTO trackers (info_hash, url) VALUES (?, ?)",
                    {
                        info_hash.to_hex(),
                        tracker
                    }
                )) {
                    database_->rollback_transaction();
                    return false;
                }
            }

            // Commit transaction
            return database_->commit_transaction();
        } catch (const std::exception& e) {
            std::cerr << "Failed to store torrent: " << e.what() << std::endl;
            database_->rollback_transaction();
            return false;
        }
    }

    std::future<bool> store_torrent_async(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info) {
        return std::async(std::launch::async, [this, info_hash, torrent_info]() {
            return store_torrent(info_hash, torrent_info);
        });
    }

    bool store_peer(const types::InfoHash& info_hash, const types::Endpoint& endpoint,
                   const std::optional<types::NodeID>& peer_id,
                   bool supports_dht,
                   bool supports_extension_protocol,
                   bool supports_fast_protocol) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Ensure infohash exists
            if (!store_infohash(info_hash)) {
                return false;
            }

            // Check if peer exists
            auto result = database_->execute(
                "SELECT id FROM peers WHERE info_hash = ? AND ip = ? AND port = ?",
                {
                    info_hash.to_hex(),
                    endpoint.address(),
                    std::to_string(endpoint.port())
                }
            );

            if (result.next()) {
                // Update existing peer
                auto now = std::chrono::system_clock::now();

                std::string peer_id_hex = peer_id ? peer_id->to_hex() : "";

                return database_->execute_update(
                    "UPDATE peers SET last_seen = ?, peer_id = ?, "
                    "supports_dht = ?, supports_extension_protocol = ?, supports_fast_protocol = ? "
                    "WHERE info_hash = ? AND ip = ? AND port = ?",
                    {
                        time_point_to_string(now),
                        peer_id_hex,
                        supports_dht ? "1" : "0",
                        supports_extension_protocol ? "1" : "0",
                        supports_fast_protocol ? "1" : "0",
                        info_hash.to_hex(),
                        endpoint.address(),
                        std::to_string(endpoint.port())
                    }
                );
            } else {
                // Insert new peer
                auto now = std::chrono::system_clock::now();

                std::string peer_id_hex = peer_id ? peer_id->to_hex() : "";

                return database_->execute_update(
                    "INSERT INTO peers (info_hash, ip, port, peer_id, first_seen, last_seen, "
                    "supports_dht, supports_extension_protocol, supports_fast_protocol) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                    {
                        info_hash.to_hex(),
                        endpoint.address(),
                        std::to_string(endpoint.port()),
                        peer_id_hex,
                        time_point_to_string(now),
                        time_point_to_string(now),
                        supports_dht ? "1" : "0",
                        supports_extension_protocol ? "1" : "0",
                        supports_fast_protocol ? "1" : "0"
                    }
                );
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to store peer: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> store_peer_async(const types::InfoHash& info_hash, const types::Endpoint& endpoint,
                                      const std::optional<types::NodeID>& peer_id,
                                      bool supports_dht,
                                      bool supports_extension_protocol,
                                      bool supports_fast_protocol) {
        return std::async(std::launch::async, [this, info_hash, endpoint, peer_id, supports_dht, supports_extension_protocol, supports_fast_protocol]() {
            return store_peer(info_hash, endpoint, peer_id, supports_dht, supports_extension_protocol, supports_fast_protocol);
        });
    }

    bool store_tracker(const types::InfoHash& info_hash, const std::string& url) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Ensure infohash exists
            if (!store_infohash(info_hash)) {
                return false;
            }

            // Check if tracker exists
            auto result = database_->execute(
                "SELECT id FROM trackers WHERE info_hash = ? AND url = ?",
                {
                    info_hash.to_hex(),
                    url
                }
            );

            if (result.next()) {
                // Update existing tracker
                auto now = std::chrono::system_clock::now();

                return database_->execute_update(
                    "UPDATE trackers SET last_seen = ? WHERE info_hash = ? AND url = ?",
                    {
                        time_point_to_string(now),
                        info_hash.to_hex(),
                        url
                    }
                );
            } else {
                // Insert new tracker
                auto now = std::chrono::system_clock::now();

                return database_->execute_update(
                    "INSERT INTO trackers (info_hash, url, first_seen, last_seen) VALUES (?, ?, ?, ?)",
                    {
                        info_hash.to_hex(),
                        url,
                        time_point_to_string(now),
                        time_point_to_string(now)
                    }
                );
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to store tracker: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> store_tracker_async(const types::InfoHash& info_hash, const std::string& url) {
        return std::async(std::launch::async, [this, info_hash, url]() {
            return store_tracker(info_hash, url);
        });
    }

    bool increment_tracker_announce_count(const types::InfoHash& info_hash, const std::string& url) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Ensure tracker exists
            if (!store_tracker(info_hash, url)) {
                return false;
            }

            auto now = std::chrono::system_clock::now();

            return database_->execute_update(
                "UPDATE trackers SET last_seen = ?, announce_count = announce_count + 1 WHERE info_hash = ? AND url = ?",
                {
                    time_point_to_string(now),
                    info_hash.to_hex(),
                    url
                }
            );
        } catch (const std::exception& e) {
            std::cerr << "Failed to increment tracker announce count: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> increment_tracker_announce_count_async(const types::InfoHash& info_hash, const std::string& url) {
        return std::async(std::launch::async, [this, info_hash, url]() {
            return increment_tracker_announce_count(info_hash, url);
        });
    }

    bool increment_tracker_scrape_count(const types::InfoHash& info_hash, const std::string& url) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Storage manager not initialized" << std::endl;
            return false;
        }

        try {
            // Ensure tracker exists
            if (!store_tracker(info_hash, url)) {
                return false;
            }

            auto now = std::chrono::system_clock::now();

            return database_->execute_update(
                "UPDATE trackers SET last_seen = ?, scrape_count = scrape_count + 1 WHERE info_hash = ? AND url = ?",
                {
                    time_point_to_string(now),
                    info_hash.to_hex(),
                    url
                }
            );
        } catch (const std::exception& e) {
            std::cerr << "Failed to increment tracker scrape count: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> increment_tracker_scrape_count_async(const types::InfoHash& info_hash, const std::string& url) {
        return std::async(std::launch::async, [this, info_hash, url]() {
            return increment_tracker_scrape_count(info_hash, url);
        });
    }

    // Helper function to convert time_point to string
    std::string time_point_to_string(const std::chrono::system_clock::time_point& time_point) {
        auto time_t = std::chrono::system_clock::to_time_t(time_point);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    std::shared_ptr<QueryInterface> query_interface() const {
        return query_interface_;
    }

    std::shared_ptr<Database> database() const {
        return database_;
    }



    std::unordered_map<std::string, std::string> get_statistics() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::unordered_map<std::string, std::string> stats;

        if (!initialized_) {
            return stats;
        }

        try {
            // Get node count
            auto result = database_->execute("SELECT COUNT(*) AS count FROM nodes");
            if (result.next()) {
                stats["storage.node_count"] = std::to_string(result.get_int("count"));
            }

            // Get infohash count
            result = database_->execute("SELECT COUNT(*) AS count FROM infohashes");
            if (result.next()) {
                stats["storage.infohash_count"] = std::to_string(result.get_int("count"));
            }

            // Get metadata count
            result = database_->execute("SELECT COUNT(*) AS count FROM metadata");
            if (result.next()) {
                stats["storage.metadata_count"] = std::to_string(result.get_int("count"));
            }

            // Get file count
            result = database_->execute("SELECT COUNT(*) AS count FROM files");
            if (result.next()) {
                stats["storage.file_count"] = std::to_string(result.get_int("count"));
            }

            // Get tracker count
            result = database_->execute("SELECT COUNT(*) AS count FROM trackers");
            if (result.next()) {
                stats["storage.tracker_count"] = std::to_string(result.get_int("count"));
            }

            // Get peer count
            result = database_->execute("SELECT COUNT(*) AS count FROM peers");
            if (result.next()) {
                stats["storage.peer_count"] = std::to_string(result.get_int("count"));
            }

            // Get database size
            result = database_->execute("PRAGMA page_count");
            int page_count = 0;
            if (result.next()) {
                page_count = result.get_int(0);
            }

            result = database_->execute("PRAGMA page_size");
            int page_size = 0;
            if (result.next()) {
                page_size = result.get_int(0);
            }

            int64_t db_size = static_cast<int64_t>(page_count) * static_cast<int64_t>(page_size);
            stats["storage.database_size"] = std::to_string(db_size);

            // Get database path
            stats["storage.database_path"] = database_->path();

            return stats;
        } catch (const std::exception& e) {
            std::cerr << "Failed to get statistics: " << e.what() << std::endl;
            return stats;
        }
    }

    std::future<std::unordered_map<std::string, std::string>> get_statistics_async() const {
        return std::async(std::launch::async, [this]() {
            return get_statistics();
        });
    }

private:
    std::string db_path_;
    std::shared_ptr<Database> database_;
    std::shared_ptr<QueryInterface> query_interface_;
    bool initialized_;
    mutable std::mutex mutex_;
};

// StorageManager public methods

StorageManager::StorageManager(const std::string& db_path, bool persistent)
    : impl_(std::make_unique<Impl>(db_path, persistent)) {
}

StorageManager::~StorageManager() = default;

bool StorageManager::initialize() {
    return impl_->initialize();
}

std::future<bool> StorageManager::initialize_async() {
    return impl_->initialize_async();
}

bool StorageManager::close() {
    return impl_->close();
}

std::future<bool> StorageManager::close_async() {
    return impl_->close_async();
}

bool StorageManager::store_node(const types::NodeID& node_id, const types::Endpoint& endpoint, bool is_responsive) {
    return impl_->store_node(node_id, endpoint, is_responsive);
}

std::future<bool> StorageManager::store_node_async(const types::NodeID& node_id, const types::Endpoint& endpoint, bool is_responsive) {
    return impl_->store_node_async(node_id, endpoint, is_responsive);
}

std::shared_ptr<QueryInterface> StorageManager::query_interface() const {
    return impl_->query_interface_;
}

std::shared_ptr<Database> StorageManager::database() const {
    return impl_->database_;
}

std::unordered_map<std::string, std::string> StorageManager::get_statistics() const {
    return impl_->get_statistics();
}

std::future<std::unordered_map<std::string, std::string>> StorageManager::get_statistics_async() const {
    return impl_->get_statistics_async();
}

bool StorageManager::update_node_responsiveness(const types::NodeID& node_id, bool is_responsive) {
    return impl_->update_node_responsiveness(node_id, is_responsive);
}

std::future<bool> StorageManager::update_node_responsiveness_async(const types::NodeID& node_id, bool is_responsive) {
    return impl_->update_node_responsiveness_async(node_id, is_responsive);
}

bool StorageManager::increment_node_ping_count(const types::NodeID& node_id) {
    return impl_->increment_node_ping_count(node_id);
}

std::future<bool> StorageManager::increment_node_ping_count_async(const types::NodeID& node_id) {
    return impl_->increment_node_ping_count_async(node_id);
}

bool StorageManager::increment_node_query_count(const types::NodeID& node_id) {
    return impl_->increment_node_query_count(node_id);
}

std::future<bool> StorageManager::increment_node_query_count_async(const types::NodeID& node_id) {
    return impl_->increment_node_query_count_async(node_id);
}

bool StorageManager::increment_node_response_count(const types::NodeID& node_id) {
    return impl_->increment_node_response_count(node_id);
}

std::future<bool> StorageManager::increment_node_response_count_async(const types::NodeID& node_id) {
    return impl_->increment_node_response_count_async(node_id);
}

bool StorageManager::store_infohash(const types::InfoHash& info_hash) {
    return impl_->store_infohash(info_hash);
}

std::future<bool> StorageManager::store_infohash_async(const types::InfoHash& info_hash) {
    return impl_->store_infohash_async(info_hash);
}

bool StorageManager::increment_infohash_announce_count(const types::InfoHash& info_hash) {
    return impl_->increment_infohash_announce_count(info_hash);
}

std::future<bool> StorageManager::increment_infohash_announce_count_async(const types::InfoHash& info_hash) {
    return impl_->increment_infohash_announce_count_async(info_hash);
}

bool StorageManager::increment_infohash_peer_count(const types::InfoHash& info_hash) {
    return impl_->increment_infohash_peer_count(info_hash);
}

std::future<bool> StorageManager::increment_infohash_peer_count_async(const types::InfoHash& info_hash) {
    return impl_->increment_infohash_peer_count_async(info_hash);
}

bool StorageManager::store_metadata(const types::InfoHash& info_hash, const types::MetadataInfo& metadata) {
    return impl_->store_metadata(info_hash, metadata);
}

std::future<bool> StorageManager::store_metadata_async(const types::InfoHash& info_hash, const types::MetadataInfo& metadata) {
    return impl_->store_metadata_async(info_hash, metadata);
}

bool StorageManager::store_torrent(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info) {
    return impl_->store_torrent(info_hash, torrent_info);
}

std::future<bool> StorageManager::store_torrent_async(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info) {
    return impl_->store_torrent_async(info_hash, torrent_info);
}

bool StorageManager::store_peer(const types::InfoHash& info_hash, const types::Endpoint& endpoint,
                               const std::optional<types::NodeID>& peer_id,
                               bool supports_dht,
                               bool supports_extension_protocol,
                               bool supports_fast_protocol) {
    return impl_->store_peer(info_hash, endpoint, peer_id, supports_dht, supports_extension_protocol, supports_fast_protocol);
}

std::future<bool> StorageManager::store_peer_async(const types::InfoHash& info_hash, const types::Endpoint& endpoint,
                                                  const std::optional<types::NodeID>& peer_id,
                                                  bool supports_dht,
                                                  bool supports_extension_protocol,
                                                  bool supports_fast_protocol) {
    return impl_->store_peer_async(info_hash, endpoint, peer_id, supports_dht, supports_extension_protocol, supports_fast_protocol);
}

bool StorageManager::store_tracker(const types::InfoHash& info_hash, const std::string& url) {
    return impl_->store_tracker(info_hash, url);
}

std::future<bool> StorageManager::store_tracker_async(const types::InfoHash& info_hash, const std::string& url) {
    return impl_->store_tracker_async(info_hash, url);
}

bool StorageManager::increment_tracker_announce_count(const types::InfoHash& info_hash, const std::string& url) {
    return impl_->increment_tracker_announce_count(info_hash, url);
}

std::future<bool> StorageManager::increment_tracker_announce_count_async(const types::InfoHash& info_hash, const std::string& url) {
    return impl_->increment_tracker_announce_count_async(info_hash, url);
}

bool StorageManager::increment_tracker_scrape_count(const types::InfoHash& info_hash, const std::string& url) {
    return impl_->increment_tracker_scrape_count(info_hash, url);
}

std::future<bool> StorageManager::increment_tracker_scrape_count_async(const types::InfoHash& info_hash, const std::string& url) {
    return impl_->increment_tracker_scrape_count_async(info_hash, url);
}



std::shared_ptr<StorageManager> create_storage_manager(const std::string& db_path, bool persistent) {
    return std::make_shared<StorageManager>(db_path, persistent);
}

} // namespace bitscrape::storage
