#include <gtest/gtest.h>
#include <bitscrape/storage/migration_manager.hpp>
#include <bitscrape/storage/database.hpp>

#include <filesystem>
#include <string>
#include <memory>
#include <vector>

using namespace bitscrape::storage;

class MigrationManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary database file
        test_db_path_ = "test_migration_manager.db";

        // Remove the file if it exists
        std::filesystem::remove(test_db_path_);

        // Create and initialize the database
        db_ = std::make_shared<Database>(test_db_path_);
        ASSERT_TRUE(db_->initialize());

        // Create the migration manager
        migration_manager_ = std::make_unique<MigrationManager>(db_);

        // Initialize the migration manager
        ASSERT_TRUE(migration_manager_->initialize());
    }

    void TearDown() override {
        // Close the database
        db_->close();

        // Remove the test database file
        std::filesystem::remove(test_db_path_);
    }

    std::string test_db_path_;
    std::shared_ptr<Database> db_;
    std::unique_ptr<MigrationManager> migration_manager_;
};

TEST_F(MigrationManagerTest, InitialVersion) {
    // Check if the initial version is 0
    EXPECT_EQ(migration_manager_->current_version(), 0);

    // Check if the latest version is 0 (no migrations registered yet)
    EXPECT_EQ(migration_manager_->latest_version(), 0);

    // Check if the database is up-to-date
    EXPECT_TRUE(migration_manager_->is_up_to_date());
}

TEST_F(MigrationManagerTest, RegisterMigration) {
    // Create a migration
    MigrationManager::Migration migration;
    migration.version = 1;
    migration.description = "Create test table";
    migration.up_queries = {"CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)"};
    migration.down_queries = {"DROP TABLE test"};

    // Register the migration
    migration_manager_->register_migration(migration);

    // Check if the latest version is updated
    EXPECT_EQ(migration_manager_->latest_version(), 1);

    // Check if the database is not up-to-date
    EXPECT_FALSE(migration_manager_->is_up_to_date());
}

TEST_F(MigrationManagerTest, MigrateUp) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create users table";
    migration1.up_queries = {"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)"};
    migration1.down_queries = {"DROP TABLE users"};

    MigrationManager::Migration migration2;
    migration2.version = 2;
    migration2.description = "Create posts table";
    migration2.up_queries = {"CREATE TABLE posts (id INTEGER PRIMARY KEY, user_id INTEGER, content TEXT)"};
    migration2.down_queries = {"DROP TABLE posts"};

    // Register the migrations
    migration_manager_->register_migration(migration1);
    migration_manager_->register_migration(migration2);

    // Migrate up
    EXPECT_TRUE(migration_manager_->migrate_up());

    // Check if the current version is updated
    EXPECT_EQ(migration_manager_->current_version(), 2);

    // Check if the database is up-to-date
    EXPECT_TRUE(migration_manager_->is_up_to_date());

    // Check if the tables were created
    auto users_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='users'");
    EXPECT_TRUE(users_result.next());

    auto posts_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='posts'");
    EXPECT_TRUE(posts_result.next());
}

TEST_F(MigrationManagerTest, MigrateUpAsync) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create products table";
    migration1.up_queries = {"CREATE TABLE products (id INTEGER PRIMARY KEY, name TEXT, price REAL)"};
    migration1.down_queries = {"DROP TABLE products"};

    // Register the migration
    migration_manager_->register_migration(migration1);

    // Migrate up asynchronously
    auto future = migration_manager_->migrate_up_async();

    // Wait for the result
    EXPECT_TRUE(future.get());

    // Check if the current version is updated
    EXPECT_EQ(migration_manager_->current_version(), 1);

    // Check if the database is up-to-date
    EXPECT_TRUE(migration_manager_->is_up_to_date());

    // Check if the table was created
    auto result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='products'");
    EXPECT_TRUE(result.next());
}

