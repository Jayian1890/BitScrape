#include "bitscrape/beacon/file_sink.hpp"

#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <system_error>

namespace bitscrape::beacon {

FileSink::FileSink(const std::string& filename, bool append)
    : filename_(filename), append_(append) {
    open_file();
}

FileSink::~FileSink() {
    close_file();
}

void FileSink::write(types::BeaconSeverity severity, types::BeaconCategory category, 
                    const std::string& message, const std::source_location& location) {
    if (!should_log(severity, category)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Make sure the file is open
    if (!file_.is_open()) {
        open_file();
        if (!file_.is_open()) {
            return; // Failed to open the file
        }
    }
    
    // Rotate the file if needed
    rotate_if_needed();
    
    // Format timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch() % std::chrono::seconds(1)).count();
    
    std::ostringstream timestamp;
    timestamp << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
             << '.' << std::setfill('0') << std::setw(3) << ms;
    
    // Format source location
    std::string file_name = location.file_name();
    // Extract just the filename without the path
    auto pos = file_name.find_last_of("/\\");
    if (pos != std::string::npos) {
        file_name = file_name.substr(pos + 1);
    }
    
    std::ostringstream source;
    source << file_name << ":" << location.line();
    
    // Format and output the beacon message
    file_ << timestamp.str() << " "
         << "[" << types::severity_to_string(severity) << "] "
         << "[" << types::category_to_string(category) << "] "
         << message << " "
         << "(" << source.str() << ")"
         << std::endl;
}

std::future<void> FileSink::write_async(types::BeaconSeverity severity, types::BeaconCategory category, 
                                      const std::string& message, const std::source_location& location) {
    return std::async(std::launch::async, [this, severity, category, message, location]() {
        this->write(severity, category, message, location);
    });
}

void FileSink::set_max_file_size(size_t max_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_file_size_ = max_size;
}

void FileSink::set_max_files(size_t max_files) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_files_ = max_files;
}

void FileSink::rotate_if_needed() {
    // Check if the file is too large
    if (file_.tellp() >= static_cast<std::streampos>(max_file_size_)) {
        // Close the current file
        close_file();
        
        // Rotate the files
        namespace fs = std::filesystem;
        
        try {
            // Remove the oldest file if it exists
            if (max_files_ > 0) {
                std::string oldest_file = filename_ + "." + std::to_string(max_files_);
                fs::remove(oldest_file);
            }
            
            // Rename the existing files
            for (size_t i = max_files_ - 1; i > 0; --i) {
                std::string old_file = filename_ + "." + std::to_string(i);
                std::string new_file = filename_ + "." + std::to_string(i + 1);
                
                if (fs::exists(old_file)) {
                    fs::rename(old_file, new_file);
                }
            }
            
            // Rename the current file
            if (fs::exists(filename_)) {
                fs::rename(filename_, filename_ + ".1");
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // Ignore errors during rotation
        }
        
        // Open a new file
        open_file();
    }
}

void FileSink::open_file() {
    file_.open(filename_, append_ ? (std::ios::out | std::ios::app) : std::ios::out);
}

void FileSink::close_file() {
    if (file_.is_open()) {
        file_.close();
    }
}

std::unique_ptr<FileSink> create_file_sink(const std::string& filename, bool append) {
    return std::make_unique<FileSink>(filename, append);
}

} // namespace bitscrape::beacon
