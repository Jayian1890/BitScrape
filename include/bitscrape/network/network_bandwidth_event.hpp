#pragma once

#include "bitscrape/network/network_event_processor.hpp"
#include "bitscrape/types/event_types.hpp"
#include <string>
#include <memory>
#include <chrono>

namespace bitscrape::network {

/**
 * @brief Network bandwidth event types
 */
enum class BandwidthEventType : uint16_t {
    BANDWIDTH_USAGE = 3000,     ///< Periodic bandwidth usage report
    BANDWIDTH_LIMIT_REACHED,    ///< Bandwidth limit has been reached
    BANDWIDTH_THROTTLED,        ///< Network has been throttled
    BANDWIDTH_RESTORED          ///< Normal bandwidth has been restored
};

/**
 * @brief Base class for network bandwidth events
 */
class NetworkBandwidthEvent : public types::Event {
public:
    /**
     * @brief Create a new network bandwidth event
     * 
     * @param type Bandwidth event type
     */
    explicit NetworkBandwidthEvent(BandwidthEventType type)
        : types::Event(types::Event::Type::USER_DEFINED, static_cast<uint32_t>(type)),
          bandwidth_event_type_(type) {}
    
    /**
     * @brief Get the bandwidth event type
     * 
     * @return Bandwidth event type
     */
    BandwidthEventType bandwidth_event_type() const { return bandwidth_event_type_; }
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<NetworkBandwidthEvent>(*this);
    }
    
    /**
     * @brief Convert the event to a string representation
     * 
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = types::Event::to_string();
        std::string type;
        
        switch (bandwidth_event_type_) {
            case BandwidthEventType::BANDWIDTH_USAGE:
                type = "BANDWIDTH_USAGE";
                break;
            case BandwidthEventType::BANDWIDTH_LIMIT_REACHED:
                type = "BANDWIDTH_LIMIT_REACHED";
                break;
            case BandwidthEventType::BANDWIDTH_THROTTLED:
                type = "BANDWIDTH_THROTTLED";
                break;
            case BandwidthEventType::BANDWIDTH_RESTORED:
                type = "BANDWIDTH_RESTORED";
                break;
            default:
                type = "UNKNOWN";
                break;
        }
        
        return base + " [NetworkBandwidthEvent: " + type + "]";
    }
    
private:
    BandwidthEventType bandwidth_event_type_; ///< Bandwidth event type
};

/**
 * @brief Event for reporting bandwidth usage
 */
class BandwidthUsageEvent : public NetworkBandwidthEvent {
public:
    /**
     * @brief Create a new bandwidth usage event
     * 
     * @param bytes_sent Number of bytes sent in the reporting period
     * @param bytes_received Number of bytes received in the reporting period
     * @param period_ms Duration of the reporting period in milliseconds
     */
    BandwidthUsageEvent(uint64_t bytes_sent, uint64_t bytes_received, uint64_t period_ms)
        : NetworkBandwidthEvent(BandwidthEventType::BANDWIDTH_USAGE),
          bytes_sent_(bytes_sent),
          bytes_received_(bytes_received),
          period_ms_(period_ms) {}
    
    /**
     * @brief Get the number of bytes sent
     * 
     * @return Number of bytes sent
     */
    uint64_t bytes_sent() const { return bytes_sent_; }
    
    /**
     * @brief Get the number of bytes received
     * 
     * @return Number of bytes received
     */
    uint64_t bytes_received() const { return bytes_received_; }
    
    /**
     * @brief Get the reporting period in milliseconds
     * 
     * @return Reporting period in milliseconds
     */
    uint64_t period_ms() const { return period_ms_; }
    
    /**
     * @brief Get the upload bandwidth in bytes per second
     * 
     * @return Upload bandwidth in bytes per second
     */
    double upload_bandwidth() const {
        return (period_ms_ > 0) ? (bytes_sent_ * 1000.0 / period_ms_) : 0.0;
    }
    
    /**
     * @brief Get the download bandwidth in bytes per second
     * 
     * @return Download bandwidth in bytes per second
     */
    double download_bandwidth() const {
        return (period_ms_ > 0) ? (bytes_received_ * 1000.0 / period_ms_) : 0.0;
    }
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<BandwidthUsageEvent>(*this);
    }
    
    /**
     * @brief Convert the event to a string representation
     * 
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = NetworkBandwidthEvent::to_string();
        std::ostringstream oss;
        oss << base << " [Sent: " << bytes_sent_ << " bytes, Received: " << bytes_received_
            << " bytes, Period: " << period_ms_ << " ms, Upload: " << upload_bandwidth()
            << " B/s, Download: " << download_bandwidth() << " B/s]";
        return oss.str();
    }
    
private:
    uint64_t bytes_sent_;     ///< Number of bytes sent
    uint64_t bytes_received_; ///< Number of bytes received
    uint64_t period_ms_;      ///< Reporting period in milliseconds
};

/**
 * @brief Event for bandwidth limit reached
 */
class BandwidthLimitReachedEvent : public NetworkBandwidthEvent {
public:
    /**
     * @brief Create a new bandwidth limit reached event
     * 
     * @param limit_type Type of limit that was reached (upload or download)
     * @param current_bandwidth Current bandwidth in bytes per second
     * @param limit_bandwidth Bandwidth limit in bytes per second
     */
    BandwidthLimitReachedEvent(const std::string& limit_type, double current_bandwidth, double limit_bandwidth)
        : NetworkBandwidthEvent(BandwidthEventType::BANDWIDTH_LIMIT_REACHED),
          limit_type_(limit_type),
          current_bandwidth_(current_bandwidth),
          limit_bandwidth_(limit_bandwidth) {}
    
    /**
     * @brief Get the type of limit that was reached
     * 
     * @return Type of limit (upload or download)
     */
    const std::string& limit_type() const { return limit_type_; }
    
    /**
     * @brief Get the current bandwidth
     * 
     * @return Current bandwidth in bytes per second
     */
    double current_bandwidth() const { return current_bandwidth_; }
    
    /**
     * @brief Get the bandwidth limit
     * 
     * @return Bandwidth limit in bytes per second
     */
    double limit_bandwidth() const { return limit_bandwidth_; }
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<BandwidthLimitReachedEvent>(*this);
    }
    
    /**
     * @brief Convert the event to a string representation
     * 
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = NetworkBandwidthEvent::to_string();
        std::ostringstream oss;
        oss << base << " [Type: " << limit_type_ << ", Current: " << current_bandwidth_
            << " B/s, Limit: " << limit_bandwidth_ << " B/s]";
        return oss.str();
    }
    
private:
    std::string limit_type_;      ///< Type of limit (upload or download)
    double current_bandwidth_;    ///< Current bandwidth in bytes per second
    double limit_bandwidth_;      ///< Bandwidth limit in bytes per second
};

} // namespace bitscrape::network