TEST_F(MigrationManagerTest, MigrateTo) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create customers table";
    migration1.up_queries = {"CREATE TABLE customers (id INTEGER PRIMARY KEY, name TEXT)"};
    migration1.down_queries = {"DROP TABLE customers"};

    MigrationManager::Migration migration2;
    migration2.version = 2;
    migration2.description = "Create orders table";
    migration2.up_queries = {"CREATE TABLE orders (id INTEGER PRIMARY KEY, customer_id INTEGER, total REAL)"};
    migration2.down_queries = {"DROP TABLE orders"};

    MigrationManager::Migration migration3;
    migration3.version = 3;
    migration3.description = "Create order_items table";
    migration3.up_queries = {"CREATE TABLE order_items (id INTEGER PRIMARY KEY, order_id INTEGER, product_id INTEGER, quantity INTEGER)"};
    migration3.down_queries = {"DROP TABLE order_items"};

    // Register the migrations
    migration_manager_->register_migration(migration1);
    migration_manager_->register_migration(migration2);
    migration_manager_->register_migration(migration3);

    // Migrate to version 2
    EXPECT_TRUE(migration_manager_->migrate_to(2));

    // Check if the current version is updated
    EXPECT_EQ(migration_manager_->current_version(), 2);

    // Check if the database is not up-to-date (since we have migrations up to version 3)
    EXPECT_FALSE(migration_manager_->is_up_to_date());

    // Check if the tables were created
    auto customers_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='customers'");
    EXPECT_TRUE(customers_result.next());

    auto orders_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='orders'");
    EXPECT_TRUE(orders_result.next());

    // Check if the order_items table was not created
    auto order_items_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='order_items'");
    EXPECT_FALSE(order_items_result.next());
}

TEST_F(MigrationManagerTest, MigrateToAsync) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create categories table";
    migration1.up_queries = {"CREATE TABLE categories (id INTEGER PRIMARY KEY, name TEXT)"};
    migration1.down_queries = {"DROP TABLE categories"};

    MigrationManager::Migration migration2;
    migration2.version = 2;
    migration2.description = "Create products table with category_id";
    migration2.up_queries = {"CREATE TABLE products (id INTEGER PRIMARY KEY, category_id INTEGER, name TEXT, price REAL)"};
    migration2.down_queries = {"DROP TABLE products"};

    // Register the migrations
    migration_manager_->register_migration(migration1);
    migration_manager_->register_migration(migration2);

    // Migrate to version 1 asynchronously
    auto future = migration_manager_->migrate_to_async(1);

    // Wait for the result
    EXPECT_TRUE(future.get());

    // Check if the current version is updated
    EXPECT_EQ(migration_manager_->current_version(), 1);

    // Check if the database is not up-to-date
    EXPECT_FALSE(migration_manager_->is_up_to_date());

    // Check if the categories table was created
    auto categories_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='categories'");
    EXPECT_TRUE(categories_result.next());

    // Check if the products table was not created
    auto products_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='products'");
    EXPECT_FALSE(products_result.next());
}

TEST_F(MigrationManagerTest, Rollback) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create users table";
    migration1.up_queries = {"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)"};
    migration1.down_queries = {"DROP TABLE users"};

    MigrationManager::Migration migration2;
    migration2.version = 2;
    migration2.description = "Create posts table";
    migration2.up_queries = {"CREATE TABLE posts (id INTEGER PRIMARY KEY, user_id INTEGER, content TEXT)"};
    migration2.down_queries = {"DROP TABLE posts"};

    // Register the migrations
    migration_manager_->register_migration(migration1);
    migration_manager_->register_migration(migration2);

    // Migrate up
    EXPECT_TRUE(migration_manager_->migrate_up());

    // Check if the current version is 2
    EXPECT_EQ(migration_manager_->current_version(), 2);

    // Rollback the last migration
    EXPECT_TRUE(migration_manager_->rollback());

    // Check if the current version is 1
    EXPECT_EQ(migration_manager_->current_version(), 1);

    // Check if the users table still exists
    auto users_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='users'");
    EXPECT_TRUE(users_result.next());

    // Check if the posts table was dropped
    auto posts_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='posts'");
    EXPECT_FALSE(posts_result.next());
}

