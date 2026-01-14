#include <bitscrape/storage/database.hpp>
#include <bitscrape/storage/data_models.hpp>

#include <sqlite3.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>

namespace bitscrape::storage {

// Database::Result implementation

Database::Result::Result()
    : current_row_(std::numeric_limits<size_t>::max()), has_rows_(false) {
}

Database::Result::Result(std::vector<std::unordered_map<std::string, std::vector<uint8_t>>> data)
    : data_(std::move(data)), current_row_(std::numeric_limits<size_t>::max()), has_rows_(false) {
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
    return get_column_name(index);
}

std::string Database::Result::get_column_name(int index) const {
    if (data_.empty() || current_row_ >= data_.size()) {
        return "";
    }

    std::vector<std::string> column_names;
    for (const auto& [key, _] : data_[current_row_]) {
        column_names.push_back(key);
    }
    std::sort(column_names.begin(), column_names.end());

    if (index >= 0 && index < static_cast<int>(column_names.size())) {
        return column_names[index];
    }
    return "";
}

int Database::Result::column_index(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size()) {
        return -1;
    }

    if (data_[current_row_].find(name) == data_[current_row_].end()) {
        return -1;
    }

    std::vector<std::string> column_names;
    for (const auto& [key, _] : data_[current_row_]) {
        column_names.push_back(key);
    }
    std::sort(column_names.begin(), column_names.end());

    auto it = std::find(column_names.begin(), column_names.end(), name);
    if (it != column_names.end()) {
        return static_cast<int>(std::distance(column_names.begin(), it));
    }
    return -1;
}

bool Database::Result::next() {
    if (data_.empty()) {
        has_rows_ = false;
        return false;
    }

    if (current_row_ == std::numeric_limits<size_t>::max()) {
        current_row_ = 0;
    } else {
        current_row_++;
    }

    has_rows_ = (current_row_ < data_.size());
    return has_rows_;
}

int Database::Result::get_int(int index) const {
    std::string col_name = get_column_name(index);
    return col_name.empty() ? 0 : get_int(col_name);
}

int Database::Result::get_int(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return 0;
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return 0;
    }

    const auto& value = it->second;
    if (value.size() == sizeof(int)) {
        int result = 0;
        std::memcpy(&result, value.data(), sizeof(int));
        return result;
    } else if (value.size() == sizeof(int64_t)) {
        int64_t result = 0;
        std::memcpy(&result, value.data(), sizeof(int64_t));
        return static_cast<int>(result);
    } else if (!value.empty()) {
        try {
            return std::stoi(std::string(value.begin(), value.end()));
        } catch (...) {}
    }
    return 0;
}

int64_t Database::Result::get_int64(int index) const {
    std::string col_name = get_column_name(index);
    return col_name.empty() ? 0 : get_int64(col_name);
}

int64_t Database::Result::get_int64(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return 0;
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return 0;
    }

    const auto& value = it->second;
    if (value.size() == sizeof(int64_t)) {
        int64_t result = 0;
        std::memcpy(&result, value.data(), sizeof(int64_t));
        return result;
    } else if (value.size() == sizeof(int)) {
        int result = 0;
        std::memcpy(&result, value.data(), sizeof(int));
        return static_cast<int64_t>(result);
    } else if (!value.empty()) {
        try {
            return std::stoll(std::string(value.begin(), value.end()));
        } catch (...) {}
    }
    return 0;
}

double Database::Result::get_double(int index) const {
    std::string col_name = get_column_name(index);
    return col_name.empty() ? 0.0 : get_double(col_name);
}

double Database::Result::get_double(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return 0.0;
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return 0.0;
    }

    const auto& value = it->second;
    if (value.size() == sizeof(double)) {
        double result = 0.0;
        std::memcpy(&result, value.data(), sizeof(double));
        return result;
    } else if (!value.empty()) {
        try {
            return std::stod(std::string(value.begin(), value.end()));
        } catch (...) {}
    }
    return 0.0;
}

std::string Database::Result::get_string(int index) const {
    std::string col_name = get_column_name(index);
    return col_name.empty() ? "" : get_string(col_name);
}

std::string Database::Result::get_string(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return "";
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return "";
    }

    return std::string(it->second.begin(), it->second.end());
}

std::vector<uint8_t> Database::Result::get_blob(int index) const {
    std::string col_name = get_column_name(index);
    return col_name.empty() ? std::vector<uint8_t>{} : get_blob(col_name);
}

std::vector<uint8_t> Database::Result::get_blob(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return {};
    }

    auto it = data_[current_row_].find(name);
    if (it == data_[current_row_].end()) {
        return {};
    }
    return it->second;
}

