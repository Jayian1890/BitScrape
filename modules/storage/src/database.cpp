#include <bitscrape/storage/database.hpp>
#include <bitscrape/storage/data_models.hpp>
#include <bitscrape/storage/detail/key_value_store.hpp>

#include <iostream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <algorithm>

namespace bitscrape::storage {

// Database::Result implementation

Database::Result::Result()
    : current_row_(0), has_rows_(false) {
}

Database::Result::Result(std::vector<std::unordered_map<std::string, std::vector<uint8_t>>> data)
    : data_(std::move(data)), current_row_(0), has_rows_(false) {
    // Check if there are rows
    has_rows_ = !data_.empty();
}

Database::Result::~Result() = default;

Database::Result::Result(Result&& other) noexcept
    : data_(std::move(other.data_)), current_row_(other.current_row_), has_rows_(other.has_rows_) {
    other.current_row_ = 0;
    other.has_rows_ = false;
}

Database::Result& Database::Result::operator=(Result&& other) noexcept {
    if (this != &other) {
        data_ = std::move(other.data_);
        current_row_ = other.current_row_;
        has_rows_ = other.has_rows_;

        other.current_row_ = 0;
        other.has_rows_ = false;
    }

    return *this;
}

bool Database::Result::has_rows() const {
    return has_rows_;
}

int Database::Result::column_count() const {
    if (data_.empty() || current_row_ >= data_.size()) {
        return 0;
    }

    return static_cast<int>(data_[current_row_].size());
}

std::string Database::Result::column_name(int index) const {
    // This function is renamed to get_column_name to avoid confusion
    return get_column_name(index);
}

std::string Database::Result::get_column_name(int index) const {
    if (data_.empty() || current_row_ >= data_.size()) {
        return "";
    }

    // Get all column names
    std::vector<std::string> column_names;
    for (const auto& [key, _] : data_[current_row_]) {
        column_names.push_back(key);
    }

    // Sort column names for consistent ordering
    std::sort(column_names.begin(), column_names.end());

    if (index >= 0 && index < static_cast<int>(column_names.size())) {
        return column_names[index];
    }

    return "";
}

int Database::Result::column_index(const std::string& name) const {
    // Renamed from column_name to avoid confusion
    if (data_.empty() || current_row_ >= data_.size()) {
        return -1;
    }

    // Check if the column exists
    if (data_[current_row_].find(name) == data_[current_row_].end()) {
        return -1;
    }

    // Get all column names
    std::vector<std::string> column_names;
    for (const auto& [key, _] : data_[current_row_]) {
        column_names.push_back(key);
    }

    // Sort column names for consistent ordering
    std::sort(column_names.begin(), column_names.end());

    // Find the index of the column
    auto it = std::find(column_names.begin(), column_names.end(), name);
    if (it != column_names.end()) {
        return static_cast<int>(std::distance(column_names.begin(), it));
    }

    return -1;
}

bool Database::Result::next() {
    if (data_.empty() || current_row_ >= data_.size()) {
        has_rows_ = false;
        return false;
    }

    current_row_++;
    has_rows_ = (current_row_ < data_.size());

    return has_rows_;
}

int Database::Result::get_int(int index) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return 0;
    }

    std::string col_name = get_column_name(index);
    if (col_name.empty()) {
        return 0;
    }

    return get_int(col_name);
}

int Database::Result::get_int(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return 0;
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return 0;
    }

    // Convert the binary data to an integer
    const auto& value = it->second;
    if (value.size() == sizeof(int)) {
        int result = 0;
        std::memcpy(&result, value.data(), sizeof(int));
        return result;
    } else if (!value.empty()) {
        // Try to parse as string
        try {
            std::string str(value.begin(), value.end());
            return std::stoi(str);
        } catch (const std::exception&) {
            // Ignore parsing errors
        }
    }

    return 0;
}

int64_t Database::Result::get_int64(int index) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return 0;
    }

    std::string col_name = get_column_name(index);
    if (col_name.empty()) {
        return 0;
    }

    return get_int64(col_name);
}

int64_t Database::Result::get_int64(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return 0;
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return 0;
    }

    // Convert the binary data to an int64_t
    const auto& value = it->second;
    if (value.size() == sizeof(int64_t)) {
        int64_t result = 0;
        std::memcpy(&result, value.data(), sizeof(int64_t));
        return result;
    } else if (!value.empty()) {
        // Try to parse as string
        try {
            std::string str(value.begin(), value.end());
            return std::stoll(str);
        } catch (const std::exception&) {
            // Ignore parsing errors
        }
    }

    return 0;
}

