#include <bitscrape/core/persistence.hpp>

#include <iostream>
#include <mutex>
#include <sqlite3.h>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace bitscrape::core {

class Persistence::Impl {
public:
    Impl(const std::string& db_path)
        : db_path_(db_path), db_(nullptr), initialized_(false) {
    }

    ~Impl() {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_) {
            return true;
        }
        
        try {
            // Create directory if it doesn't exist
            std::filesystem::path path(db_path_);
            std::filesystem::create_directories(path.parent_path());
            
            // Open database
            int rc = sqlite3_open(db_path_.c_str(), &db_);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
                return false;
            }
            
            // Create tables
            if (!create_tables()) {
                std::cerr << "Failed to create tables" << std::endl;
                return false;
            }
            
            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize persistence: " << e.what() << std::endl;
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
        
        if (db_) {
            int rc = sqlite3_close(db_);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to close database: " << sqlite3_errmsg(db_) << std::endl;
                return false;
            }
            db_ = nullptr;
        }
        
        initialized_ = false;
        return true;
    }

    std::future<bool> close_async() {
        return std::async(std::launch::async, [this]() {
            return close();
        });
    }

    bool store_node(const types::NodeID& node_id, const types::Endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            std::cerr << "Persistence not initialized" << std::endl;
            return false;
        }
        
        try {
            // Prepare statement
            sqlite3_stmt* stmt;
            const char* sql = "INSERT OR REPLACE INTO nodes (node_id, ip, port, last_seen) VALUES (?, ?, ?, ?)";
            
            int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
                return false;
            }
            
            // Bind parameters
            std::string node_id_str = node_id.to_string();
            std::string ip = endpoint.get_address();
            int port = endpoint.get_port();
            
            sqlite3_bind_text(stmt, 1, node_id_str.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, ip.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 3, port);
            
            // Current timestamp
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            sqlite3_bind_int64(stmt, 4, timestamp);
            
            // Execute statement
            rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to store node: " << sqlite3_errmsg(db_) << std::endl;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to store node: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> store_node_async(const types::NodeID& node_id, const types::Endpoint& endpoint) {
        return std::async(std::launch::async, [this, node_id, endpoint]() {
            return store_node(node_id, endpoint);
        });
    }

    bool store_infohash(const types::InfoHash& info_hash) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            std::cerr << "Persistence not initialized" << std::endl;
            return false;
        }
        
        try {
            // Prepare statement
            sqlite3_stmt* stmt;
            const char* sql = "INSERT OR IGNORE INTO infohashes (info_hash, discovered_at) VALUES (?, ?)";
            
            int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
                return false;
            }
            
            // Bind parameters
            std::string info_hash_str = info_hash.to_string();
            
            sqlite3_bind_text(stmt, 1, info_hash_str.c_str(), -1, SQLITE_TRANSIENT);
            
            // Current timestamp
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            sqlite3_bind_int64(stmt, 2, timestamp);
            
            // Execute statement
            rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            if (rc != SQLITE_DONE) {
                std::cerr << "Failed to store infohash: " << sqlite3_errmsg(db_) << std::endl;
                return false;
            }
            
            return true;
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

    bool store_metadata(const types::InfoHash& info_hash, const types::MetadataInfo& metadata) {
        // TODO: Implement metadata storage
        return false;
    }

    std::future<bool> store_metadata_async(const types::InfoHash& info_hash, const types::MetadataInfo& metadata) {
        return std::async(std::launch::async, [this, info_hash, metadata]() {
            return store_metadata(info_hash, metadata);
        });
    }

    bool store_torrent_info(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info) {
        // TODO: Implement torrent info storage
        return false;
    }

    std::future<bool> store_torrent_info_async(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info) {
        return std::async(std::launch::async, [this, info_hash, torrent_info]() {
            return store_torrent_info(info_hash, torrent_info);
        });
    }

    std::optional<types::Endpoint> get_node(const types::NodeID& node_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            std::cerr << "Persistence not initialized" << std::endl;
            return std::nullopt;
        }
        
        try {
            // Prepare statement
            sqlite3_stmt* stmt;
            const char* sql = "SELECT ip, port FROM nodes WHERE node_id = ?";
            
            int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
                return std::nullopt;
            }
            
            // Bind parameters
            std::string node_id_str = node_id.to_string();
            sqlite3_bind_text(stmt, 1, node_id_str.c_str(), -1, SQLITE_TRANSIENT);
            
            // Execute statement
            rc = sqlite3_step(stmt);
            
            if (rc == SQLITE_ROW) {
                // Get result
                std::string ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                int port = sqlite3_column_int(stmt, 1);
                
                sqlite3_finalize(stmt);
                return types::Endpoint(ip, port);
            } else {
                sqlite3_finalize(stmt);
                return std::nullopt;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to get node: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    std::future<std::optional<types::Endpoint>> get_node_async(const types::NodeID& node_id) const {
        return std::async(std::launch::async, [this, node_id]() {
            return get_node(node_id);
        });
    }

    std::vector<std::pair<types::NodeID, types::Endpoint>> get_nodes(size_t limit, size_t offset) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::pair<types::NodeID, types::Endpoint>> result;
        
        if (!initialized_) {
            std::cerr << "Persistence not initialized" << std::endl;
            return result;
        }
        
        try {
            // Prepare statement
            sqlite3_stmt* stmt;
            const char* sql = "SELECT node_id, ip, port FROM nodes ORDER BY last_seen DESC LIMIT ? OFFSET ?";
            
            int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
                return result;
            }
            
            // Bind parameters
            sqlite3_bind_int(stmt, 1, static_cast<int>(limit));
            sqlite3_bind_int(stmt, 2, static_cast<int>(offset));
            
            // Execute statement
            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                // Get result
                std::string node_id_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                std::string ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                int port = sqlite3_column_int(stmt, 2);
                
                types::NodeID node_id = types::NodeID::from_string(node_id_str);
                types::Endpoint endpoint(ip, port);
                
                result.emplace_back(node_id, endpoint);
            }
            
            sqlite3_finalize(stmt);
            
            return result;
        } catch (const std::exception& e) {
            std::cerr << "Failed to get nodes: " << e.what() << std::endl;
            return result;
        }
    }

    std::future<std::vector<std::pair<types::NodeID, types::Endpoint>>> get_nodes_async(size_t limit, size_t offset) const {
        return std::async(std::launch::async, [this, limit, offset]() {
            return get_nodes(limit, offset);
        });
    }

    std::vector<types::InfoHash> get_infohashes(size_t limit, size_t offset) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<types::InfoHash> result;
        
        if (!initialized_) {
            std::cerr << "Persistence not initialized" << std::endl;
            return result;
        }
        
        try {
            // Prepare statement
            sqlite3_stmt* stmt;
            const char* sql = "SELECT info_hash FROM infohashes ORDER BY discovered_at DESC LIMIT ? OFFSET ?";
            
            int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
                return result;
            }
            
            // Bind parameters
            sqlite3_bind_int(stmt, 1, static_cast<int>(limit));
            sqlite3_bind_int(stmt, 2, static_cast<int>(offset));
            
            // Execute statement
            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                // Get result
                std::string info_hash_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                types::InfoHash info_hash = types::InfoHash::from_string(info_hash_str);
                result.push_back(info_hash);
            }
            
            sqlite3_finalize(stmt);
            
            return result;
        } catch (const std::exception& e) {
            std::cerr << "Failed to get infohashes: " << e.what() << std::endl;
            return result;
        }
    }

    std::future<std::vector<types::InfoHash>> get_infohashes_async(size_t limit, size_t offset) const {
        return std::async(std::launch::async, [this, limit, offset]() {
            return get_infohashes(limit, offset);
        });
    }

    std::optional<types::MetadataInfo> get_metadata(const types::InfoHash& info_hash) const {
        // TODO: Implement metadata retrieval
        return std::nullopt;
    }

    std::future<std::optional<types::MetadataInfo>> get_metadata_async(const types::InfoHash& info_hash) const {
        return std::async(std::launch::async, [this, info_hash]() {
            return get_metadata(info_hash);
        });
    }

    std::optional<types::TorrentInfo> get_torrent_info(const types::InfoHash& info_hash) const {
        // TODO: Implement torrent info retrieval
        return std::nullopt;
    }

    std::future<std::optional<types::TorrentInfo>> get_torrent_info_async(const types::InfoHash& info_hash) const {
        return std::async(std::launch::async, [this, info_hash]() {
            return get_torrent_info(info_hash);
        });
    }

    std::unordered_map<std::string, std::string> get_statistics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::unordered_map<std::string, std::string> stats;
        
        if (!initialized_) {
            stats["persistence.initialized"] = "false";
            return stats;
        }
        
        stats["persistence.initialized"] = "true";
        stats["persistence.db_path"] = db_path_;
        
        try {
            // Get node count
            sqlite3_stmt* stmt;
            int rc = sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM nodes", -1, &stmt, nullptr);
            if (rc == SQLITE_OK) {
                rc = sqlite3_step(stmt);
                if (rc == SQLITE_ROW) {
                    int count = sqlite3_column_int(stmt, 0);
                    stats["persistence.node_count"] = std::to_string(count);
                }
                sqlite3_finalize(stmt);
            }
            
            // Get infohash count
            rc = sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM infohashes", -1, &stmt, nullptr);
            if (rc == SQLITE_OK) {
                rc = sqlite3_step(stmt);
                if (rc == SQLITE_ROW) {
                    int count = sqlite3_column_int(stmt, 0);
                    stats["persistence.infohash_count"] = std::to_string(count);
                }
                sqlite3_finalize(stmt);
            }
            
            // Get metadata count
            rc = sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM metadata", -1, &stmt, nullptr);
            if (rc == SQLITE_OK) {
                rc = sqlite3_step(stmt);
                if (rc == SQLITE_ROW) {
                    int count = sqlite3_column_int(stmt, 0);
                    stats["persistence.metadata_count"] = std::to_string(count);
                }
                sqlite3_finalize(stmt);
            }
            
            // Get torrent info count
            rc = sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM torrent_info", -1, &stmt, nullptr);
            if (rc == SQLITE_OK) {
                rc = sqlite3_step(stmt);
                if (rc == SQLITE_ROW) {
                    int count = sqlite3_column_int(stmt, 0);
                    stats["persistence.torrent_info_count"] = std::to_string(count);
                }
                sqlite3_finalize(stmt);
            }
            
            return stats;
        } catch (const std::exception& e) {
            std::cerr << "Failed to get statistics: " << e.what() << std::endl;
            stats["persistence.error"] = e.what();
            return stats;
        }
    }

