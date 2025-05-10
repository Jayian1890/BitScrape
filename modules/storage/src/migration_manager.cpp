#include <bitscrape/storage/migration_manager.hpp>

#include <iostream>
#include <algorithm>
#include <sstream>

namespace bitscrape::storage {

class MigrationManager::Impl {
public:
    Impl(std::shared_ptr<Database> database)
        : database_(database), current_version_(0) {
        // Register initial migrations
        register_initial_migrations();
    }
    
    bool initialize() {
        if (!database_->is_initialized()) {
            if (!database_->initialize()) {
                return false;
            }
        }
        
        // Create migrations table if it doesn't exist
        if (!create_migrations_table()) {
            return false;
        }
        
        // Get current version
        current_version_ = get_current_version_from_db();
        
        return true;
    }
    
    std::future<bool> initialize_async() {
        return std::async(std::launch::async, [this]() {
            return initialize();
        });
    }
    
    void register_migration(const Migration& migration) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if migration already exists
        auto it = std::find_if(migrations_.begin(), migrations_.end(),
                              [&migration](const Migration& m) {
                                  return m.version == migration.version;
                              });
        
        if (it != migrations_.end()) {
            // Replace existing migration
            *it = migration;
        } else {
            // Add new migration
            migrations_.push_back(migration);
            
            // Sort migrations by version
            std::sort(migrations_.begin(), migrations_.end(),
                     [](const Migration& a, const Migration& b) {
                         return a.version < b.version;
                     });
        }
    }
    
    int current_version() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_version_;
    }
    
    int latest_version() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (migrations_.empty()) {
            return 0;
        }
        
        return migrations_.back().version;
    }
    
    bool is_up_to_date() const {
        return current_version() >= latest_version();
    }
    
    bool migrate_up() {
        return migrate_to(latest_version());
    }
    
    std::future<bool> migrate_up_async() {
        return std::async(std::launch::async, [this]() {
            return migrate_up();
        });
    }
    
    bool migrate_to(int target_version) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (current_version_ == target_version) {
            return true;
        }
        
        if (current_version_ > target_version) {
            return rollback_to_internal(target_version);
        }
        
        // Find migrations to apply
        std::vector<Migration> migrations_to_apply;
        for (const auto& migration : migrations_) {
            if (migration.version > current_version_ && migration.version <= target_version) {
                migrations_to_apply.push_back(migration);
            }
        }
        
        if (migrations_to_apply.empty()) {
            return true;
        }
        
        // Begin transaction
        if (!database_->begin_transaction()) {
            return false;
        }
        
        try {
            // Apply migrations
            for (const auto& migration : migrations_to_apply) {
                std::cout << "Applying migration " << migration.version << ": " << migration.description << std::endl;
                
                // Apply migration queries
                for (const auto& query : migration.up_queries) {
                    if (!database_->execute_update(query)) {
                        std::cerr << "Failed to apply migration " << migration.version << std::endl;
                        database_->rollback_transaction();
                        return false;
                    }
                }
                
                // Update migrations table
                if (!update_migration_version(migration.version, migration.description)) {
                    std::cerr << "Failed to update migration version" << std::endl;
                    database_->rollback_transaction();
                    return false;
                }
                
                current_version_ = migration.version;
            }
            
            // Commit transaction
            if (!database_->commit_transaction()) {
                std::cerr << "Failed to commit transaction" << std::endl;
                database_->rollback_transaction();
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Migration failed: " << e.what() << std::endl;
            database_->rollback_transaction();
            return false;
        }
    }
    
    std::future<bool> migrate_to_async(int target_version) {
        return std::async(std::launch::async, [this, target_version]() {
            return migrate_to(target_version);
        });
    }
    
    bool rollback() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (current_version_ <= 0) {
            return true;
        }
        
        // Find previous version
        int previous_version = 0;
        for (const auto& migration : migrations_) {
            if (migration.version < current_version_ && migration.version > previous_version) {
                previous_version = migration.version;
            }
        }
        
        return rollback_to_internal(previous_version);
    }
    
    std::future<bool> rollback_async() {
        return std::async(std::launch::async, [this]() {
            return rollback();
        });
    }
    
    bool rollback_to(int target_version) {
        std::lock_guard<std::mutex> lock(mutex_);
        return rollback_to_internal(target_version);
    }
    
    std::future<bool> rollback_to_async(int target_version) {
        return std::async(std::launch::async, [this, target_version]() {
            return rollback_to(target_version);
        });
    }
    
    std::vector<Migration> migration_history() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<Migration> history;
        
        // Get migration history from database
        auto result = database_->execute(
            "SELECT version, description, applied_at FROM migrations ORDER BY version ASC"
        );
        
        while (result.next()) {
            Migration migration;
            migration.version = result.get_int("version");
            migration.description = result.get_string("description");
            
            // Find the migration in our list to get the queries
            auto it = std::find_if(migrations_.begin(), migrations_.end(),
                                  [&migration](const Migration& m) {
                                      return m.version == migration.version;
                                  });
            
            if (it != migrations_.end()) {
                migration.up_queries = it->up_queries;
                migration.down_queries = it->down_queries;
            }
            
            history.push_back(migration);
        }
        
        return history;
    }
    
    std::future<std::vector<Migration>> migration_history_async() const {
        return std::async(std::launch::async, [this]() {
            return migration_history();
        });
    }
    