double Database::Result::get_double(int index) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return 0.0;
    }

    std::string col_name = get_column_name(index);
    if (col_name.empty()) {
        return 0.0;
    }

    return get_double(col_name);
}

double Database::Result::get_double(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return 0.0;
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return 0.0;
    }

    // Convert the binary data to a double
    const auto& value = it->second;
    if (value.size() == sizeof(double)) {
        double result = 0.0;
        std::memcpy(&result, value.data(), sizeof(double));
        return result;
    } else if (!value.empty()) {
        // Try to parse as string
        try {
            std::string str(value.begin(), value.end());
            return std::stod(str);
        } catch (const std::exception&) {
            // Ignore parsing errors
        }
    }

    return 0.0;
}

std::string Database::Result::get_string(int index) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return "";
    }

    std::string col_name = get_column_name(index);
    if (col_name.empty()) {
        return "";
    }

    return get_string(col_name);
}

std::string Database::Result::get_string(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return "";
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return "";
    }

    // Convert the binary data to a string
    const auto& value = it->second;
    if (!value.empty()) {
        return std::string(value.begin(), value.end());
    }

    return "";
}

std::vector<uint8_t> Database::Result::get_blob(int index) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return {};
    }

    std::string col_name = get_column_name(index);
    if (col_name.empty()) {
        return {};
    }

    return get_blob(col_name);
}

std::vector<uint8_t> Database::Result::get_blob(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return {};
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return {};
    }

    // Return the binary data directly
    return it->second;
}

bool Database::Result::is_null(int index) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return true;
    }

    std::string col_name = get_column_name(index);
    if (col_name.empty()) {
        return true;
    }

    return is_null(col_name);
}

bool Database::Result::is_null(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return true;
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return true;
    }

    // Consider empty vectors as null
    return it->second.empty();
}

// Database implementation

class Database::Impl {
public:
    Impl(const std::string& path, bool persistent)
        : path_(path.empty() ? "data/default.db" : path),
          store_(std::make_unique<detail::KeyValueStore>(path.empty() ? "data/default.db" : path, true)),
          initialized_(false) {
        // If original path was empty, we're using the default path
        if (path.empty()) {
            std::cerr << "Using default database path: " << path_ << std::endl;
        }
    }

    ~Impl() {
        close();
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (initialized_) {
            return true;
        }

        try {
            // Initialize the key-value store
            if (!store_->initialize()) {
                std::cerr << "Failed to initialize key-value store" << std::endl;
                return false;
            }

            // Create tables if they don't exist
            create_tables();

            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize database: " << e.what() << std::endl;
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
            bool success = true;

            // Close the key-value store
            if (!store_->close()) {
                std::cerr << "Failed to close key-value store" << std::endl;
                success = false;
                // Continue with cleanup even if close fails
            }

            initialized_ = false;
            return success;
        } catch (const std::exception& e) {
            std::cerr << "Failed to close database: " << e.what() << std::endl;

            // Try to clean up anyway
            try {
                initialized_ = false;
            } catch (...) {
                // Ignore any exceptions during cleanup
            }

            return false;
        }
    }

    std::future<bool> close_async() {
        return std::async(std::launch::async, [this]() {
            return close();
        });
    }

    // Helper method to create tables
    bool create_tables() {
        // Create indexes for common prefixes
        store_->create_index("nodes:");
        store_->create_index("infohashes:");
        store_->create_index("metadata:");
        store_->create_index("files:");
        store_->create_index("trackers:");
        store_->create_index("peers:");

        return true;
    }

    Database::Result execute(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Database not initialized" << std::endl;
            return Database::Result();
        }

        try {
            // Parse the SQL query to determine what to do
            std::string query = sql;

            // Replace parameters in the query
            for (size_t i = 0; i < params.size(); ++i) {
                std::string param_placeholder = "?";
                size_t pos = query.find(param_placeholder);
                if (pos != std::string::npos) {
                    query.replace(pos, param_placeholder.length(), params[i]);
                }
            }

            // Simple SQL parser to handle basic queries
            if (query.find("SELECT") == 0 || query.find("select") == 0) {
                return execute_select(query);
            } else if (query.find("INSERT") == 0 || query.find("insert") == 0) {
                execute_insert(query);
                return Database::Result();
            } else if (query.find("UPDATE") == 0 || query.find("update") == 0) {
                execute_update(query, {});
                return Database::Result();
            } else if (query.find("DELETE") == 0 || query.find("delete") == 0) {
                execute_delete(query);
                return Database::Result();
            } else if (query.find("PRAGMA") == 0 || query.find("pragma") == 0) {
                // Ignore PRAGMA statements
                return Database::Result();
            } else {
                std::cerr << "Unsupported SQL query: " << query << std::endl;
                return Database::Result();
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to execute query: " << e.what() << std::endl;
            return Database::Result();
        }
    }

