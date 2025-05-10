#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <future>
#include <source_location>

#include "bitscrape/types/beacon_types.hpp"
#include "bitscrape/beacon/beacon_sink.hpp"

namespace bitscrape::beacon {

/**
 * @brief Main beacon class for logging messages
 * 
 * Beacon is the main class for logging messages. It provides methods for logging
 * messages at different severity levels and manages multiple output sinks.
 */
class Beacon {
public:
    /**
     * @brief Create a beacon
     */
    Beacon();
    
    /**
     * @brief Destructor
     */
    ~Beacon();
    
    /**
     * @brief Add a sink to the beacon
     * 
     * @param sink Sink to add
     */
    void add_sink(std::unique_ptr<BeaconSink> sink);
    
    /**
     * @brief Log a debug message
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     */
    void debug(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
              const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log an info message
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     */
    void info(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
             const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log a warning message
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     */
    void warning(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
                const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log an error message
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     */
    void error(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
              const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log a critical message
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     */
    void critical(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
                 const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log a debug message asynchronously
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     * @return Future that will be completed when the message has been logged
     */
    std::future<void> debug_async(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
                                 const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log an info message asynchronously
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     * @return Future that will be completed when the message has been logged
     */
    std::future<void> info_async(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
                                const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log a warning message asynchronously
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     * @return Future that will be completed when the message has been logged
     */
    std::future<void> warning_async(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
                                   const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log an error message asynchronously
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     * @return Future that will be completed when the message has been logged
     */
    std::future<void> error_async(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
                                 const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log a critical message asynchronously
     * 
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     * @return Future that will be completed when the message has been logged
     */
    std::future<void> critical_async(const std::string& message, types::BeaconCategory category = types::BeaconCategory::GENERAL,
                                    const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log a message
     * 
     * @param severity Message severity
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     */
    void log(types::BeaconSeverity severity, const std::string& message, 
            types::BeaconCategory category = types::BeaconCategory::GENERAL,
            const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Log a message asynchronously
     * 
     * @param severity Message severity
     * @param message Message to log
     * @param category Message category
     * @param location Source location where the message was emitted
     * @return Future that will be completed when the message has been logged
     */
    std::future<void> log_async(types::BeaconSeverity severity, const std::string& message, 
                               types::BeaconCategory category = types::BeaconCategory::GENERAL,
                               const std::source_location& location = std::source_location::current());
    
private:
    std::vector<std::unique_ptr<BeaconSink>> sinks_; ///< Output sinks
    std::mutex mutex_; ///< Mutex for thread safety
};

/**
 * @brief Create a new beacon
 * 
 * @return Unique pointer to a new beacon
 */
std::unique_ptr<Beacon> create_beacon();

} // namespace bitscrape::beacon