bool Database::Result::is_null(int index) const {
    std::string col_name = get_column_name(index);
    return col_name.empty() ? true : is_null(col_name);
}

bool Database::Result::is_null(const std::string& name) const {
    if (data_.empty() || current_row_ >= data_.size() || !has_rows_) {
        return true;
    }

    auto it = data_[current_row_].find(name);
    return it == data_[current_row_].end();
}

// Database implementation

class Database::Impl {
public:
    Impl(const std::string& path, bool persistent)
        : path_(path.empty() ? "data/bitscrape.db" : path),
          db_(nullptr),
          initialized_(false),
          is_persistent_(persistent) {
    }

    ~Impl() {
        close();
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) return true;

        if (!is_persistent_) {
            path_ = ":memory:";
        } else {
            std::filesystem::path fs_path(path_);
            if (fs_path.has_parent_path() && !std::filesystem::exists(fs_path.parent_path())) {
                std::filesystem::create_directories(fs_path.parent_path());
            }
        }

        int rc = sqlite3_open(path_.c_str(), &db_);
        if (rc != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }

        // Enable WAL mode for better concurrency
        sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

        create_tables();

        initialized_ = true;
        return true;
    }

    void create_tables() {
        const char* sql = 
            "CREATE TABLE IF NOT EXISTS nodes ("
            "  node_id TEXT PRIMARY KEY,"
            "  ip TEXT,"
            "  port INTEGER,"
            "  first_seen TIMESTAMP,"
            "  last_seen TIMESTAMP,"
            "  ping_count INTEGER DEFAULT 0,"
            "  query_count INTEGER DEFAULT 0,"
            "  response_count INTEGER DEFAULT 0,"
            "  last_rtt_ms INTEGER,"
            "  is_responsive INTEGER DEFAULT 1"
            ");"
            "CREATE TABLE IF NOT EXISTS infohashes ("
            "  info_hash TEXT PRIMARY KEY,"
            "  first_seen TIMESTAMP,"
            "  last_seen TIMESTAMP,"
            "  announce_count INTEGER DEFAULT 0,"
            "  peer_count INTEGER DEFAULT 0,"
            "  has_metadata INTEGER DEFAULT 0"
            ");"
            "CREATE TABLE IF NOT EXISTS metadata ("
            "  info_hash TEXT PRIMARY KEY,"
            "  name TEXT,"
            "  total_size INTEGER,"
            "  piece_count INTEGER,"
            "  piece_length INTEGER,"
            "  file_count INTEGER,"
            "  comment TEXT,"
            "  download_time TIMESTAMP,"
            "  raw_metadata BLOB"
            ");"
            "CREATE TABLE IF NOT EXISTS files ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  info_hash TEXT,"
            "  path TEXT,"
            "  size INTEGER,"
            "  FOREIGN KEY(info_hash) REFERENCES infohashes(info_hash)"
            ");"
            "CREATE TABLE IF NOT EXISTS trackers ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  info_hash TEXT,"
            "  url TEXT,"
            "  announce_count INTEGER DEFAULT 0,"
            "  scrape_count INTEGER DEFAULT 0,"
            "  first_seen TIMESTAMP,"
            "  last_seen TIMESTAMP,"
            "  FOREIGN KEY(info_hash) REFERENCES infohashes(info_hash)"
            ");"
            "CREATE TABLE IF NOT EXISTS peers ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  info_hash TEXT,"
            "  ip TEXT,"
            "  port INTEGER,"
            "  peer_id TEXT,"
            "  first_seen TIMESTAMP,"
            "  last_seen TIMESTAMP,"
            "  supports_dht INTEGER DEFAULT 0,"
            "  supports_extension_protocol INTEGER DEFAULT 0,"
            "  supports_fast_protocol INTEGER DEFAULT 0,"
            "  FOREIGN KEY(info_hash) REFERENCES infohashes(info_hash)"
            ");";
        
        char* err_msg = nullptr;
        int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err_msg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        }
    }

    bool close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        initialized_ = false;
        return true;
    }

    Database::Result execute(const std::string& sql, const std::vector<std::string>& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) return {};

        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << " SQL: " << sql << std::endl;
            return {};
        }

        for (size_t i = 0; i < params.size(); ++i) {
            sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        }

        std::vector<std::unordered_map<std::string, std::vector<uint8_t>>> results;
        int col_count = sqlite3_column_count(stmt);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::unordered_map<std::string, std::vector<uint8_t>> row;
            for (int i = 0; i < col_count; ++i) {
                const char* name = sqlite3_column_name(stmt, i);
                int type = sqlite3_column_type(stmt, i);
                
                std::vector<uint8_t> val;
                if (type == SQLITE_INTEGER) {
                    int64_t v = sqlite3_column_int64(stmt, i);
                    val.resize(sizeof(int64_t));
                    std::memcpy(val.data(), &v, sizeof(int64_t));
                } else if (type == SQLITE_FLOAT) {
                    double v = sqlite3_column_double(stmt, i);
                    val.resize(sizeof(double));
                    std::memcpy(val.data(), &v, sizeof(double));
                } else if (type == SQLITE_TEXT) {
                    const unsigned char* text = sqlite3_column_text(stmt, i);
                    if (text) {
                        val.assign(text, text + std::strlen(reinterpret_cast<const char*>(text)));
                    }
                } else if (type == SQLITE_BLOB) {
                    const void* blob = sqlite3_column_blob(stmt, i);
                    int size = sqlite3_column_bytes(stmt, i);
                    if (blob && size > 0) {
                        val.assign(static_cast<const uint8_t*>(blob), static_cast<const uint8_t*>(blob) + size);
                    }
                }
                row[name] = std::move(val);
            }
            results.push_back(std::move(row));
        }

        sqlite3_finalize(stmt);
        return Database::Result(std::move(results));
    }

    bool execute_update(const std::string& sql, const std::vector<std::string>& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!initialized_) return false;

        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare update: " << sqlite3_errmsg(db_) << " SQL: " << sql << std::endl;
            return false;
        }

        for (size_t i = 0; i < params.size(); ++i) {
            sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);
        }

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (rc == SQLITE_DONE || rc == SQLITE_OK);
    }

    bool begin_transaction() {
        return execute_update("BEGIN TRANSACTION;", {});
    }

    bool commit_transaction() {
        return execute_update("COMMIT;", {});
    }

    bool rollback_transaction() {
        return execute_update("ROLLBACK;", {});
    }

    int64_t last_insert_rowid() const {
        return sqlite3_last_insert_rowid(db_);
    }

    int changes() const {
        return sqlite3_changes(db_);
    }

    std::string path() const { return path_; }
    bool is_initialized() const { return initialized_; }
    bool is_persistent() const { return is_persistent_; }