    std::future<Database::Result> execute_async(const std::string& sql, const std::vector<std::string>& params = {}) {
        return std::async(std::launch::async, [this, sql, params]() {
            return execute(sql, params);
        });
    }

    // Helper methods for SQL parsing and execution

    Database::Result execute_select(const std::string& query) {
        // Very simple SQL parser for SELECT statements
        // This is not a full SQL parser, just enough to handle basic queries

        // Parse the table name
        std::string table_name;
        size_t from_pos = query.find("FROM");
        if (from_pos == std::string::npos) {
            from_pos = query.find("from");
        }

        if (from_pos != std::string::npos) {
            size_t table_start = query.find_first_not_of(" \t\n\r", from_pos + 4);
            size_t table_end = query.find_first_of(" \t\n\r,;", table_start);
            if (table_end == std::string::npos) {
                table_end = query.length();
            }

            table_name = query.substr(table_start, table_end - table_start);
        }

        // Parse the WHERE clause
        std::string where_clause;
        size_t where_pos = query.find("WHERE");
        if (where_pos == std::string::npos) {
            where_pos = query.find("where");
        }

        if (where_pos != std::string::npos) {
            where_clause = query.substr(where_pos + 5);
        }

        // Get all keys with the table prefix
        std::string prefix = table_name + ":";
        auto keys = store_->keys_with_prefix(prefix);

        // Filter keys based on the WHERE clause
        if (!where_clause.empty()) {
            // Very simple WHERE clause parser
            // Only supports basic equality conditions
            std::vector<std::string> filtered_keys;

            for (const auto& key : keys) {
                // Get the value for this key
                auto value_opt = store_->get(key);
                if (!value_opt) {
                    continue;
                }

                // Convert to string for simple comparison
                std::string value_str(value_opt->begin(), value_opt->end());

                // Check if the value matches the WHERE clause
                // This is a very simple check, not a full SQL parser
                if (where_clause.find(value_str) != std::string::npos) {
                    filtered_keys.push_back(key);
                }
            }

            keys = filtered_keys;
        }

        // Build the result
        std::vector<std::unordered_map<std::string, std::vector<uint8_t>>> result_data;

        for (const auto& key : keys) {
            auto value_opt = store_->get(key);
            if (!value_opt) {
                continue;
            }

            // Parse the key to get the column name
            // Format: table:id:column
            std::string column_name;
            size_t first_colon = key.find(':');
            size_t second_colon = key.find(':', first_colon + 1);

            if (second_colon != std::string::npos) {
                column_name = key.substr(second_colon + 1);
            } else {
                column_name = key.substr(first_colon + 1);
            }

            // Add to the result
            std::unordered_map<std::string, std::vector<uint8_t>> row;
            row[column_name] = *value_opt;
            result_data.push_back(row);
        }

        return Database::Result(result_data);
    }

