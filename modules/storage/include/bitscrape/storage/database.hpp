#pragma once

#include <bitscrape/storage/detail/database_result.hpp>

#include <string>
#include <memory>
#include <mutex>
#include <future>
#include <vector>
#include <unordered_map>
#include <functional>

namespace bitscrape::storage {

// Forward declaration for KeyValueStore
namespace detail {
    class KeyValueStore;
}

/**
 * @brief Database connection and query execution
 *
 * The Database class provides a thread-safe interface to the storage backend.
 * It handles connection management, query execution, and transaction support.
 */
class Database {
public:
    /**
     * @brief Result of a database query
     */
    class Result : public detail::DatabaseResult {
    public:
        /**
         * @brief Create an empty result
         */
        Result();

        /**
         * @brief Create a result from raw data
         *
         * @param data Result data
         */
        explicit Result(std::vector<std::unordered_map<std::string, std::vector<uint8_t>>> data);

        /**
         * @brief Destructor
         */
        ~Result();

        /**
         * @brief Move constructor
         */
        Result(Result&& other) noexcept;

        /**
         * @brief Move assignment operator
         */
        Result& operator=(Result&& other) noexcept;

        /**
         * @brief Check if the result has rows
         *
         * @return true if the result has rows, false otherwise
         */
        bool has_rows() const;

        /**
         * @brief Get the number of columns in the result
         *
         * @return Number of columns
         */
        int column_count() const;

        /**
         * @brief Get the name of a column
         *
         * @param index Column index
         * @return Column name
         */
        std::string column_name(int index) const;

        /**
         * @brief Get the name of a column (renamed from column_name to avoid confusion)
         *
         * @param index Column index
         * @return Column name
         */
        std::string get_column_name(int index) const;

        /**
         * @brief Get the index of a column by name
         *
         * @param name Column name
         * @return Column index, or -1 if not found
         */
        int column_index(const std::string& name) const;

        /**
         * @brief Move to the next row
         *
         * @return true if there is a next row, false otherwise
         */
        bool next();

        /**
         * @brief Get an integer value from the current row
         *
         * @param index Column index
         * @return Integer value
         */
        int get_int(int index) const;

        /**
         * @brief Get an integer value from the current row
         *
         * @param name Column name
         * @return Integer value
         */
        int get_int(const std::string& name) const;

        /**
         * @brief Get a 64-bit integer value from the current row
         *
         * @param index Column index
         * @return 64-bit integer value
         */
        int64_t get_int64(int index) const;

        /**
         * @brief Get a 64-bit integer value from the current row
         *
         * @param name Column name
         * @return 64-bit integer value
         */
        int64_t get_int64(const std::string& name) const;

        /**
         * @brief Get a double value from the current row
         *
         * @param index Column index
         * @return Double value
         */
        double get_double(int index) const;

        /**
         * @brief Get a double value from the current row
         *
         * @param name Column name
         * @return Double value
         */
        double get_double(const std::string& name) const;

        /**
         * @brief Get a string value from the current row
         *
         * @param index Column index
         * @return String value
         */
        std::string get_string(int index) const;

        /**
         * @brief Get a string value from the current row
         *
         * @param name Column name
         * @return String value
         */
        std::string get_string(const std::string& name) const;

        /**
         * @brief Get a blob value from the current row
         *
         * @param index Column index
         * @return Blob value as a vector of bytes
         */
        std::vector<uint8_t> get_blob(int index) const;

        /**
         * @brief Get a blob value from the current row
         *
         * @param name Column name
         * @return Blob value as a vector of bytes
         */
        std::vector<uint8_t> get_blob(const std::string& name) const;

        /**
         * @brief Check if a column is null
         *
         * @param index Column index
         * @return true if the column is null, false otherwise
         */
        bool is_null(int index) const;

        /**
         * @brief Check if a column is null
         *
         * @param name Column name
         * @return true if the column is null, false otherwise
         */
        bool is_null(const std::string& name) const;

    private:
        std::vector<std::unordered_map<std::string, std::vector<uint8_t>>> data_;
        size_t current_row_;
        bool has_rows_;

        // Disable copy
        Result(const Result&) = delete;
        Result& operator=(const Result&) = delete;
    };

    /**
     * @brief Create a database connection
     *
     * @param path Path to the database file
     */
    explicit Database(const std::string& path);

    /**
     * @brief Destructor
     */
    ~Database();

    /**
     * @brief Initialize the database
     *
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Initialize the database asynchronously
     *
     * @return Future with the result of the initialization
     */
    std::future<bool> initialize_async();

    /**
     * @brief Close the database connection
     *
     * @return true if successful, false otherwise
     */
    bool close();

    /**
     * @brief Close the database connection asynchronously
     *
     * @return Future with the result of the close operation
     */
    std::future<bool> close_async();

    /**
     * @brief Execute a SQL query
     *
     * @param sql SQL query
     * @param params Query parameters
     * @return Result of the query
     */
    Result execute(const std::string& sql, const std::vector<std::string>& params = {});

    /**
     * @brief Execute a SQL query asynchronously
     *
     * @param sql SQL query
     * @param params Query parameters
     * @return Future with the result of the query
     */
    std::future<Result> execute_async(const std::string& sql, const std::vector<std::string>& params = {});

    /**
     * @brief Execute a SQL query without returning a result
     *
     * @param sql SQL query
     * @param params Query parameters
     * @return true if successful, false otherwise
     */
    bool execute_update(const std::string& sql, const std::vector<std::string>& params = {});

    /**
     * @brief Execute a SQL query without returning a result asynchronously
     *
     * @param sql SQL query
     * @param params Query parameters
     * @return Future with the result of the query
     */
    std::future<bool> execute_update_async(const std::string& sql, const std::vector<std::string>& params = {});

    /**
     * @brief Begin a transaction
     *
     * @return true if successful, false otherwise
     */
    bool begin_transaction();

    /**
     * @brief Begin a transaction asynchronously
     *
     * @return Future with the result of the operation
     */
    std::future<bool> begin_transaction_async();

    /**
     * @brief Commit a transaction
     *
     * @return true if successful, false otherwise
     */
    bool commit_transaction();

    /**
     * @brief Commit a transaction asynchronously
     *
     * @return Future with the result of the operation
     */
    std::future<bool> commit_transaction_async();

    /**
     * @brief Rollback a transaction
     *
     * @return true if successful, false otherwise
     */
    bool rollback_transaction();

    /**
     * @brief Rollback a transaction asynchronously
     *
     * @return Future with the result of the operation
     */
    std::future<bool> rollback_transaction_async();

    /**
     * @brief Get the last inserted row ID
     *
     * @return Last inserted row ID
     */
    int64_t last_insert_rowid() const;

    /**
     * @brief Get the number of rows affected by the last query
     *
     * @return Number of rows affected
     */
    int changes() const;

    /**
     * @brief Get the database path
     *
     * @return Database path
     */
    std::string path() const;

    /**
     * @brief Check if the database is initialized
     *
     * @return true if initialized, false otherwise
     */
    bool is_initialized() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Disable copy
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
};

} // namespace bitscrape::storage
