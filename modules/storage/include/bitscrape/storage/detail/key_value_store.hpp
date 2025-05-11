#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <fstream>
#include <optional>
#include <filesystem>
#include <functional>
#include <future>

namespace bitscrape::storage::detail {

/**
 * @brief Custom key-value store for efficient data storage
 *
 * This class provides a thread-safe key-value store with disk persistence.
 * It supports transactions, indexing, and efficient querying.
 */
class KeyValueStore {
public:
    /**
     * @brief Value type for the key-value store
     */
    using Value = std::vector<uint8_t>;

    /**
     * @brief Create a key-value store
     *
     * @param path Path to the store file. If empty, a default path will be used.
     * @param persistent Whether to persist the store to disk. Always true for disk-based storage.
     */
    explicit KeyValueStore(const std::string& path, bool persistent = true);

    /**
     * @brief Destructor
     */
    ~KeyValueStore();

    /**
     * @brief Initialize the store
     *
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Initialize the store asynchronously
     *
     * @return Future with the result of the initialization
     */
    std::future<bool> initialize_async();

    /**
     * @brief Close the store
     *
     * @return true if successful, false otherwise
     */
    bool close();

    /**
     * @brief Close the store asynchronously
     *
     * @return Future with the result of the close operation
     */
    std::future<bool> close_async();

    /**
     * @brief Put a value in the store
     *
     * @param key Key
     * @param value Value
     * @return true if successful, false otherwise
     */
    bool put(const std::string& key, const Value& value);

    /**
     * @brief Put a value in the store asynchronously
     *
     * @param key Key
     * @param value Value
     * @return Future with the result of the operation
     */
    std::future<bool> put_async(const std::string& key, const Value& value);

    /**
     * @brief Get a value from the store
     *
     * @param key Key
     * @return Value if found, empty optional otherwise
     */
    std::optional<Value> get(const std::string& key);

    /**
     * @brief Get a value from the store asynchronously
     *
     * @param key Key
     * @return Future with the value if found, empty optional otherwise
     */
    std::future<std::optional<Value>> get_async(const std::string& key);

    /**
     * @brief Delete a value from the store
     *
     * @param key Key
     * @return true if successful, false otherwise
     */
    bool remove(const std::string& key);

    /**
     * @brief Delete a value from the store asynchronously
     *
     * @param key Key
     * @return Future with the result of the operation
     */
    std::future<bool> remove_async(const std::string& key);

    /**
     * @brief Check if a key exists in the store
     *
     * @param key Key
     * @return true if the key exists, false otherwise
     */
    bool exists(const std::string& key);

    /**
     * @brief Check if a key exists in the store asynchronously
     *
     * @param key Key
     * @return Future with true if the key exists, false otherwise
     */
    std::future<bool> exists_async(const std::string& key);

    /**
     * @brief Get all keys in the store
     *
     * @return Vector of keys
     */
    std::vector<std::string> keys();

    /**
     * @brief Get all keys in the store asynchronously
     *
     * @return Future with a vector of keys
     */
    std::future<std::vector<std::string>> keys_async();

    /**
     * @brief Get all keys with a prefix
     *
     * @param prefix Key prefix
     * @return Vector of keys
     */
    std::vector<std::string> keys_with_prefix(const std::string& prefix);

    /**
     * @brief Get all keys with a prefix asynchronously
     *
     * @param prefix Key prefix
     * @return Future with a vector of keys
     */
    std::future<std::vector<std::string>> keys_with_prefix_async(const std::string& prefix);

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
     * @brief Get the path to the store file
     *
     * @return Path to the store file
     */
    std::string path() const;

    /**
     * @brief Check if the store is initialized
     *
     * @return true if initialized, false otherwise
     */
    bool is_initialized() const;

    /**
     * @brief Check if the store is persistent
     *
     * @return true if persistent, false otherwise
     */
    bool is_persistent() const;

    /**
     * @brief Get the number of entries in the store
     *
     * @return Number of entries
     */
    size_t size() const;