    bool execute_insert(const std::string& query) {
        // Very simple SQL parser for INSERT statements
        // Format: INSERT INTO table (col1, col2, ...) VALUES (val1, val2, ...)

        // Parse the table name
        std::string table_name;
        size_t into_pos = query.find("INTO");
        if (into_pos == std::string::npos) {
            into_pos = query.find("into");
        }

        if (into_pos != std::string::npos) {
            size_t table_start = query.find_first_not_of(" \t\n\r", into_pos + 4);
            size_t table_end = query.find_first_of(" \t\n\r(,;", table_start);
            if (table_end == std::string::npos) {
                table_end = query.length();
            }

            table_name = query.substr(table_start, table_end - table_start);
        }

        // Parse the columns
        std::vector<std::string> columns;
        size_t columns_start = query.find('(', into_pos);
        if (columns_start != std::string::npos) {
            size_t columns_end = query.find(')', columns_start);
            if (columns_end != std::string::npos) {
                std::string columns_str = query.substr(columns_start + 1, columns_end - columns_start - 1);

                // Split by comma
                size_t pos = 0;
                while (pos < columns_str.length()) {
                    size_t comma_pos = columns_str.find(',', pos);
                    if (comma_pos == std::string::npos) {
                        comma_pos = columns_str.length();
                    }

                    std::string column = columns_str.substr(pos, comma_pos - pos);

                    // Trim whitespace
                    column.erase(0, column.find_first_not_of(" \t\n\r"));
                    column.erase(column.find_last_not_of(" \t\n\r") + 1);

                    columns.push_back(column);

                    pos = comma_pos + 1;
                }
            }
        }

        // Parse the values
        std::vector<std::string> values;
        size_t values_pos = query.find("VALUES");
        if (values_pos == std::string::npos) {
            values_pos = query.find("values");
        }

        if (values_pos != std::string::npos) {
            size_t values_start = query.find('(', values_pos);
            if (values_start != std::string::npos) {
                size_t values_end = query.find(')', values_start);
                if (values_end != std::string::npos) {
                    std::string values_str = query.substr(values_start + 1, values_end - values_start - 1);

                    // Split by comma
                    size_t pos = 0;
                    while (pos < values_str.length()) {
                        size_t comma_pos = values_str.find(',', pos);
                        if (comma_pos == std::string::npos) {
                            comma_pos = values_str.length();
                        }

                        std::string value = values_str.substr(pos, comma_pos - pos);

                        // Trim whitespace
                        value.erase(0, value.find_first_not_of(" \t\n\r"));
                        value.erase(value.find_last_not_of(" \t\n\r") + 1);

                        // Remove quotes
                        if (value.front() == '\'' && value.back() == '\'' ||
                            value.front() == '"' && value.back() == '"') {
                            value = value.substr(1, value.length() - 2);
                        }

                        values.push_back(value);

                        pos = comma_pos + 1;
                    }
                }
            }
        }

        // Check if we have the same number of columns and values
        if (columns.size() != values.size()) {
            std::cerr << "Number of columns and values don't match" << std::endl;
            return false;
        }

        // Generate a unique ID for this row
        std::string id = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        // Store each column-value pair
        for (size_t i = 0; i < columns.size(); ++i) {
            std::string key = table_name + ":" + id + ":" + columns[i];
            std::vector<uint8_t> value(values[i].begin(), values[i].end());

            if (!store_->put(key, value)) {
                std::cerr << "Failed to insert value for key: " << key << std::endl;
                return false;
            }
        }

        return true;
    }

    bool execute_update(const std::string& query) {
        // Very simple SQL parser for UPDATE statements
        // Format: UPDATE table SET col1 = val1, col2 = val2, ... WHERE condition

        // Parse the table name
        std::string table_name;
        size_t update_pos = query.find("UPDATE");
        if (update_pos == std::string::npos) {
            update_pos = query.find("update");
        }

        if (update_pos != std::string::npos) {
            size_t table_start = query.find_first_not_of(" \t\n\r", update_pos + 6);
            size_t table_end = query.find_first_of(" \t\n\r,;", table_start);
            if (table_end == std::string::npos) {
                table_end = query.length();
            }

            table_name = query.substr(table_start, table_end - table_start);
        }

        // Parse the SET clause
        std::unordered_map<std::string, std::string> updates;
        size_t set_pos = query.find("SET");
        if (set_pos == std::string::npos) {
            set_pos = query.find("set");
        }

        size_t where_pos = query.find("WHERE");
        if (where_pos == std::string::npos) {
            where_pos = query.find("where");
        }

        if (set_pos != std::string::npos) {
            size_t set_end = (where_pos != std::string::npos) ? where_pos : query.length();
            std::string set_clause = query.substr(set_pos + 3, set_end - set_pos - 3);

            // Split by comma
            size_t pos = 0;
            while (pos < set_clause.length()) {
                size_t comma_pos = set_clause.find(',', pos);
                if (comma_pos == std::string::npos) {
                    comma_pos = set_clause.length();
                }

                std::string assignment = set_clause.substr(pos, comma_pos - pos);

                // Split by equals
                size_t equals_pos = assignment.find('=');
                if (equals_pos != std::string::npos) {
                    std::string column = assignment.substr(0, equals_pos);
                    std::string value = assignment.substr(equals_pos + 1);

                    // Trim whitespace
                    column.erase(0, column.find_first_not_of(" \t\n\r"));
                    column.erase(column.find_last_not_of(" \t\n\r") + 1);

                    value.erase(0, value.find_first_not_of(" \t\n\r"));
                    value.erase(value.find_last_not_of(" \t\n\r") + 1);

                    // Remove quotes
                    if (value.front() == '\'' && value.back() == '\'' ||
                        value.front() == '"' && value.back() == '"') {
                        value = value.substr(1, value.length() - 2);
                    }

                    updates[column] = value;
                }

                pos = comma_pos + 1;
            }
        }

        // Parse the WHERE clause
        std::string where_clause;
        if (where_pos != std::string::npos) {
            where_clause = query.substr(where_pos + 5);
        }

        // Get all keys with the table prefix
        std::string prefix = table_name + ":";
        auto keys = store_->keys_with_prefix(prefix);

        // Filter keys based on the WHERE clause
        if (!where_clause.empty()) {
            // Very simple WHERE clause parser
            // Only supports basic equality conditions
            std::vector<std::string> filtered_keys;

            for (const auto& key : keys) {
                // Get the value for this key
                auto value_opt = store_->get(key);
                if (!value_opt) {
                    continue;
                }

                // Convert to string for simple comparison
                std::string value_str(value_opt->begin(), value_opt->end());

                // Check if the value matches the WHERE clause
                // This is a very simple check, not a full SQL parser
                if (where_clause.find(value_str) != std::string::npos) {
                    filtered_keys.push_back(key);
                }
            }

            keys = filtered_keys;
        }

        // Update each key
        for (const auto& key : keys) {
            // Parse the key to get the ID and column
            // Format: table:id:column
            size_t first_colon = key.find(':');
            size_t second_colon = key.find(':', first_colon + 1);

            if (second_colon == std::string::npos) {
                continue;
            }

            std::string id = key.substr(first_colon + 1, second_colon - first_colon - 1);
            std::string column = key.substr(second_colon + 1);

            // Check if this column should be updated
            auto it = updates.find(column);
            if (it != updates.end()) {
                // Update the value
                std::vector<uint8_t> value(it->second.begin(), it->second.end());

                if (!store_->put(key, value)) {
                    std::cerr << "Failed to update value for key: " << key << std::endl;
                    return false;
                }
            }
        }

        return true;
    }