private:
    std::shared_ptr<Database> database_;
    std::vector<Migration> migrations_;
    int current_version_;
    mutable std::mutex mutex_;
    
    bool create_migrations_table() {
        return database_->execute_update(
            "CREATE TABLE IF NOT EXISTS migrations ("
            "    version INTEGER PRIMARY KEY,"
            "    description TEXT NOT NULL,"
            "    applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");"
        );
    }
    
    int get_current_version_from_db() {
        auto result = database_->execute("SELECT MAX(version) AS version FROM migrations");
        if (result.next()) {
            return result.get_int("version");
        }
        
        return 0;
    }
    
    bool update_migration_version(int version, const std::string& description) {
        return database_->execute_update(
            "INSERT INTO migrations (version, description) VALUES (?, ?)",
            {std::to_string(version), description}
        );
    }
    
    bool rollback_to_internal(int target_version) {
        if (current_version_ <= target_version) {
            return true;
        }
        
        // Find migrations to rollback
        std::vector<Migration> migrations_to_rollback;
        for (auto it = migrations_.rbegin(); it != migrations_.rend(); ++it) {
            if (it->version > target_version && it->version <= current_version_) {
                migrations_to_rollback.push_back(*it);
            }
        }
        
        if (migrations_to_rollback.empty()) {
            return true;
        }
        
        // Begin transaction
        if (!database_->begin_transaction()) {
            return false;
        }
        
        try {
            // Apply rollbacks
            for (const auto& migration : migrations_to_rollback) {
                std::cout << "Rolling back migration " << migration.version << ": " << migration.description << std::endl;
                
                // Apply rollback queries
                for (const auto& query : migration.down_queries) {
                    if (!database_->execute_update(query)) {
                        std::cerr << "Failed to rollback migration " << migration.version << std::endl;
                        database_->rollback_transaction();
                        return false;
                    }
                }
                
                // Delete from migrations table
                if (!database_->execute_update(
                    "DELETE FROM migrations WHERE version = ?",
                    {std::to_string(migration.version)}
                )) {
                    std::cerr << "Failed to update migration version" << std::endl;
                    database_->rollback_transaction();
                    return false;
                }
                
                current_version_ = get_current_version_from_db();
            }
            
            // Commit transaction
            if (!database_->commit_transaction()) {
                std::cerr << "Failed to commit transaction" << std::endl;
                database_->rollback_transaction();
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Rollback failed: " << e.what() << std::endl;
            database_->rollback_transaction();
            return false;
        }
    }
    
    void register_initial_migrations() {
        // Migration 1: Create initial schema
        Migration migration1;
        migration1.version = 1;
        migration1.description = "Create initial schema";
        
        // Up queries
        migration1.up_queries = {
            // Nodes table
            "CREATE TABLE nodes ("
            "    node_id BLOB PRIMARY KEY,"
            "    ip TEXT NOT NULL,"
            "    port INTEGER NOT NULL,"
            "    first_seen TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    last_seen TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    ping_count INTEGER NOT NULL DEFAULT 0,"
            "    query_count INTEGER NOT NULL DEFAULT 0,"
            "    response_count INTEGER NOT NULL DEFAULT 0,"
            "    is_responsive BOOLEAN NOT NULL DEFAULT 0"
            ");",
            
            // Infohashes table
            "CREATE TABLE infohashes ("
            "    info_hash BLOB PRIMARY KEY,"
            "    first_seen TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    last_seen TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    announce_count INTEGER NOT NULL DEFAULT 0,"
            "    peer_count INTEGER NOT NULL DEFAULT 0,"
            "    has_metadata BOOLEAN NOT NULL DEFAULT 0"
            ");",
            
            // Metadata table
            "CREATE TABLE metadata ("
            "    info_hash BLOB PRIMARY KEY,"
            "    download_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    name TEXT NOT NULL,"
            "    total_size INTEGER NOT NULL,"
            "    piece_count INTEGER NOT NULL,"
            "    file_count INTEGER NOT NULL,"
            "    comment TEXT,"
            "    created_by TEXT,"
            "    creation_date TIMESTAMP,"
            "    raw_metadata BLOB NOT NULL,"
            "    FOREIGN KEY (info_hash) REFERENCES infohashes (info_hash) ON DELETE CASCADE"
            ");",
            
            // Files table
            "CREATE TABLE files ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    info_hash BLOB NOT NULL,"
            "    path TEXT NOT NULL,"
            "    size INTEGER NOT NULL,"
            "    FOREIGN KEY (info_hash) REFERENCES metadata (info_hash) ON DELETE CASCADE"
            ");",
            
            // Trackers table
            "CREATE TABLE trackers ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    info_hash BLOB NOT NULL,"
            "    url TEXT NOT NULL,"
            "    first_seen TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    last_seen TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    announce_count INTEGER NOT NULL DEFAULT 0,"
            "    scrape_count INTEGER NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (info_hash) REFERENCES infohashes (info_hash) ON DELETE CASCADE,"
            "    UNIQUE (info_hash, url)"
            ");",
            
            // Peers table
            "CREATE TABLE peers ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    info_hash BLOB NOT NULL,"
            "    ip TEXT NOT NULL,"
            "    port INTEGER NOT NULL,"
            "    peer_id BLOB,"
            "    first_seen TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    last_seen TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    supports_dht BOOLEAN NOT NULL DEFAULT 0,"
            "    supports_extension_protocol BOOLEAN NOT NULL DEFAULT 0,"
            "    supports_fast_protocol BOOLEAN NOT NULL DEFAULT 0,"
            "    FOREIGN KEY (info_hash) REFERENCES infohashes (info_hash) ON DELETE CASCADE,"
            "    UNIQUE (info_hash, ip, port)"
            ");",
            
            // Indexes
            "CREATE INDEX idx_nodes_last_seen ON nodes (last_seen);",
            "CREATE INDEX idx_nodes_is_responsive ON nodes (is_responsive);",
            "CREATE INDEX idx_infohashes_last_seen ON infohashes (last_seen);",
            "CREATE INDEX idx_infohashes_has_metadata ON infohashes (has_metadata);",
            "CREATE INDEX idx_metadata_name ON metadata (name);",
            "CREATE INDEX idx_metadata_download_time ON metadata (download_time);",
            "CREATE INDEX idx_files_info_hash ON files (info_hash);",
            "CREATE INDEX idx_trackers_info_hash ON trackers (info_hash);",
            "CREATE INDEX idx_peers_info_hash ON peers (info_hash);",
            "CREATE INDEX idx_peers_last_seen ON peers (last_seen);"
        };
        
        // Down queries
        migration1.down_queries = {
            "DROP INDEX IF EXISTS idx_peers_last_seen;",
            "DROP INDEX IF EXISTS idx_peers_info_hash;",
            "DROP INDEX IF EXISTS idx_trackers_info_hash;",
            "DROP INDEX IF EXISTS idx_files_info_hash;",
            "DROP INDEX IF EXISTS idx_metadata_download_time;",
            "DROP INDEX IF EXISTS idx_metadata_name;",
            "DROP INDEX IF EXISTS idx_infohashes_has_metadata;",
            "DROP INDEX IF EXISTS idx_infohashes_last_seen;",
            "DROP INDEX IF EXISTS idx_nodes_is_responsive;",
            "DROP INDEX IF EXISTS idx_nodes_last_seen;",
            "DROP TABLE IF EXISTS peers;",
            "DROP TABLE IF EXISTS trackers;",
            "DROP TABLE IF EXISTS files;",
            "DROP TABLE IF EXISTS metadata;",
            "DROP TABLE IF EXISTS infohashes;",
            "DROP TABLE IF EXISTS nodes;"
        };
        
        register_migration(migration1);
    }
};

