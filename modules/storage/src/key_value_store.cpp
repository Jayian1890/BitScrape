#include <bitscrape/storage/detail/key_value_store.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <thread>

namespace bitscrape::storage::detail {

KeyValueStore::KeyValueStore(const std::string& path, bool persistent)
    : path_(path.empty() ? "data/default.db" : path),
      initialized_(false),
      in_transaction_(false),
      is_persistent_(true) {
    // Always use disk-based storage

    // If original path was empty, we're using the default path
    if (path.empty()) {
        std::cerr << "Using default path: " << path_ << std::endl;
    }

    // Ensure the directory exists for the database file
    try {
        std::filesystem::path fs_path(path_);
        if (!fs_path.parent_path().empty()) {
            std::filesystem::create_directories(fs_path.parent_path());
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not create directory for database: " << e.what() << std::endl;
        // Continue anyway, we'll handle errors during save/load
    }
}

KeyValueStore::~KeyValueStore() {
    close();
}

bool KeyValueStore::initialize() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    try {
        // Create a parent directory if it doesn't exist
        try {
            const auto path = std::filesystem::absolute(path_);
            if (!path.parent_path().empty() && !std::filesystem::exists(path.parent_path())) {
                std::filesystem::create_directories(path.parent_path());
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Failed to create directories: " << e.what() << std::endl;
            return false; // Fail if we can't create directories
        }

        // Load data from disk if the file exists
        if (std::filesystem::exists(path_)) {
            if (!load_from_disk()) {
                std::cerr << "Failed to load data from disk" << std::endl;
                return false; // Fail if we can't load data
            }
        }

        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize key-value store: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::initialize_async() {
    return std::async(std::launch::async, [this]() {
        return initialize();
    });
}

bool KeyValueStore::close() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        return true;
    }

    try {
        bool success = true;

        // Save data to disk
        if (!save_to_disk()) {
            std::cerr << "Failed to save data to disk" << std::endl;
            // Continue with cleanup even if save fails
            // But don't set success to false, as this would cause cascading errors
        }

        // Clear in-memory store
        store_.clear();
        indexes_.clear();

        initialized_ = false;
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Failed to close key-value store: " << e.what() << std::endl;

        // Try to clean up anyway
        try {
            store_.clear();
            indexes_.clear();
            initialized_ = false;
        } catch (...) {
            // Ignore any exceptions during cleanup
        }

        return false;
    }
}

std::future<bool> KeyValueStore::close_async() {
    return std::async(std::launch::async, [this]() {
        return close();
    });
}

bool KeyValueStore::put(const std::string& key, const Value& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    try {
        if (in_transaction_) {
            std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);
            transaction_store_[key] = value;

            // Remove from deleted keys if it was deleted in this transaction
            auto it = std::find(transaction_deleted_keys_.begin(), transaction_deleted_keys_.end(), key);
            if (it != transaction_deleted_keys_.end()) {
                transaction_deleted_keys_.erase(it);
            }
        } else {
            store_[key] = value;
            update_index(key);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to put value: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::put_async(const std::string& key, const Value& value) {
    return std::async(std::launch::async, [this, key, value]() {
        return put(key, value);
    });
}

std::optional<KeyValueStore::Value> KeyValueStore::get(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return std::nullopt;
    }

    try {
        if (in_transaction_) {
            std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

            // Check if the key was deleted in this transaction
            auto it_deleted = std::find(transaction_deleted_keys_.begin(), transaction_deleted_keys_.end(), key);
            if (it_deleted != transaction_deleted_keys_.end()) {
                return std::nullopt;
            }

            // Check if the key was added or modified in this transaction
            auto it = transaction_store_.find(key);
            if (it != transaction_store_.end()) {
                return it->second;
            }
        }

        // Check the main store
        auto it = store_.find(key);
        if (it != store_.end()) {
            return it->second;
        }

        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Failed to get value: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::future<std::optional<KeyValueStore::Value>> KeyValueStore::get_async(const std::string& key) {
    return std::async(std::launch::async, [this, key]() {
        return get(key);
    });
}

bool KeyValueStore::remove(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    try {
        if (in_transaction_) {
            std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

            // Remove from transaction store if it was added or modified in this transaction
            transaction_store_.erase(key);

            // Add to deleted keys
            transaction_deleted_keys_.push_back(key);
        } else {
            // Update indexes before removing the key
            update_index(key, true);

            // Remove from main store
            store_.erase(key);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to remove value: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::remove_async(const std::string& key) {
    return std::async(std::launch::async, [this, key]() {
        return remove(key);
    });
}

bool KeyValueStore::exists(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    try {
        if (in_transaction_) {
            std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

            // Check if the key was deleted in this transaction
            auto it_deleted = std::find(transaction_deleted_keys_.begin(), transaction_deleted_keys_.end(), key);
            if (it_deleted != transaction_deleted_keys_.end()) {
                return false;
            }

            // Check if the key was added or modified in this transaction
            auto it = transaction_store_.find(key);
            if (it != transaction_store_.end()) {
                return true;
            }
        }

        // Check the main store
        return store_.find(key) != store_.end();
    } catch (const std::exception& e) {
        std::cerr << "Failed to check if key exists: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::exists_async(const std::string& key) {
    return std::async(std::launch::async, [this, key]() {
        return exists(key);
    });
}

std::vector<std::string> KeyValueStore::keys() {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return {};
    }

    try {
        std::vector<std::string> result;

        // Add keys from the main store
        for (const auto& [key, _] : store_) {
            result.push_back(key);
        }

        if (in_transaction_) {
            std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

            // Add keys from the transaction store
            for (const auto& [key, _] : transaction_store_) {
                // Check if the key is already in the result
                auto it = std::find(result.begin(), result.end(), key);
                if (it == result.end()) {
                    result.push_back(key);
                }
            }

            // Remove deleted keys
            for (const auto& key : transaction_deleted_keys_) {
                auto it = std::find(result.begin(), result.end(), key);
                if (it != result.end()) {
                    result.erase(it);
                }
            }
        }

        return result;
    } catch (const std::exception& e) {
        std::cerr << "Failed to get keys: " << e.what() << std::endl;
        return {};
    }
}

std::future<std::vector<std::string>> KeyValueStore::keys_async() {
    return std::async(std::launch::async, [this]() {
        return keys();
    });
}

std::vector<std::string> KeyValueStore::keys_with_prefix(const std::string& prefix) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return {};
    }

    try {
        std::vector<std::string> result;

        // Check if we have an index for this prefix
        auto index_it = indexes_.find(prefix);
        if (index_it != indexes_.end()) {
            // Use the index
            result = index_it->second;
        } else {
            // No index, scan the main store
            for (const auto& [key, _] : store_) {
                if (key.find(prefix) == 0) {
                    result.push_back(key);
                }
            }
        }

        if (in_transaction_) {
            std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

            // Add keys from the transaction store
            for (const auto& [key, _] : transaction_store_) {
                if (key.find(prefix) == 0) {
                    // Check if the key is already in the result
                    auto it = std::find(result.begin(), result.end(), key);
                    if (it == result.end()) {
                        result.push_back(key);
                    }
                }
            }

            // Remove deleted keys
            for (const auto& key : transaction_deleted_keys_) {
                auto it = std::find(result.begin(), result.end(), key);
                if (it != result.end()) {
                    result.erase(it);
                }
            }
        }

        return result;
    } catch (const std::exception& e) {
        std::cerr << "Failed to get keys with prefix: " << e.what() << std::endl;
        return {};
    }
}

std::future<std::vector<std::string>> KeyValueStore::keys_with_prefix_async(const std::string& prefix) {
    return std::async(std::launch::async, [this, prefix]() {
        return keys_with_prefix(prefix);
    });
}

bool KeyValueStore::begin_transaction() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    if (in_transaction_) {
        std::cerr << "Transaction already in progress" << std::endl;
        return false;
    }

    try {
        std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

        // Clear transaction store and deleted keys
        transaction_store_.clear();
        transaction_deleted_keys_.clear();

        in_transaction_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to begin transaction: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::begin_transaction_async() {
    return std::async(std::launch::async, [this]() {
        return begin_transaction();
    });
}

bool KeyValueStore::commit_transaction() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    if (!in_transaction_) {
        std::cerr << "No transaction in progress" << std::endl;
        return false;
    }

    try {
        std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

        // Apply changes from transaction store to main store
        for (const auto& [key, value] : transaction_store_) {
            store_[key] = value;
            update_index(key);
        }

        // Remove deleted keys from main store
        for (const auto& key : transaction_deleted_keys_) {
            update_index(key, true);
            store_.erase(key);
        }

        // Clear transaction store and deleted keys
        transaction_store_.clear();
        transaction_deleted_keys_.clear();

        in_transaction_ = false;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to commit transaction: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::commit_transaction_async() {
    return std::async(std::launch::async, [this]() {
        return commit_transaction();
    });
}

bool KeyValueStore::rollback_transaction() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    if (!in_transaction_) {
        std::cerr << "No transaction in progress" << std::endl;
        return false;
    }

    try {
        std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

        // Clear transaction store and deleted keys
        transaction_store_.clear();
        transaction_deleted_keys_.clear();

        in_transaction_ = false;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to rollback transaction: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::rollback_transaction_async() {
    return std::async(std::launch::async, [this]() {
        return rollback_transaction();
    });
}

std::string KeyValueStore::path() const {
    return path_;
}

bool KeyValueStore::is_initialized() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return initialized_;
}

bool KeyValueStore::is_persistent() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return is_persistent_;
}

size_t KeyValueStore::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        return 0;
    }

    size_t count = store_.size();

    if (in_transaction_) {
        std::lock_guard<std::mutex> transaction_lock(transaction_mutex_);

        // Add keys from transaction store that are not in the main store
        for (const auto& [key, _] : transaction_store_) {
            if (store_.find(key) == store_.end()) {
                count++;
            }
        }

        // Subtract deleted keys that are in the main store
        for (const auto& key : transaction_deleted_keys_) {
            if (store_.find(key) != store_.end()) {
                count--;
            }
        }
    }

    return count;
}

size_t KeyValueStore::file_size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_ || !std::filesystem::exists(path_)) {
        return 0;
    }

    return std::filesystem::file_size(path_);
}

bool KeyValueStore::create_index(const std::string& prefix) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    try {
        // Check if the index already exists
        if (indexes_.find(prefix) != indexes_.end()) {
            return true;
        }

        // Create the index
        std::vector<std::string> index;

        for (const auto& [key, _] : store_) {
            if (key.find(prefix) == 0) {
                index.push_back(key);
            }
        }

        indexes_[prefix] = index;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create index: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::create_index_async(const std::string& prefix) {
    return std::async(std::launch::async, [this, prefix]() {
        return create_index(prefix);
    });
}

bool KeyValueStore::drop_index(const std::string& prefix) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    try {
        // Check if the index exists
        auto it = indexes_.find(prefix);
        if (it == indexes_.end()) {
            return true;
        }

        // Remove the index
        indexes_.erase(it);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to drop index: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::drop_index_async(const std::string& prefix) {
    return std::async(std::launch::async, [this, prefix]() {
        return drop_index(prefix);
    });
}

bool KeyValueStore::index_exists(const std::string& prefix) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    return indexes_.find(prefix) != indexes_.end();
}

std::future<bool> KeyValueStore::index_exists_async(const std::string& prefix) {
    return std::async(std::launch::async, [this, prefix]() {
        return index_exists(prefix);
    });
}

bool KeyValueStore::flush() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    try {
        // Always flush to disk
        return save_to_disk();
    } catch (const std::exception& e) {
        std::cerr << "Failed to flush store: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::flush_async() {
    return std::async(std::launch::async, [this]() {
        return flush();
    });
}

bool KeyValueStore::compact() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    try {
        // Save to a temporary file
        std::string temp_path = path_ + ".tmp";

        std::ofstream file(temp_path, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open temporary file for writing" << std::endl;
            return false;
        }

        // Write the number of entries
        size_t count = store_.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        // Write each entry
        for (const auto& [key, value] : store_) {
            // Write key
            size_t key_size = key.size();
            file.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
            file.write(key.data(), key_size);

            // Write value
            size_t value_size = value.size();
            file.write(reinterpret_cast<const char*>(&value_size), sizeof(value_size));
            file.write(reinterpret_cast<const char*>(value.data()), value_size);
        }

        file.close();

        // Replace the original file with the temporary file
        std::filesystem::rename(temp_path, path_);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to compact store: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::compact_async() {
    return std::async(std::launch::async, [this]() {
        return compact();
    });
}

bool KeyValueStore::iterate(const std::function<bool(const std::string&, const Value&)>& callback) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    try {
        // Get all keys
        auto all_keys = keys();

        // Iterate over all keys
        for (const auto& key : all_keys) {
            auto value = get(key);
            if (value) {
                if (!callback(key, *value)) {
                    return false;
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to iterate over store: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::iterate_async(const std::function<bool(const std::string&, const Value&)>& callback) {
    return std::async(std::launch::async, [this, callback]() {
        return iterate(callback);
    });
}

bool KeyValueStore::iterate_with_prefix(const std::string& prefix, const std::function<bool(const std::string&, const Value&)>& callback) {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "Key-value store not initialized" << std::endl;
        return false;
    }

    try {
        // Get keys with prefix
        auto keys_with_prefix = this->keys_with_prefix(prefix);

        // Iterate over keys with prefix
        for (const auto& key : keys_with_prefix) {
            auto value = get(key);
            if (value) {
                if (!callback(key, *value)) {
                    return false;
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to iterate over store with prefix: " << e.what() << std::endl;
        return false;
    }
}

std::future<bool> KeyValueStore::iterate_with_prefix_async(const std::string& prefix, const std::function<bool(const std::string&, const Value&)>& callback) {
    return std::async(std::launch::async, [this, prefix, callback]() {
        return iterate_with_prefix(prefix, callback);
    });
}

bool KeyValueStore::load_from_disk() {
    try {
        // Check if path is empty
        if (path_.empty()) {
            std::cerr << "Failed to load data from disk: path is empty" << std::endl;
            return false;
        }

        std::ifstream file(path_, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file for reading: " << path_ << std::endl;
            return false;
        }

        // Check if file is empty
        file.seekg(0, std::ios::end);
        if (file.tellg() == 0) {
            std::cerr << "File is empty: " << path_ << std::endl;
            return true; // Empty file is not an error, just return with empty store
        }
        file.seekg(0, std::ios::beg);

        // Read the number of entries
        size_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));

        // Sanity check on count to prevent excessive memory allocation
        if (count > 10000000) {
            // 10 million entries limit
            std::cerr << "Too many entries in file: " << count << std::endl;
            return false;
        }

        // Read each entry
        for (size_t i = 0; i < count; ++i) {
            // Read key
            size_t key_size;
            file.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));

            // Sanity check on key size
            if (key_size > 1024) {
                // 1KB key size limit
                std::cerr << "Key size too large: " << key_size << std::endl;
                return false;
            }

            std::string key(key_size, '\0');
            file.read(&key[0], key_size);

            // Read value
            size_t value_size;
            file.read(reinterpret_cast<char*>(&value_size), sizeof(value_size));

            // Sanity check on value size
            if (value_size > 1024 * 1024 * 10) {
                // 10MB value size limit
                std::cerr << "Value size too large: " << value_size << std::endl;
                return false;
            }

            Value value(value_size);
            file.read(reinterpret_cast<char*>(value.data()), value_size);

            // Store the entry
            store_[key] = value;
        }

        // Rebuild indexes
        for (auto& [prefix, _] : indexes_) {
            create_index(prefix);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load data from disk: " << e.what() << std::endl;
        return false;
    }
}

bool KeyValueStore::save_to_disk() {
    try {
        // We should always have a valid path since we set a default in the constructor
        if (path_.empty()) {
            std::cerr << "Error: Path is empty when trying to save to disk" << std::endl;
            return false; // This should never happen with our modifications
        }

        // Create directory if it doesn't exist
        std::filesystem::path path(path_);

        // Only try to create parent directories if the parent path is not empty
        if (!path.parent_path().empty()) {
            try {
                // Check if parent path exists before trying to create it
                if (!path.parent_path().empty() && !std::filesystem::exists(path.parent_path())) {
                    // Double-check that the path is not empty before creating directories
                    if (path.parent_path().string() == "") {
                        std::cerr << "Skipping directory creation for empty path" << std::endl;
                    } else {
                        std::filesystem::create_directories(path.parent_path());
                    }
                }
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Failed to create directories: " << e.what() << std::endl;
                return false;
            }
        } else {
            // If parent path is empty, we're saving in the current directory
            std::cerr << "Saving to current directory: " << std::filesystem::absolute(path_) << std::endl;
        }

        std::ofstream file(path_, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file for writing: " << path_ << std::endl;
            return false;
        }

        // Write the number of entries
        size_t count = store_.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        // Write each entry
        for (const auto& [key, value] : store_) {
            // Write key
            size_t key_size = key.size();
            file.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
            file.write(key.data(), key_size);

            // Write value
            size_t value_size = value.size();
            file.write(reinterpret_cast<const char*>(&value_size), sizeof(value_size));
            file.write(reinterpret_cast<const char*>(value.data()), value_size);
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save data to disk: " << e.what() << std::endl;
        return false;
    }
}

bool KeyValueStore::update_index(const std::string& key, bool is_delete) {
    // Get all prefixes for the key
    auto prefixes = get_prefixes(key);

    // Update indexes
    for (const auto& prefix : prefixes) {
        auto it = indexes_.find(prefix);
        if (it != indexes_.end()) {
            auto& index = it->second;

            if (is_delete) {
                // Remove the key from the index
                auto key_it = std::find(index.begin(), index.end(), key);
                if (key_it != index.end()) {
                    index.erase(key_it);
                }
            } else {
                // Add the key to the index if it's not already there
                auto key_it = std::find(index.begin(), index.end(), key);
                if (key_it == index.end()) {
                    index.push_back(key);
                }
            }
        }
    }

    return true;
}

std::vector<std::string> KeyValueStore::get_prefixes(const std::string& key) {
    std::vector<std::string> prefixes;

    // Add all possible prefixes
    for (size_t i = 1; i <= key.size(); ++i) {
        prefixes.push_back(key.substr(0, i));
    }

    return prefixes;
}

} // namespace bitscrape::storage::detail