TEST_F(MigrationManagerTest, RollbackAsync) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create products table";
    migration1.up_queries = {"CREATE TABLE products (id INTEGER PRIMARY KEY, name TEXT, price REAL)"};
    migration1.down_queries = {"DROP TABLE products"};

    // Register the migration
    migration_manager_->register_migration(migration1);

    // Migrate up
    EXPECT_TRUE(migration_manager_->migrate_up());

    // Check if the current version is 1
    EXPECT_EQ(migration_manager_->current_version(), 1);

    // Rollback the last migration asynchronously
    auto future = migration_manager_->rollback_async();

    // Wait for the result
    EXPECT_TRUE(future.get());

    // Check if the current version is 0
    EXPECT_EQ(migration_manager_->current_version(), 0);

    // Check if the products table was dropped
    auto result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='products'");
    EXPECT_FALSE(result.next());
}

TEST_F(MigrationManagerTest, RollbackTo) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create customers table";
    migration1.up_queries = {"CREATE TABLE customers (id INTEGER PRIMARY KEY, name TEXT)"};
    migration1.down_queries = {"DROP TABLE customers"};

    MigrationManager::Migration migration2;
    migration2.version = 2;
    migration2.description = "Create orders table";
    migration2.up_queries = {"CREATE TABLE orders (id INTEGER PRIMARY KEY, customer_id INTEGER, total REAL)"};
    migration2.down_queries = {"DROP TABLE orders"};

    MigrationManager::Migration migration3;
    migration3.version = 3;
    migration3.description = "Create order_items table";
    migration3.up_queries = {"CREATE TABLE order_items (id INTEGER PRIMARY KEY, order_id INTEGER, product_id INTEGER, quantity INTEGER)"};
    migration3.down_queries = {"DROP TABLE order_items"};

    // Register the migrations
    migration_manager_->register_migration(migration1);
    migration_manager_->register_migration(migration2);
    migration_manager_->register_migration(migration3);

    // Migrate up
    EXPECT_TRUE(migration_manager_->migrate_up());

    // Check if the current version is 3
    EXPECT_EQ(migration_manager_->current_version(), 3);

    // Rollback to version 1
    EXPECT_TRUE(migration_manager_->rollback_to(1));

    // Check if the current version is 1
    EXPECT_EQ(migration_manager_->current_version(), 1);

    // Check if the customers table still exists
    auto customers_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='customers'");
    EXPECT_TRUE(customers_result.next());

    // Check if the orders and order_items tables were dropped
    auto orders_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='orders'");
    EXPECT_FALSE(orders_result.next());

    auto order_items_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='order_items'");
    EXPECT_FALSE(order_items_result.next());
}

