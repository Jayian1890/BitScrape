#include <gtest/gtest.h>
#include <bitscrape/storage/database.hpp>

#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

using namespace bitscrape::storage;

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary database file
        test_db_path_ = "test_database.db";
        
        // Remove the file if it exists
        std::filesystem::remove(test_db_path_);
    }
    
    void TearDown() override {
        // Remove the test database file
        std::filesystem::remove(test_db_path_);
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
    
    // Create a test table
    EXPECT_TRUE(db.execute_update(
        "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)"
    ));
    
    // Insert data
    EXPECT_TRUE(db.execute_update(
        "INSERT INTO test (name, value) VALUES (?, ?)",
        {"test1", "42"}
    ));
    
    // Check last insert rowid
    EXPECT_EQ(db.last_insert_rowid(), 1);
    
    // Check changes
    EXPECT_EQ(db.changes(), 1);
}

TEST_F(DatabaseTest, ExecuteQuery) {
    Database db(test_db_path_);
    ASSERT_TRUE(db.initialize());
    
    // Create a test table
    ASSERT_TRUE(db.execute_update(
        "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT, value INTEGER)"
    ));
    
    // Insert data
    ASSERT_TRUE(db.execute_update(
        "INSERT INTO test (name, value) VALUES (?, ?)",
        {"test1", "42"}
    ));
    
    ASSERT_TRUE(db.execute_update(
        "INSERT INTO test (name, value) VALUES (?, ?)",
        {"test2", "43"}
    ));
    
    // Query data
    auto result = db.execute("SELECT * FROM test ORDER BY id");
    
    // Check column count
    EXPECT_EQ(result.column_count(), 3);
    
    // Check column names
    EXPECT_EQ(result.column_name(0), "id");
    EXPECT_EQ(result.column_name(1), "name");
    EXPECT_EQ(result.column_name(2), "value");
    
    // Check column indices
    EXPECT_EQ(result.column_index("id"), 0);
    EXPECT_EQ(result.column_index("name"), 1);
    EXPECT_EQ(result.column_index("value"), 2);
    
    // Check first row
    EXPECT_TRUE(result.next());
    EXPECT_EQ(result.get_int("id"), 1);
    EXPECT_EQ(result.get_string("name"), "test1");
    EXPECT_EQ(result.get_int("value"), 42);
    
    // Check second row
    EXPECT_TRUE(result.next());
    EXPECT_EQ(result.get_int("id"), 2);
    EXPECT_EQ(result.get_string("name"), "test2");
    EXPECT_EQ(result.get_int("value"), 43);
    
    // Check no more rows
    EXPECT_FALSE(result.next());
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
    auto result = db.execute("SELECT COUNT(*) AS count FROM test");
    EXPECT_TRUE(result.next());
    EXPECT_EQ(result.get_int("count"), 1);
    
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
    result = db.execute("SELECT COUNT(*) AS count FROM test");
    EXPECT_TRUE(result.next());
    EXPECT_EQ(result.get_int("count"), 1);
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
    EXPECT_TRUE(result.next());
    EXPECT_EQ(result.get_int("id"), 1);
    EXPECT_EQ(result.get_string("name"), "test1");
    EXPECT_EQ(result.get_int("value"), 42);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
