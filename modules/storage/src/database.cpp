#include <bitscrape/storage/database.hpp>
#include <sqlite3.h>

#include <iostream>
#include <sstream>
#include <filesystem>
#include <chrono>

namespace bitscrape::storage {

// Database::Result implementation

Database::Result::Result()
    : stmt_(nullptr), db_(nullptr), has_rows_(false) {
}

Database::Result::Result(sqlite3_stmt* stmt, sqlite3* db)
    : stmt_(stmt), db_(db), has_rows_(false) {
    // Check if there are rows
    has_rows_ = (stmt_ != nullptr);
}

Database::Result::~Result() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
}

Database::Result::Result(Result&& other) noexcept
    : stmt_(other.stmt_), db_(other.db_), has_rows_(other.has_rows_) {
    other.stmt_ = nullptr;
    other.db_ = nullptr;
    other.has_rows_ = false;
}

Database::Result& Database::Result::operator=(Result&& other) noexcept {
    if (this != &other) {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
        
        stmt_ = other.stmt_;
        db_ = other.db_;
        has_rows_ = other.has_rows_;
        
        other.stmt_ = nullptr;
        other.db_ = nullptr;
        other.has_rows_ = false;
    }
    
    return *this;
}

bool Database::Result::has_rows() const {
    return has_rows_;
}

int Database::Result::column_count() const {
    if (!stmt_) {
        return 0;
    }
    
    return sqlite3_column_count(stmt_);
}

std::string Database::Result::column_name(int index) const {
    if (!stmt_) {
        return "";
    }
    
    const char* name = sqlite3_column_name(stmt_, index);
    return name ? name : "";
}

int Database::Result::column_index(const std::string& name) const {
    if (!stmt_) {
        return -1;
    }
    
    int count = sqlite3_column_count(stmt_);
    for (int i = 0; i < count; ++i) {
        const char* column_name = sqlite3_column_name(stmt_, i);
        if (column_name && name == column_name) {
            return i;
        }
    }
    
    return -1;
}

bool Database::Result::next() {
    if (!stmt_) {
        return false;
    }
    
    int result = sqlite3_step(stmt_);
    has_rows_ = (result == SQLITE_ROW);
    
    return has_rows_;
}

int Database::Result::get_int(int index) const {
    if (!stmt_ || !has_rows_) {
        return 0;
    }
    
    return sqlite3_column_int(stmt_, index);
}

int Database::Result::get_int(const std::string& name) const {
    int index = column_index(name);
    if (index < 0) {
        return 0;
    }
    
    return get_int(index);
}

int64_t Database::Result::get_int64(int index) const {
    if (!stmt_ || !has_rows_) {
        return 0;
    }
    
    return sqlite3_column_int64(stmt_, index);
}

int64_t Database::Result::get_int64(const std::string& name) const {
    int index = column_index(name);
    if (index < 0) {
        return 0;
    }
    
    return get_int64(index);
}

double Database::Result::get_double(int index) const {
    if (!stmt_ || !has_rows_) {
        return 0.0;
    }
    
    return sqlite3_column_double(stmt_, index);
}

double Database::Result::get_double(const std::string& name) const {
    int index = column_index(name);
    if (index < 0) {
        return 0.0;
    }
    
    return get_double(index);
}

std::string Database::Result::get_string(int index) const {
    if (!stmt_ || !has_rows_) {
        return "";
    }
    
    const unsigned char* text = sqlite3_column_text(stmt_, index);
    return text ? reinterpret_cast<const char*>(text) : "";
}

std::string Database::Result::get_string(const std::string& name) const {
    int index = column_index(name);
    if (index < 0) {
        return "";
    }
    
    return get_string(index);
}

std::vector<uint8_t> Database::Result::get_blob(int index) const {
    if (!stmt_ || !has_rows_) {
        return {};
    }
    
    const void* blob = sqlite3_column_blob(stmt_, index);
    int size = sqlite3_column_bytes(stmt_, index);
    
    if (!blob || size <= 0) {
        return {};
    }
    
    const uint8_t* data = static_cast<const uint8_t*>(blob);
    return std::vector<uint8_t>(data, data + size);
}