TEST_F(MigrationManagerTest, RollbackToAsync) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create categories table";
    migration1.up_queries = {"CREATE TABLE categories (id INTEGER PRIMARY KEY, name TEXT)"};
    migration1.down_queries = {"DROP TABLE categories"};

    MigrationManager::Migration migration2;
    migration2.version = 2;
    migration2.description = "Create products table";
    migration2.up_queries = {"CREATE TABLE products (id INTEGER PRIMARY KEY, category_id INTEGER, name TEXT, price REAL)"};
    migration2.down_queries = {"DROP TABLE products"};

    MigrationManager::Migration migration3;
    migration3.version = 3;
    migration3.description = "Create inventory table";
    migration3.up_queries = {"CREATE TABLE inventory (id INTEGER PRIMARY KEY, product_id INTEGER, quantity INTEGER)"};
    migration3.down_queries = {"DROP TABLE inventory"};

    // Register the migrations
    migration_manager_->register_migration(migration1);
    migration_manager_->register_migration(migration2);
    migration_manager_->register_migration(migration3);

    // Migrate up
    EXPECT_TRUE(migration_manager_->migrate_up());

    // Check if the current version is 3
    EXPECT_EQ(migration_manager_->current_version(), 3);

    // Rollback to version 0 asynchronously
    auto future = migration_manager_->rollback_to_async(0);

    // Wait for the result
    EXPECT_TRUE(future.get());

    // Check if the current version is 0
    EXPECT_EQ(migration_manager_->current_version(), 0);

    // Check if all tables were dropped
    auto categories_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='categories'");
    EXPECT_FALSE(categories_result.next());

    auto products_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='products'");
    EXPECT_FALSE(products_result.next());

    auto inventory_result = db_->execute("SELECT name FROM sqlite_master WHERE type='table' AND name='inventory'");
    EXPECT_FALSE(inventory_result.next());
}

TEST_F(MigrationManagerTest, MigrationHistory) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create users table";
    migration1.up_queries = {"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)"};
    migration1.down_queries = {"DROP TABLE users"};

    MigrationManager::Migration migration2;
    migration2.version = 2;
    migration2.description = "Create posts table";
    migration2.up_queries = {"CREATE TABLE posts (id INTEGER PRIMARY KEY, user_id INTEGER, content TEXT)"};
    migration2.down_queries = {"DROP TABLE posts"};

    // Register the migrations
    migration_manager_->register_migration(migration1);
    migration_manager_->register_migration(migration2);

    // Migrate up
    EXPECT_TRUE(migration_manager_->migrate_up());

    // Get migration history
    auto history = migration_manager_->migration_history();

    // Check if we have the expected number of migrations
    EXPECT_EQ(history.size(), 2);

    // Check if the migrations are in the correct order
    EXPECT_EQ(history[0].version, 1);
    EXPECT_EQ(history[0].description, "Create users table");

    EXPECT_EQ(history[1].version, 2);
    EXPECT_EQ(history[1].description, "Create posts table");
}

TEST_F(MigrationManagerTest, MigrationHistoryAsync) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create products table";
    migration1.up_queries = {"CREATE TABLE products (id INTEGER PRIMARY KEY, name TEXT, price REAL)"};
    migration1.down_queries = {"DROP TABLE products"};

    // Register the migration
    migration_manager_->register_migration(migration1);

    // Migrate up
    EXPECT_TRUE(migration_manager_->migrate_up());

    // Get migration history asynchronously
    auto future = migration_manager_->migration_history_async();

    // Wait for the result
    auto history = future.get();

    // Check if we have the expected number of migrations
    EXPECT_EQ(history.size(), 1);

    // Check if the migration is correct
    EXPECT_EQ(history[0].version, 1);
    EXPECT_EQ(history[0].description, "Create products table");
}

TEST_F(MigrationManagerTest, RegisterMigrationWithInvalidVersion) {
    // Create a migration with an invalid version (0)
    MigrationManager::Migration migration;
    migration.version = 0;
    migration.description = "Invalid migration";
    migration.up_queries = {"CREATE TABLE invalid (id INTEGER PRIMARY KEY)"};
    migration.down_queries = {"DROP TABLE invalid"};

    // Try to register the migration
    EXPECT_THROW(migration_manager_->register_migration(migration), std::invalid_argument);

    // Check if the latest version is still 0
    EXPECT_EQ(migration_manager_->latest_version(), 0);
}