private:
    std::string path_;
    sqlite3* db_;
    bool initialized_;
    bool is_persistent_;
    mutable std::mutex mutex_;
};

Database::Database(const std::string& path, bool persistent)
    : impl_(std::make_unique<Impl>(path, persistent)) {
}

Database::~Database() = default;

bool Database::initialize() { return impl_->initialize(); }
std::future<bool> Database::initialize_async() {
    return std::async(std::launch::async, [this]() { return initialize(); });
}

bool Database::close() { return impl_->close(); }
std::future<bool> Database::close_async() {
    return std::async(std::launch::async, [this]() { return close(); });
}

Database::Result Database::execute(const std::string& sql, const std::vector<std::string>& params) {
    return impl_->execute(sql, params);
}

std::future<Database::Result> Database::execute_async(const std::string& sql, const std::vector<std::string>& params) {
    return std::async(std::launch::async, [this, sql, params]() { return execute(sql, params); });
}

bool Database::execute_update(const std::string& sql, const std::vector<std::string>& params) {
    return impl_->execute_update(sql, params);
}

std::future<bool> Database::execute_update_async(const std::string& sql, const std::vector<std::string>& params) {
    return std::async(std::launch::async, [this, sql, params]() { return execute_update(sql, params); });
}

bool Database::begin_transaction() { return impl_->begin_transaction(); }
std::future<bool> Database::begin_transaction_async() {
    return std::async(std::launch::async, [this]() { return begin_transaction(); });
}

bool Database::commit_transaction() { return impl_->commit_transaction(); }
std::future<bool> Database::commit_transaction_async() {
    return std::async(std::launch::async, [this]() { return commit_transaction(); });
}

bool Database::rollback_transaction() { return impl_->rollback_transaction(); }
std::future<bool> Database::rollback_transaction_async() {
    return std::async(std::launch::async, [this]() { return rollback_transaction(); });
}

int64_t Database::last_insert_rowid() const { return impl_->last_insert_rowid(); }
int Database::changes() const { return impl_->changes(); }
std::string Database::path() const { return impl_->path(); }
bool Database::is_initialized() const { return impl_->is_initialized(); }
bool Database::is_persistent() const { return impl_->is_persistent(); }

} // namespace bitscrape::storage
