#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <source_location>
#include <chrono>
#include <memory>
#include <future>

#include "bitscrape/types/event_types.hpp"

namespace bitscrape::types {

/**
 * @brief Beacon severity level enumeration
 * 
 * Defines the severity levels for beacon messages, from least severe (DEBUG)
 * to most severe (CRITICAL).
 */
enum class BeaconSeverity {
    DEBUG,      ///< Detailed debugging information
    INFO,       ///< General information about system operation
    WARNING,    ///< Potential issues that don't prevent normal operation
    ERROR,      ///< Errors that prevent specific operations from completing
    CRITICAL    ///< Critical errors that may prevent the system from functioning
};

/**
 * @brief Get ANSI color code for a beacon severity level
 * 
 * @param severity Beacon severity level
 * @return ANSI color code string
 */
std::string get_severity_color(BeaconSeverity severity);

/**
 * @brief Get ANSI reset color code
 * 
 * @return ANSI reset color code string
 */
std::string get_reset_color();

/**
 * @brief Convert BeaconSeverity to string
 * 
 * @param severity Beacon severity level
 * @return String representation of the severity level
 */
std::string severity_to_string(BeaconSeverity severity);

/**
 * @brief Beacon category enumeration
 * 
 * Defines the categories for beacon messages, used for filtering and organization.
 */
enum class BeaconCategory {
    GENERAL,    ///< General messages not specific to any category
    SYSTEM,     ///< System-related messages
    NETWORK,    ///< Network-related messages
    DHT,        ///< DHT-related messages
    BITTORRENT, ///< BitTorrent-related messages
    TRACKER,    ///< Tracker-related messages
    DATABASE,   ///< Database-related messages
    UI          ///< User interface-related messages
};

/**
 * @brief Convert BeaconCategory to string
 * 
 * @param category Beacon category
 * @return String representation of the category
 */
std::string category_to_string(BeaconCategory category);

/**
 * @brief Beacon event class
 * 
 * Represents a beacon message as an event that can be dispatched through the event system.
 * Contains severity level, category, message, and source location information.
 */
class BeaconEvent : public Event {
public:
    /**
     * @brief Create a beacon event
     * 
     * @param severity Beacon severity level
     * @param category Beacon category
     * @param message Beacon message
     * @param location Source location where the beacon was emitted
     */
    BeaconEvent(BeaconSeverity severity, BeaconCategory category, const std::string& message,
                const std::source_location& location = std::source_location::current());
    
    /**
     * @brief Virtual destructor
     */
    ~BeaconEvent() override = default;
    
    /**
     * @brief Get the beacon severity level
     * 
     * @return Beacon severity level
     */
    BeaconSeverity severity() const { return severity_; }
    
    /**
     * @brief Get the beacon category
     * 
     * @return Beacon category
     */
    BeaconCategory category() const { return category_; }
    
    /**
     * @brief Get the beacon message
     * 
     * @return Beacon message
     */
    const std::string& message() const { return message_; }
    
    /**
     * @brief Get the source location
     * 
     * @return Source location where the beacon was emitted
     */
    const std::source_location& location() const { return location_; }
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<Event> clone() const override;
    
    /**
     * @brief Convert the event to a string representation
     * 
     * @return String representation of the event
     */
    std::string to_string() const override;

private:
    BeaconSeverity severity_;      ///< Beacon severity level
    BeaconCategory category_;      ///< Beacon category
    std::string message_;          ///< Beacon message
    std::source_location location_; ///< Source location where the beacon was emitted
};

} // namespace bitscrape::types
