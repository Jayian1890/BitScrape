#include <bitscrape/storage/storage_manager.hpp>

#include <iostream>
#include <sstream>
#include <chrono>

namespace bitscrape::storage {

class StorageManager::Impl {
public:
    Impl(const std::string& db_path)
        : db_path_(db_path),
          database_(std::make_shared<Database>(db_path)),
          query_interface_(std::make_shared<QueryInterface>(database_)),
          migration_manager_(std::make_shared<MigrationManager>(database_)),
          initialized_(false) {
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
            
            // Initialize migration manager
            if (!migration_manager_->initialize()) {
                std::cerr << "Failed to initialize migration manager" << std::endl;
                return false;
            }
            
            // Apply migrations
            if (!migration_manager_->migrate_up()) {
                std::cerr << "Failed to apply migrations" << std::endl;
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
            // Close database
            if (!database_->close()) {
                std::cerr << "Failed to close database" << std::endl;
                return false;
            }
            
            initialized_ = false;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to close storage manager: " << e.what() << std::endl;
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
    
    std::shared_ptr<MigrationManager> migration_manager() const {
        return migration_manager_;
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
            
            // Get migration version
            stats["storage.migration_version"] = std::to_string(migration_manager_->current_version());
            
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
    std::shared_ptr<MigrationManager> migration_manager_;
    bool initialized_;
    mutable std::mutex mutex_;
};

// StorageManager public methods

StorageManager::StorageManager(const std::string& db_path)
    : impl_(std::make_unique<Impl>(db_path)) {
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
    return impl_->query_interface();
}

std::shared_ptr<Database> StorageManager::database() const {
    return impl_->database();
}

std::shared_ptr<MigrationManager> StorageManager::migration_manager() const {
    return impl_->migration_manager();
}

std::unordered_map<std::string, std::string> StorageManager::get_statistics() const {
    return impl_->get_statistics();
}

std::future<std::unordered_map<std::string, std::string>> StorageManager::get_statistics_async() const {
    return impl_->get_statistics_async();
}

// TODO: Implement remaining methods

} // namespace bitscrape::storage