    /**
     * @brief Get the size of the store file
     *
     * @return Size of the store file in bytes
     */
    size_t file_size() const;

    /**
     * @brief Create an index on a key prefix
     *
     * @param prefix Key prefix
     * @return true if successful, false otherwise
     */
    bool create_index(const std::string& prefix);

    /**
     * @brief Create an index on a key prefix asynchronously
     *
     * @param prefix Key prefix
     * @return Future with the result of the operation
     */
    std::future<bool> create_index_async(const std::string& prefix);

    /**
     * @brief Drop an index on a key prefix
     *
     * @param prefix Key prefix
     * @return true if successful, false otherwise
     */
    bool drop_index(const std::string& prefix);

    /**
     * @brief Drop an index on a key prefix asynchronously
     *
     * @param prefix Key prefix
     * @return Future with the result of the operation
     */
    std::future<bool> drop_index_async(const std::string& prefix);

    /**
     * @brief Check if an index exists on a key prefix
     *
     * @param prefix Key prefix
     * @return true if the index exists, false otherwise
     */
    bool index_exists(const std::string& prefix);

    /**
     * @brief Check if an index exists on a key prefix asynchronously
     *
     * @param prefix Key prefix
     * @return Future with true if the index exists, false otherwise
     */
    std::future<bool> index_exists_async(const std::string& prefix);

    /**
     * @brief Flush the store to disk
     *
     * @return true if successful, false otherwise
     */
    bool flush();

    /**
     * @brief Flush the store to disk asynchronously
     *
     * @return Future with the result of the operation
     */
    std::future<bool> flush_async();

    /**
     * @brief Compact the store
     *
     * @return true if successful, false otherwise
     */
    bool compact();

    /**
     * @brief Compact the store asynchronously
     *
     * @return Future with the result of the operation
     */
    std::future<bool> compact_async();

    /**
     * @brief Iterate over all key-value pairs
     *
     * @param callback Callback function to call for each key-value pair
     * @return true if successful, false otherwise
     */
    bool iterate(const std::function<bool(const std::string&, const Value&)>& callback);

    /**
     * @brief Iterate over all key-value pairs asynchronously
     *
     * @param callback Callback function to call for each key-value pair
     * @return Future with the result of the operation
     */
    std::future<bool> iterate_async(const std::function<bool(const std::string&, const Value&)>& callback);

    /**
     * @brief Iterate over all key-value pairs with a prefix
     *
     * @param prefix Key prefix
     * @param callback Callback function to call for each key-value pair
     * @return true if successful, false otherwise
     */
    bool iterate_with_prefix(const std::string& prefix, const std::function<bool(const std::string&, const Value&)>& callback);

    /**
     * @brief Iterate over all key-value pairs with a prefix asynchronously
     *
     * @param prefix Key prefix
     * @param callback Callback function to call for each key-value pair
     * @return Future with the result of the operation
     */
    std::future<bool> iterate_with_prefix_async(const std::string& prefix, const std::function<bool(const std::string&, const Value&)>& callback);

private:
    // Store path
    std::string path_;

    // In-memory store
    std::unordered_map<std::string, Value> store_;

    // Transaction store
    std::unordered_map<std::string, Value> transaction_store_;

    // Deleted keys in transaction
    std::vector<std::string> transaction_deleted_keys_;

    // Indexes
    std::unordered_map<std::string, std::vector<std::string>> indexes_;

    // Mutex for thread safety
    mutable std::shared_mutex mutex_;

    // Transaction mutex
    mutable std::mutex transaction_mutex_;

    // Initialization flag
    bool initialized_;

    // Transaction flag
    bool in_transaction_;

    // Persistence flag
    bool is_persistent_;

    // Helper methods
    bool load_from_disk();
    bool save_to_disk();
    bool update_index(const std::string& key, bool is_delete = false);
    std::vector<std::string> get_prefixes(const std::string& key);
};

} // namespace bitscrape::storage::detail
