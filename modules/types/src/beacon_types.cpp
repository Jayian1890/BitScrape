#include "bitscrape/types/beacon_types.hpp"

#include <sstream>
#include <iomanip>

namespace bitscrape::types {

std::string get_severity_color(BeaconSeverity severity) {
    switch (severity) {
        case BeaconSeverity::DEBUG:
            return "\033[36m"; // Cyan
        case BeaconSeverity::INFO:
            return "\033[32m"; // Green
        case BeaconSeverity::WARNING:
            return "\033[33m"; // Yellow
        case BeaconSeverity::ERROR:
            return "\033[31m"; // Red
        case BeaconSeverity::CRITICAL:
            return "\033[35m"; // Magenta
        default:
            return "\033[0m";  // Reset
    }
}

std::string get_reset_color() {
    return "\033[0m";
}

std::string severity_to_string(BeaconSeverity severity) {
    switch (severity) {
        case BeaconSeverity::DEBUG:
            return "DEBUG";
        case BeaconSeverity::INFO:
            return "INFO";
        case BeaconSeverity::WARNING:
            return "WARNING";
        case BeaconSeverity::ERROR:
            return "ERROR";
        case BeaconSeverity::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

std::string category_to_string(BeaconCategory category) {
    switch (category) {
        case BeaconCategory::GENERAL:
            return "GENERAL";
        case BeaconCategory::SYSTEM:
            return "SYSTEM";
        case BeaconCategory::NETWORK:
            return "NETWORK";
        case BeaconCategory::DHT:
            return "DHT";
        case BeaconCategory::BITTORRENT:
            return "BITTORRENT";
        case BeaconCategory::TRACKER:
            return "TRACKER";
        case BeaconCategory::DATABASE:
            return "DATABASE";
        case BeaconCategory::UI:
            return "UI";
        default:
            return "UNKNOWN";
    }
}

BeaconEvent::BeaconEvent(BeaconSeverity severity, BeaconCategory category, const std::string& message,
                         const std::source_location& location)
    : Event(Event::Type::USER_DEFINED, 1001), // Use a custom type ID for beacon events
      severity_(severity),
      category_(category),
      message_(message),
      location_(location) {
}

std::unique_ptr<Event> BeaconEvent::clone() const {
    return std::make_unique<BeaconEvent>(*this);
}

std::string BeaconEvent::to_string() const {
    std::ostringstream oss;
    
    // Get the base event string (includes timestamp)
    oss << Event::to_string() << " ";
    
    // Add severity and category
    oss << "[" << severity_to_string(severity_) << "] "
        << "[" << category_to_string(category_) << "] ";
    
    // Add message
    oss << message_ << " ";
    
    // Add source location
    oss << "(" << location_.file_name() << ":" << location_.line() << ":" << location_.function_name() << ")";
    
    return oss.str();
}

} // namespace bitscrape::types