// MigrationManager public methods

MigrationManager::MigrationManager(std::shared_ptr<Database> database)
    : impl_(std::make_unique<Impl>(database)) {
}

MigrationManager::~MigrationManager() = default;

bool MigrationManager::initialize() {
    return impl_->initialize();
}

std::future<bool> MigrationManager::initialize_async() {
    return impl_->initialize_async();
}

void MigrationManager::register_migration(const Migration& migration) {
    impl_->register_migration(migration);
}

int MigrationManager::current_version() const {
    return impl_->current_version();
}

int MigrationManager::latest_version() const {
    return impl_->latest_version();
}

bool MigrationManager::is_up_to_date() const {
    return impl_->is_up_to_date();
}

bool MigrationManager::migrate_up() {
    return impl_->migrate_up();
}

std::future<bool> MigrationManager::migrate_up_async() {
    return impl_->migrate_up_async();
}

bool MigrationManager::migrate_to(int target_version) {
    return impl_->migrate_to(target_version);
}

std::future<bool> MigrationManager::migrate_to_async(int target_version) {
    return impl_->migrate_to_async(target_version);
}

bool MigrationManager::rollback() {
    return impl_->rollback();
}

std::future<bool> MigrationManager::rollback_async() {
    return impl_->rollback_async();
}

bool MigrationManager::rollback_to(int target_version) {
    return impl_->rollback_to(target_version);
}

std::future<bool> MigrationManager::rollback_to_async(int target_version) {
    return impl_->rollback_to_async(target_version);
}

std::vector<MigrationManager::Migration> MigrationManager::migration_history() const {
    return impl_->migration_history();
}

std::future<std::vector<MigrationManager::Migration>> MigrationManager::migration_history_async() const {
    return impl_->migration_history_async();
}

} // namespace bitscrape::storage