    bool execute_delete(const std::string& query) {
        // Very simple SQL parser for DELETE statements
        // Format: DELETE FROM table WHERE condition

        // Parse the table name
        std::string table_name;
        size_t from_pos = query.find("FROM");
        if (from_pos == std::string::npos) {
            from_pos = query.find("from");
        }

        if (from_pos != std::string::npos) {
            size_t table_start = query.find_first_not_of(" \t\n\r", from_pos + 4);
            size_t table_end = query.find_first_of(" \t\n\r,;", table_start);
            if (table_end == std::string::npos) {
                table_end = query.length();
            }

            table_name = query.substr(table_start, table_end - table_start);
        }

        // Parse the WHERE clause
        std::string where_clause;
        size_t where_pos = query.find("WHERE");
        if (where_pos == std::string::npos) {
            where_pos = query.find("where");
        }

        if (where_pos != std::string::npos) {
            where_clause = query.substr(where_pos + 5);
        }

        // Get all keys with the table prefix
        std::string prefix = table_name + ":";
        auto keys = store_->keys_with_prefix(prefix);

        // Filter keys based on the WHERE clause
        if (!where_clause.empty()) {
            // Very simple WHERE clause parser
            // Only supports basic equality conditions
            std::vector<std::string> filtered_keys;

            for (const auto& key : keys) {
                // Get the value for this key
                auto value_opt = store_->get(key);
                if (!value_opt) {
                    continue;
                }

                // Convert to string for simple comparison
                std::string value_str(value_opt->begin(), value_opt->end());

                // Check if the value matches the WHERE clause
                // This is a very simple check, not a full SQL parser
                if (where_clause.find(value_str) != std::string::npos) {
                    filtered_keys.push_back(key);
                }
            }

            keys = filtered_keys;
        }

        // Delete each key
        for (const auto& key : keys) {
            if (!store_->remove(key)) {
                std::cerr << "Failed to delete key: " << key << std::endl;
                return false;
            }
        }

        return true;
    }

    bool execute_update(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Database not initialized" << std::endl;
            return false;
        }

