#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Forward declaration for SQLite3
struct sqlite3;
struct sqlite3_stmt;

namespace bitscrape::storage::detail {

/**
 * @brief Base class for database query results
 * 
 * This class provides the interface for accessing database query results.
 * It is used to avoid circular dependencies between header files.
 */
class DatabaseResult {
public:
    /**
     * @brief Create an empty result
     */
    DatabaseResult() = default;
    
    /**
     * @brief Destructor
     */
    virtual ~DatabaseResult() = default;
    
    /**
     * @brief Check if the result has rows
     * 
     * @return true if the result has rows, false otherwise
     */
    virtual bool has_rows() const = 0;
    
    /**
     * @brief Get the number of columns in the result
     * 
     * @return Number of columns
     */
    virtual int column_count() const = 0;
    
    /**
     * @brief Get the name of a column
     * 
     * @param index Column index
     * @return Column name
     */
    virtual std::string column_name(int index) const = 0;
    
    /**
     * @brief Get the index of a column by name
     * 
     * @param name Column name
     * @return Column index, or -1 if not found
     */
    virtual int column_index(const std::string& name) const = 0;
    
    /**
     * @brief Move to the next row
     * 
     * @return true if there is a next row, false otherwise
     */
    virtual bool next() = 0;
    
    /**
     * @brief Get an integer value from the current row
     * 
     * @param index Column index
     * @return Integer value
     */
    virtual int get_int(int index) const = 0;
    
    /**
     * @brief Get an integer value from the current row
     * 
     * @param name Column name
     * @return Integer value
     */
    virtual int get_int(const std::string& name) const = 0;
    
    /**
     * @brief Get a 64-bit integer value from the current row
     * 
     * @param index Column index
     * @return 64-bit integer value
     */
    virtual int64_t get_int64(int index) const = 0;
    
    /**
     * @brief Get a 64-bit integer value from the current row
     * 
     * @param name Column name
     * @return 64-bit integer value
     */
    virtual int64_t get_int64(const std::string& name) const = 0;
    
    /**
     * @brief Get a double value from the current row
     * 
     * @param index Column index
     * @return Double value
     */
    virtual double get_double(int index) const = 0;
    
    /**
     * @brief Get a double value from the current row
     * 
     * @param name Column name
     * @return Double value
     */
    virtual double get_double(const std::string& name) const = 0;
    
    /**
     * @brief Get a string value from the current row
     * 
     * @param index Column index
     * @return String value
     */
    virtual std::string get_string(int index) const = 0;
    
    /**
     * @brief Get a string value from the current row
     * 
     * @param name Column name
     * @return String value
     */
    virtual std::string get_string(const std::string& name) const = 0;
    
    /**
     * @brief Get a blob value from the current row
     * 
     * @param index Column index
     * @return Blob value as a vector of bytes
     */
    virtual std::vector<uint8_t> get_blob(int index) const = 0;
    
    /**
     * @brief Get a blob value from the current row
     * 
     * @param name Column name
     * @return Blob value as a vector of bytes
     */
    virtual std::vector<uint8_t> get_blob(const std::string& name) const = 0;
    
    /**
     * @brief Check if a column is null
     * 
     * @param index Column index
     * @return true if the column is null, false otherwise
     */
    virtual bool is_null(int index) const = 0;
    
    /**
     * @brief Check if a column is null
     * 
     * @param name Column name
     * @return true if the column is null, false otherwise
     */
    virtual bool is_null(const std::string& name) const = 0;
};

} // namespace bitscrape::storage::detail