private:
    bool create_tables() {
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS nodes (
                node_id TEXT PRIMARY KEY,
                ip TEXT NOT NULL,
                port INTEGER NOT NULL,
                last_seen INTEGER NOT NULL
            );
            
            CREATE TABLE IF NOT EXISTS infohashes (
                info_hash TEXT PRIMARY KEY,
                discovered_at INTEGER NOT NULL
            );
            
            CREATE TABLE IF NOT EXISTS metadata (
                info_hash TEXT PRIMARY KEY,
                data BLOB NOT NULL,
                downloaded_at INTEGER NOT NULL,
                FOREIGN KEY (info_hash) REFERENCES infohashes (info_hash)
            );
            
            CREATE TABLE IF NOT EXISTS torrent_info (
                info_hash TEXT PRIMARY KEY,
                name TEXT,
                size INTEGER,
                file_count INTEGER,
                created_by TEXT,
                creation_date INTEGER,
                comment TEXT,
                FOREIGN KEY (info_hash) REFERENCES infohashes (info_hash)
            );
            
            CREATE TABLE IF NOT EXISTS files (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                info_hash TEXT NOT NULL,
                path TEXT NOT NULL,
                size INTEGER NOT NULL,
                FOREIGN KEY (info_hash) REFERENCES torrent_info (info_hash)
            );
            
            CREATE INDEX IF NOT EXISTS idx_nodes_last_seen ON nodes (last_seen);
            CREATE INDEX IF NOT EXISTS idx_infohashes_discovered_at ON infohashes (discovered_at);
            CREATE INDEX IF NOT EXISTS idx_metadata_downloaded_at ON metadata (downloaded_at);
            CREATE INDEX IF NOT EXISTS idx_files_info_hash ON files (info_hash);
        )";
        
        char* error_msg = nullptr;
        int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &error_msg);
        
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << error_msg << std::endl;
            sqlite3_free(error_msg);
            return false;
        }
        
        return true;
    }

    std::string db_path_;
    mutable sqlite3* db_;
    bool initialized_;
    mutable std::mutex mutex_;
};