std::vector<uint8_t> Database::Result::get_blob(const std::string& name) const {
    int index = column_index(name);
    if (index < 0) {
        return {};
    }
    
    return get_blob(index);
}

bool Database::Result::is_null(int index) const {
    if (!stmt_ || !has_rows_) {
        return true;
    }
    
    return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
}

bool Database::Result::is_null(const std::string& name) const {
    int index = column_index(name);
    if (index < 0) {
        return true;
    }
    
    return is_null(index);
}

// Database implementation

class Database::Impl {
public:
    Impl(const std::string& path)
        : path_(path), db_(nullptr), initialized_(false) {
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
            // Create directory if it doesn't exist
            std::filesystem::path path(path_);
            std::filesystem::create_directories(path.parent_path());
            
            // Open database
            int rc = sqlite3_open(path_.c_str(), &db_);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
                return false;
            }
            
            // Enable foreign keys
            execute_update("PRAGMA foreign_keys = ON;");
            
            // Set busy timeout
            sqlite3_busy_timeout(db_, 5000);  // 5 seconds
            
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
        
        if (db_) {
            int rc = sqlite3_close(db_);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to close database: " << sqlite3_errmsg(db_) << std::endl;
                return false;
            }
            db_ = nullptr;
        }
        
        initialized_ = false;
        return true;
    }
    
    std::future<bool> close_async() {
        return std::async(std::launch::async, [this]() {
            return close();
        });
    }
    
    Database::Result execute(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            std::cerr << "Database not initialized" << std::endl;
            return Database::Result();
        }
        
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
            return Database::Result();
        }
        
        // Bind parameters
        for (size_t i = 0; i < params.size(); ++i) {
            rc = sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to bind parameter " << i + 1 << ": " << sqlite3_errmsg(db_) << std::endl;
                sqlite3_finalize(stmt);
                return Database::Result();
            }
        }
        
        return Database::Result(stmt, db_);
    }
    
    std::future<Database::Result> execute_async(const std::string& sql, const std::vector<std::string>& params = {}) {
        return std::async(std::launch::async, [this, sql, params]() {
            return execute(sql, params);
        });
    }
    
    bool execute_update(const std::string& sql, const std::vector<std::string>& params = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            std::cerr << "Database not initialized" << std::endl;
            return false;
        }
        
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        // Bind parameters
        for (size_t i = 0; i < params.size(); ++i) {
            rc = sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to bind parameter " << i + 1 << ": " << sqlite3_errmsg(db_) << std::endl;
                sqlite3_finalize(stmt);
                return false;
            }
        }
        
        // Execute statement
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db_) << std::endl;
            return false;
        }
        
        return true;
    }
    
    std::future<bool> execute_update_async(const std::string& sql, const std::vector<std::string>& params = {}) {
        return std::async(std::launch::async, [this, sql, params]() {
            return execute_update(sql, params);
        });
    }
    
    bool begin_transaction() {
        return execute_update("BEGIN TRANSACTION;");
    }
    
    std::future<bool> begin_transaction_async() {
        return std::async(std::launch::async, [this]() {
            return begin_transaction();
        });
    }
    
    bool commit_transaction() {
        return execute_update("COMMIT;");
    }
    
    std::future<bool> commit_transaction_async() {
        return std::async(std::launch::async, [this]() {
            return commit_transaction();
        });
    }
    
    bool rollback_transaction() {
        return execute_update("ROLLBACK;");
    }
    
    std::future<bool> rollback_transaction_async() {
        return std::async(std::launch::async, [this]() {
            return rollback_transaction();
        });
    }
    
    int64_t last_insert_rowid() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_ || !db_) {
            return 0;
        }
        
        return sqlite3_last_insert_rowid(db_);
    }
    
    int changes() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_ || !db_) {
            return 0;
        }
        
        return sqlite3_changes(db_);
    }
    
    std::string path() const {
        return path_;
    }
    
    bool is_initialized() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return initialized_;
    }
    
private:
    std::string path_;
    sqlite3* db_;
    bool initialized_;
    mutable std::mutex mutex_;
};

// Database public methods

Database::Database(const std::string& path)
    : impl_(std::make_unique<Impl>(path)) {
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

} // namespace bitscrape::storage