        try {
            // Parse the SQL query to determine what to do
            std::string query = sql;

            // Replace parameters in the query
            for (size_t i = 0; i < params.size(); ++i) {
                std::string param_placeholder = "?";
                size_t pos = query.find(param_placeholder);
                if (pos != std::string::npos) {
                    query.replace(pos, param_placeholder.length(), params[i]);
                }
            }

            // Simple SQL parser to handle basic queries
            if (query.find("INSERT") == 0 || query.find("insert") == 0) {
                return execute_insert(query);
            } else if (query.find("UPDATE") == 0 || query.find("update") == 0) {
                return execute_update(query, {});
            } else if (query.find("DELETE") == 0 || query.find("delete") == 0) {
                return execute_delete(query);
            } else if (query.find("BEGIN") == 0 || query.find("begin") == 0) {
                return store_->begin_transaction();
            } else if (query.find("COMMIT") == 0 || query.find("commit") == 0) {
                return store_->commit_transaction();
            } else if (query.find("ROLLBACK") == 0 || query.find("rollback") == 0) {
                return store_->rollback_transaction();
            } else if (query.find("PRAGMA") == 0 || query.find("pragma") == 0) {
                // Ignore PRAGMA statements
                return true;
            } else {
                std::cerr << "Unsupported SQL query: " << query << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to execute query: " << e.what() << std::endl;
            return false;
        }
    }

    std::future<bool> execute_update_async(const std::string& sql, const std::vector<std::string>& params = {}) {
        return std::async(std::launch::async, [this, sql, params]() {
            return execute_update(sql, params);
        });
    }

    bool begin_transaction() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Database not initialized" << std::endl;
            return false;
        }

        return store_->begin_transaction();
    }

    std::future<bool> begin_transaction_async() {
        return std::async(std::launch::async, [this]() {
            return begin_transaction();
        });
    }

    bool commit_transaction() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Database not initialized" << std::endl;
            return false;
        }

        return store_->commit_transaction();
    }

    std::future<bool> commit_transaction_async() {
        return std::async(std::launch::async, [this]() {
            return commit_transaction();
        });
    }

    bool rollback_transaction() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            std::cerr << "Database not initialized" << std::endl;
            return false;
        }

        return store_->rollback_transaction();
    }

    std::future<bool> rollback_transaction_async() {
        return std::async(std::launch::async, [this]() {
            return rollback_transaction();
        });
    }

    int64_t last_insert_rowid() const {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            return 0;
        }

        // Generate a timestamp-based ID
        return std::chrono::system_clock::now().time_since_epoch().count();
    }

    int changes() const {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) {
            return 0;
        }

        // We don't track changes, so return 1 to indicate success
        return 1;
    }

    std::string path() const {
        return path_;
    }

    bool is_initialized() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return initialized_;
    }

    bool is_persistent() const {
        return store_->is_persistent();
    }

private:
    std::string path_;
    std::unique_ptr<detail::KeyValueStore> store_;
    bool initialized_;
    mutable std::mutex mutex_;
};

// Database public methods

Database::Database(const std::string& path, bool persistent)
    : impl_(std::make_unique<Impl>(path, persistent)) {
}

Database::~Database() = default;

bool Database::initialize() {
    return impl_->initialize();
}

std::future<bool> Database::initialize_async() {
    return impl_->initialize_async();
}

bool Database::close() {
    return impl_->close();
}

std::future<bool> Database::close_async() {
    return impl_->close_async();
}

Database::Result Database::execute(const std::string& sql, const std::vector<std::string>& params) {
    return impl_->execute(sql, params);
}

std::future<Database::Result> Database::execute_async(const std::string& sql, const std::vector<std::string>& params) {
    return impl_->execute_async(sql, params);
}

bool Database::execute_update(const std::string& sql, const std::vector<std::string>& params) {
    return impl_->execute_update(sql, params);
}

std::future<bool> Database::execute_update_async(const std::string& sql, const std::vector<std::string>& params) {
    return impl_->execute_update_async(sql, params);
}

bool Database::begin_transaction() {
    return impl_->begin_transaction();
}

std::future<bool> Database::begin_transaction_async() {
    return impl_->begin_transaction_async();
}

bool Database::commit_transaction() {
    return impl_->commit_transaction();
}

std::future<bool> Database::commit_transaction_async() {
    return impl_->commit_transaction_async();
}

bool Database::rollback_transaction() {
    return impl_->rollback_transaction();
}

std::future<bool> Database::rollback_transaction_async() {
    return impl_->rollback_transaction_async();
}

int64_t Database::last_insert_rowid() const {
    return impl_->last_insert_rowid();
}

int Database::changes() const {
    return impl_->changes();
}

std::string Database::path() const {
    return impl_->path();
}

bool Database::is_initialized() const {
    return impl_->is_initialized();
}

bool Database::is_persistent() const {
    return impl_->is_persistent();
}

} // namespace bitscrape::storage