// Persistence implementation

Persistence::Persistence(const std::string& db_path)
    : impl_(std::make_unique<Impl>(db_path)) {
}

Persistence::~Persistence() = default;

bool Persistence::initialize() {
    return impl_->initialize();
}

std::future<bool> Persistence::initialize_async() {
    return impl_->initialize_async();
}

bool Persistence::close() {
    return impl_->close();
}

std::future<bool> Persistence::close_async() {
    return impl_->close_async();
}

bool Persistence::store_node(const types::NodeID& node_id, const types::Endpoint& endpoint) {
    return impl_->store_node(node_id, endpoint);
}

std::future<bool> Persistence::store_node_async(const types::NodeID& node_id, const types::Endpoint& endpoint) {
    return impl_->store_node_async(node_id, endpoint);
}

bool Persistence::store_infohash(const types::InfoHash& info_hash) {
    return impl_->store_infohash(info_hash);
}

std::future<bool> Persistence::store_infohash_async(const types::InfoHash& info_hash) {
    return impl_->store_infohash_async(info_hash);
}

bool Persistence::store_metadata(const types::InfoHash& info_hash, const types::MetadataInfo& metadata) {
    return impl_->store_metadata(info_hash, metadata);
}

std::future<bool> Persistence::store_metadata_async(const types::InfoHash& info_hash, const types::MetadataInfo& metadata) {
    return impl_->store_metadata_async(info_hash, metadata);
}

