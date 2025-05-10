#pragma once

#include "bitscrape/beacon/beacon_sink.hpp"
#include "bitscrape/event/event_bus.hpp"

namespace bitscrape::beacon {

/**
 * @brief Event output destination for beacon messages
 * 
 * EventSink publishes beacon messages as events to the event bus.
 */
class EventSink : public BeaconSink {
public:
    /**
     * @brief Create an event sink
     * 
     * @param event_bus Event bus to publish events to
     */
    explicit EventSink(event::EventBus& event_bus);
    
    /**
     * @brief Destructor
     */
    ~EventSink() override = default;
    
    /**
     * @brief Write a beacon message as an event
     * 
     * @param severity Beacon severity level
     * @param category Beacon category
     * @param message Beacon message
     * @param location Source location where the beacon was emitted
     */
    void write(types::BeaconSeverity severity, types::BeaconCategory category, 
              const std::string& message, const std::source_location& location) override;
    
    /**
     * @brief Write a beacon message as an event asynchronously
     * 
     * @param severity Beacon severity level
     * @param category Beacon category
     * @param message Beacon message
     * @param location Source location where the beacon was emitted
     * @return Future that will be completed when the event has been published
     */
    std::future<void> write_async(types::BeaconSeverity severity, types::BeaconCategory category, 
                                const std::string& message, const std::source_location& location) override;
    
private:
    event::EventBus& event_bus_; ///< Event bus to publish events to
};

/**
 * @brief Create a new event sink
 * 
 * @param event_bus Event bus to publish events to
 * @return Unique pointer to a new event sink
 */
std::unique_ptr<EventSink> create_event_sink(event::EventBus& event_bus);

} // namespace bitscrape::beacon
