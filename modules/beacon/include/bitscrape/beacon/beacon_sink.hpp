#pragma once

#include <vector>
#include <mutex>
#include <future>
#include <source_location>

#include "bitscrape/types/beacon_types.hpp"

namespace bitscrape::beacon {

/**
 * @brief Interface for beacon output destinations
 *
 * BeaconSink defines the interface for all beacon output destinations.
 * It provides methods for writing beacon messages and filtering by severity and category.
 */
class BeaconSink {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~BeaconSink() = default;

    /**
     * @brief Write a beacon message
     *
     * @param severity Beacon severity level
     * @param category Beacon category
     * @param message Beacon message
     * @param location Source location where the beacon was emitted
     */
    virtual void write(types::BeaconSeverity severity, types::BeaconCategory category,
                      const std::string& message, const std::source_location& location) = 0;

    /**
     * @brief Write a beacon message asynchronously
     *
     * @param severity Beacon severity level
     * @param category Beacon category
     * @param message Beacon message
     * @param location Source location where the beacon was emitted
     * @return Future that will be completed when the message has been written
     */
    virtual std::future<void> write_async(types::BeaconSeverity severity, types::BeaconCategory category,
                                        const std::string& message, const std::source_location& location);

    /**
     * @brief Set the minimum severity level to log
     *
     * @param min_severity Minimum severity level
     */
    virtual void set_min_severity(types::BeaconSeverity min_severity);

    /**
     * @brief Set the categories to log
     *
     * @param categories Categories to log
     */
    virtual void set_categories(const std::vector<types::BeaconCategory>& categories);

    /**
     * @brief Clear the category filter
     */
    virtual void clear_category_filter();

public:
    /**
     * @brief Check if a message should be logged
     *
     * @param severity Beacon severity level
     * @param category Beacon category
     * @return true if the message should be logged, false otherwise
     */
    bool should_log(types::BeaconSeverity severity, types::BeaconCategory category) const;

    types::BeaconSeverity min_severity_ = types::BeaconSeverity::DEBUG; ///< Minimum severity level to log
    std::vector<types::BeaconCategory> categories_; ///< Categories to log
    bool filter_categories_ = false; ///< Whether to filter by category
    mutable std::mutex mutex_; ///< Mutex for thread safety
};

} // namespace bitscrape::beacon
