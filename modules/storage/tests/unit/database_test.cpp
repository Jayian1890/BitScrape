#include <gtest/gtest.h>
#include <bitscrape/storage/database.hpp>
#include <bitscrape/storage/detail/key_value_store.hpp>

#include <filesystem>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

using namespace bitscrape::storage;

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary database directory and file
        auto current_path = std::filesystem::current_path() / "test_db";
        std::filesystem::create_directories(current_path);
        test_db_path_ = (current_path / "test_database.db").string();

        // Remove the file if it exists
        std::filesystem::remove(test_db_path_);
    }

    void TearDown() override {
        // Remove the test database file
        std::filesystem::remove(test_db_path_);

        // Remove the test directory
        std::filesystem::remove("test_db");
    }

    std::string test_db_path_;
};

TEST_F(DatabaseTest, InitializeAndClose) {
    Database db(test_db_path_);

    // Test initialization
    EXPECT_TRUE(db.initialize());
    EXPECT_TRUE(db.is_initialized());

    // Test path
    EXPECT_EQ(db.path(), test_db_path_);

    // Test close
    EXPECT_TRUE(db.close());
    EXPECT_FALSE(db.is_initialized());
}

TEST_F(DatabaseTest, AsyncInitializeAndClose) {
    Database db(test_db_path_);

    // Test async initialization
    auto init_future = db.initialize_async();
    EXPECT_TRUE(init_future.get());
    EXPECT_TRUE(db.is_initialized());

    // Test async close
    auto close_future = db.close_async();
    EXPECT_TRUE(close_future.get());
    EXPECT_FALSE(db.is_initialized());
}

TEST_F(DatabaseTest, ExecuteUpdate) {
    Database db(test_db_path_);
    ASSERT_TRUE(db.initialize());

    // Create a test table - this is now handled by the key-value store
    // which creates indexes for common prefixes
    EXPECT_TRUE(db.execute_update(
        "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)"
    ));

    // Insert data
    EXPECT_TRUE(db.execute_update(
        "INSERT INTO test (name, value) VALUES (?, ?)",
        {"test1", "42"}
    ));

    // Check last insert rowid - this is now a timestamp-based ID
    EXPECT_GT(db.last_insert_rowid(), 0);

    // Check changes - always returns 1 for success in the key-value implementation
    EXPECT_EQ(db.changes(), 1);
}

TEST_F(DatabaseTest, ExecuteQuery) {
    Database db(test_db_path_);
    ASSERT_TRUE(db.initialize());

    // Create a test table
    ASSERT_TRUE(db.execute_update(
        "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)"
    ));

    // Insert data - with the key-value store, each row gets a timestamp-based ID
    ASSERT_TRUE(db.execute_update(
        "INSERT INTO test (name, value) VALUES (?, ?)",
        {"test1", "42"}
    ));

    ASSERT_TRUE(db.execute_update(
        "INSERT INTO test (name, value) VALUES (?, ?)",
        {"test2", "43"}
    ));

    // Query data - the key-value store implementation doesn't support ORDER BY,
    // but we can still test the basic functionality
    auto result = db.execute("SELECT * FROM test");

    // The key-value implementation returns results differently
    // Each row is a separate entry in the result
    // We should have at least one row
    EXPECT_TRUE(result.has_rows());

    // Check that we can iterate through the results
    // and access the values by name
    bool found_test1 = false;
    bool found_test2 = false;

    while (result.next()) {
        std::string name = result.get_string("name");
        int value = result.get_int("value");

        if (name == "test1" && value == 42) {
            found_test1 = true;
        } else if (name == "test2" && value == 43) {
            found_test2 = true;
        }
    }

    // Verify that we found both test entries
    EXPECT_TRUE(found_test1);
    EXPECT_TRUE(found_test2);
}

TEST_F(DatabaseTest, Transactions) {
    Database db(test_db_path_);
    ASSERT_TRUE(db.initialize());

    // Create a test table
    ASSERT_TRUE(db.execute_update(
        "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)"
    ));

    // Begin transaction
    EXPECT_TRUE(db.begin_transaction());

    // Insert data
    EXPECT_TRUE(db.execute_update(
        "INSERT INTO test (name, value) VALUES (?, ?)",
        {"test1", "42"}
    ));

    // Commit transaction
    EXPECT_TRUE(db.commit_transaction());

    // Check data was committed
    // The key-value store doesn't support COUNT(*), so we'll check if the data exists
    auto result = db.execute("SELECT * FROM test");
    bool found_test1 = false;

    while (result.next()) {
        std::string name = result.get_string("name");
        int value = result.get_int("value");

        if (name == "test1" && value == 42) {
            found_test1 = true;
        }
    }

    EXPECT_TRUE(found_test1);

    // Begin another transaction
    EXPECT_TRUE(db.begin_transaction());

    // Insert more data
    EXPECT_TRUE(db.execute_update(
        "INSERT INTO test (name, value) VALUES (?, ?)",
        {"test2", "43"}
    ));

    // Rollback transaction
    EXPECT_TRUE(db.rollback_transaction());

    // Check data was not committed
    result = db.execute("SELECT * FROM test");
    bool found_test2 = false;
    found_test1 = false;

    while (result.next()) {
        std::string name = result.get_string("name");
        int value = result.get_int("value");

        if (name == "test1" && value == 42) {
            found_test1 = true;
        } else if (name == "test2" && value == 43) {
            found_test2 = true;
        }
    }

    // test1 should be found, test2 should not be found after rollback
    EXPECT_TRUE(found_test1);
    EXPECT_FALSE(found_test2);
}

TEST_F(DatabaseTest, AsyncExecute) {
    Database db(test_db_path_);
    ASSERT_TRUE(db.initialize());

    // Create a test table
    ASSERT_TRUE(db.execute_update(
        "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)"
    ));

    // Insert data asynchronously
    auto future = db.execute_update_async(
        "INSERT INTO test (name, value) VALUES (?, ?)",
        {"test1", "42"}
    );

    // Wait for the operation to complete
    EXPECT_TRUE(future.get());

    // Query data asynchronously
    auto result_future = db.execute_async("SELECT * FROM test");

    // Wait for the query to complete
    auto result = result_future.get();

    // Check the result
    bool found_test1 = false;

    while (result.next()) {
        std::string name = result.get_string("name");
        int value = result.get_int("value");

        if (name == "test1" && value == 42) {
            found_test1 = true;
        }
    }

    EXPECT_TRUE(found_test1);
}

// Main function moved to storage_manager_test.cpp
