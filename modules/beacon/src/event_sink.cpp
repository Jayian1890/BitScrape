#include "bitscrape/beacon/event_sink.hpp"

namespace bitscrape::beacon {

EventSink::EventSink(event::EventBus& event_bus)
    : event_bus_(event_bus) {
}

void EventSink::write(types::BeaconSeverity severity, types::BeaconCategory category, 
                     const std::string& message, const std::source_location& location) {
    if (!should_log(severity, category)) {
        return;
    }
    
    // Create and publish a BeaconEvent
    types::BeaconEvent event(severity, category, message, location);
    event_bus_.publish(event);
}

std::future<void> EventSink::write_async(types::BeaconSeverity severity, types::BeaconCategory category, 
                                       const std::string& message, const std::source_location& location) {
    if (!should_log(severity, category)) {
        // Return a future that's already satisfied
        std::promise<void> promise;
        promise.set_value();
        return promise.get_future();
    }
    
    // Create a BeaconEvent
    types::BeaconEvent event(severity, category, message, location);
    
    // Publish the event asynchronously
    return event_bus_.publish_async(event);
}

std::unique_ptr<EventSink> create_event_sink(event::EventBus& event_bus) {
    return std::make_unique<EventSink>(event_bus);
}

} // namespace bitscrape::beacon
