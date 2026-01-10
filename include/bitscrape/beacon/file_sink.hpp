#pragma once

#include <string>
#include <fstream>
#include <mutex>

#include "bitscrape/beacon/beacon_sink.hpp"

namespace bitscrape::beacon {

/**
 * @brief File output destination for beacon messages
 * 
 * FileSink outputs beacon messages to a file with optional rotation.
 */
class FileSink : public BeaconSink {
public:
    /**
     * @brief Create a file sink
     * 
     * @param filename File name to write to
     * @param append Whether to append to the file if it exists
     */
    explicit FileSink(const std::string& filename, bool append = true);
    
    /**
     * @brief Destructor
     */
    ~FileSink() override;
    
    /**
     * @brief Write a beacon message to the file
     * 
     * @param severity Beacon severity level
     * @param category Beacon category
     * @param message Beacon message
     * @param location Source location where the beacon was emitted
     */
    void write(types::BeaconSeverity severity, types::BeaconCategory category, 
              const std::string& message, const std::source_location& location) override;
    
    /**
     * @brief Write a beacon message to the file asynchronously
     * 
     * @param severity Beacon severity level
     * @param category Beacon category
     * @param message Beacon message
     * @param location Source location where the beacon was emitted
     * @return Future that will be completed when the message has been written
     */
    std::future<void> write_async(types::BeaconSeverity severity, types::BeaconCategory category, 
                                const std::string& message, const std::source_location& location) override;
    
    /**
     * @brief Set the maximum file size before rotation
     * 
     * @param max_size Maximum file size in bytes
     */
    void set_max_file_size(size_t max_size);
    
    /**
     * @brief Set the maximum number of rotated files to keep
     * 
     * @param max_files Maximum number of rotated files
     */
    void set_max_files(size_t max_files);
    
private:
    std::string filename_; ///< File name to write to
    bool append_; ///< Whether to append to the file if it exists
    size_t max_file_size_ = 10 * 1024 * 1024; ///< Maximum file size in bytes (10 MB default)
    size_t max_files_ = 5; ///< Maximum number of rotated files (5 default)
    std::mutex mutex_; ///< Mutex for thread-safe file access
    std::ofstream file_; ///< Output file stream
    
    /**
     * @brief Rotate the file if needed
     */
    void rotate_if_needed();
    
    /**
     * @brief Open the file
     */
    void open_file();
    
    /**
     * @brief Close the file
     */
    void close_file();
};

/**
 * @brief Create a new file sink
 * 
 * @param filename File name to write to
 * @param append Whether to append to the file if it exists
 * @return Unique pointer to a new file sink
 */
std::unique_ptr<FileSink> create_file_sink(const std::string& filename, bool append = true);

} // namespace bitscrape::beacon
