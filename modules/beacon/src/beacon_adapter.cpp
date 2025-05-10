#include "bitscrape/beacon/beacon_adapter.hpp"

namespace bitscrape::beacon {

BeaconAdapter::BeaconAdapter(Beacon& beacon)
    : beacon_(beacon) {
}

void BeaconAdapter::register_handlers() {
    // Subscribe to all events
    subscribe<types::Event>([this](const types::Event& event) {
        this->handle_event(event);
    });
}

void BeaconAdapter::handle_event(const types::Event& event) {
    // Get the event type
    std::type_index type_index = std::type_index(typeid(event));
    
    // Check if we have a mapping for this event type
    auto mapping_it = event_mappings_.find(type_index);
    if (mapping_it == event_mappings_.end()) {
        // No mapping, use default (INFO, GENERAL)
        types::BeaconSeverity severity = types::BeaconSeverity::INFO;
        types::BeaconCategory category = types::BeaconCategory::GENERAL;
        
        // Format the event
        std::string message = event.to_string();
        
        // Log the event
        beacon_.log(severity, message, category);
        return;
    }
    
    // Use the mapping
    types::BeaconSeverity severity = mapping_it->second.first;
    types::BeaconCategory category = mapping_it->second.second;
    
    // Check if we have a formatter for this event type
    std::string message;
    auto formatter_it = event_formatters_.find(type_index);
    if (formatter_it != event_formatters_.end()) {
        // Use the formatter
        message = formatter_it->second(event);
    } else {
        // Use default formatting
        message = event.to_string();
    }
    
    // Log the event
    beacon_.log(severity, message, category);
}

std::unique_ptr<BeaconAdapter> create_beacon_adapter(Beacon& beacon) {
    return std::make_unique<BeaconAdapter>(beacon);
}

} // namespace bitscrape::beacon