TEST_F(MigrationManagerTest, RegisterMigrationWithDuplicateVersion) {
    // Create and register a migration
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "First migration";
    migration1.up_queries = {"CREATE TABLE first (id INTEGER PRIMARY KEY)"};
    migration1.down_queries = {"DROP TABLE first"};

    // Register the migration
    migration_manager_->register_migration(migration1);

    // Create another migration with the same version
    MigrationManager::Migration migration2;
    migration2.version = 1; // Same version as migration1
    migration2.description = "Duplicate version migration";
    migration2.up_queries = {"CREATE TABLE duplicate (id INTEGER PRIMARY KEY)"};
    migration2.down_queries = {"DROP TABLE duplicate"};

    // Try to register the migration
    EXPECT_THROW(migration_manager_->register_migration(migration2), std::invalid_argument);

    // Check if the latest version is still 1
    EXPECT_EQ(migration_manager_->latest_version(), 1);
}

TEST_F(MigrationManagerTest, MigrateToInvalidVersion) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create users table";
    migration1.up_queries = {"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)"};
    migration1.down_queries = {"DROP TABLE users"};

    MigrationManager::Migration migration2;
    migration2.version = 2;
    migration2.description = "Create posts table";
    migration2.up_queries = {"CREATE TABLE posts (id INTEGER PRIMARY KEY, user_id INTEGER, content TEXT)"};
    migration2.down_queries = {"DROP TABLE posts"};

    // Register the migrations
    migration_manager_->register_migration(migration1);
    migration_manager_->register_migration(migration2);

    // Try to migrate to an invalid version (3, which doesn't exist)
    EXPECT_FALSE(migration_manager_->migrate_to(3));

    // Check if the current version is still 0
    EXPECT_EQ(migration_manager_->current_version(), 0);
}

TEST_F(MigrationManagerTest, RollbackToInvalidVersion) {
    // Create and register migrations
    MigrationManager::Migration migration1;
    migration1.version = 1;
    migration1.description = "Create users table";
    migration1.up_queries = {"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)"};
    migration1.down_queries = {"DROP TABLE users"};

    MigrationManager::Migration migration2;
    migration2.version = 2;
    migration2.description = "Create posts table";
    migration2.up_queries = {"CREATE TABLE posts (id INTEGER PRIMARY KEY, user_id INTEGER, content TEXT)"};
    migration2.down_queries = {"DROP TABLE posts"};

    // Register the migrations
    migration_manager_->register_migration(migration1);
    migration_manager_->register_migration(migration2);

    // Migrate up
    EXPECT_TRUE(migration_manager_->migrate_up());

    // Try to rollback to an invalid version (3, which doesn't exist)
    EXPECT_FALSE(migration_manager_->rollback_to(3));

    // Check if the current version is still 2
    EXPECT_EQ(migration_manager_->current_version(), 2);
}

TEST_F(MigrationManagerTest, MigrateWithFailingQuery) {
    // Create a migration with an invalid SQL query
    MigrationManager::Migration migration;
    migration.version = 1;
    migration.description = "Invalid SQL query";
    migration.up_queries = {"CREATE TABLE invalid (id INTEGER PRIMARY KEY, INVALID SYNTAX)"};
    migration.down_queries = {"DROP TABLE invalid"};

    // Register the migration
    migration_manager_->register_migration(migration);

    // Try to migrate up
    EXPECT_FALSE(migration_manager_->migrate_up());

    // Check if the current version is still 0
    EXPECT_EQ(migration_manager_->current_version(), 0);
}

TEST_F(MigrationManagerTest, RollbackWithFailingQuery) {
    // Create a migration with an invalid down query
    MigrationManager::Migration migration;
    migration.version = 1;
    migration.description = "Invalid down query";
    migration.up_queries = {"CREATE TABLE valid (id INTEGER PRIMARY KEY)"};
    migration.down_queries = {"DROP TABLE invalid_table"}; // Table doesn't exist

    // Register the migration
    migration_manager_->register_migration(migration);

    // Migrate up
    EXPECT_TRUE(migration_manager_->migrate_up());

    // Try to rollback
    EXPECT_FALSE(migration_manager_->rollback());

    // Check if the current version is still 1
    EXPECT_EQ(migration_manager_->current_version(), 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
