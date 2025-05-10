#pragma once

#include <bitscrape/storage/database.hpp>

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <future>

namespace bitscrape::storage {

/**
 * @brief Database migration manager
 * 
 * Handles database schema migrations to ensure the database is up-to-date.
 */
class MigrationManager {
public:
    /**
     * @brief Migration definition
     */
    struct Migration {
        int version;                                        ///< Migration version
        std::string description;                            ///< Migration description
        std::vector<std::string> up_queries;                ///< Queries to apply the migration
        std::vector<std::string> down_queries;              ///< Queries to revert the migration
    };
    
    /**
     * @brief Create a migration manager
     * 
     * @param database Database instance
     */
    explicit MigrationManager(std::shared_ptr<Database> database);
    
    /**
     * @brief Destructor
     */
    ~MigrationManager();
    
    /**
     * @brief Initialize the migration manager
     * 
     * Creates the migrations table if it doesn't exist.
     * 
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Initialize the migration manager asynchronously
     * 
     * @return Future with the result of the initialization
     */
    std::future<bool> initialize_async();
    
    /**
     * @brief Register a migration
     * 
     * @param migration Migration to register
     */
    void register_migration(const Migration& migration);
    
    /**
     * @brief Get the current database version
     * 
     * @return Current database version
     */
    int current_version() const;
    
    /**
     * @brief Get the latest available migration version
     * 
     * @return Latest available migration version
     */
    int latest_version() const;
    
    /**
     * @brief Check if the database is up-to-date
     * 
     * @return true if up-to-date, false otherwise
     */
    bool is_up_to_date() const;
    
    /**
     * @brief Migrate the database to the latest version
     * 
     * @return true if successful, false otherwise
     */
    bool migrate_up();
    
    /**
     * @brief Migrate the database to the latest version asynchronously
     * 
     * @return Future with the result of the migration
     */
    std::future<bool> migrate_up_async();
    
    /**
     * @brief Migrate the database to a specific version
     * 
     * @param target_version Target version
     * @return true if successful, false otherwise
     */
    bool migrate_to(int target_version);
    
    /**
     * @brief Migrate the database to a specific version asynchronously
     * 
     * @param target_version Target version
     * @return Future with the result of the migration
     */
    std::future<bool> migrate_to_async(int target_version);
    
    /**
     * @brief Rollback the last migration
     * 
     * @return true if successful, false otherwise
     */
    bool rollback();
    
    /**
     * @brief Rollback the last migration asynchronously
     * 
     * @return Future with the result of the rollback
     */
    std::future<bool> rollback_async();
    
    /**
     * @brief Rollback to a specific version
     * 
     * @param target_version Target version
     * @return true if successful, false otherwise
     */
    bool rollback_to(int target_version);
    
    /**
     * @brief Rollback to a specific version asynchronously
     * 
     * @param target_version Target version
     * @return Future with the result of the rollback
     */
    std::future<bool> rollback_to_async(int target_version);
    
    /**
     * @brief Get the migration history
     * 
     * @return Vector of applied migrations
     */
    std::vector<Migration> migration_history() const;
    
    /**
     * @brief Get the migration history asynchronously
     * 
     * @return Future with a vector of applied migrations
     */
    std::future<std::vector<Migration>> migration_history_async() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    // Disable copy
    MigrationManager(const MigrationManager&) = delete;
    MigrationManager& operator=(const MigrationManager&) = delete;
};

} // namespace bitscrape::storage