bool Persistence::store_torrent_info(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info) {
    return impl_->store_torrent_info(info_hash, torrent_info);
}

std::future<bool> Persistence::store_torrent_info_async(const types::InfoHash& info_hash, const types::TorrentInfo& torrent_info) {
    return impl_->store_torrent_info_async(info_hash, torrent_info);
}

std::optional<types::Endpoint> Persistence::get_node(const types::NodeID& node_id) const {
    return impl_->get_node(node_id);
}

std::future<std::optional<types::Endpoint>> Persistence::get_node_async(const types::NodeID& node_id) const {
    return impl_->get_node_async(node_id);
}

std::vector<std::pair<types::NodeID, types::Endpoint>> Persistence::get_nodes(size_t limit, size_t offset) const {
    return impl_->get_nodes(limit, offset);
}

std::future<std::vector<std::pair<types::NodeID, types::Endpoint>>> Persistence::get_nodes_async(size_t limit, size_t offset) const {
    return impl_->get_nodes_async(limit, offset);
}

std::vector<types::InfoHash> Persistence::get_infohashes(size_t limit, size_t offset) const {
    return impl_->get_infohashes(limit, offset);
}

std::future<std::vector<types::InfoHash>> Persistence::get_infohashes_async(size_t limit, size_t offset) const {
    return impl_->get_infohashes_async(limit, offset);
}

std::optional<types::MetadataInfo> Persistence::get_metadata(const types::InfoHash& info_hash) const {
    return impl_->get_metadata(info_hash);
}

std::future<std::optional<types::MetadataInfo>> Persistence::get_metadata_async(const types::InfoHash& info_hash) const {
    return impl_->get_metadata_async(info_hash);
}

std::optional<types::TorrentInfo> Persistence::get_torrent_info(const types::InfoHash& info_hash) const {
    return impl_->get_torrent_info(info_hash);
}

std::future<std::optional<types::TorrentInfo>> Persistence::get_torrent_info_async(const types::InfoHash& info_hash) const {
    return impl_->get_torrent_info_async(info_hash);
}

std::unordered_map<std::string, std::string> Persistence::get_statistics() const {
    return impl_->get_statistics();
}

} // namespace bitscrape::core
