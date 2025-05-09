#pragma once

#include "bitscrape/types/event_types.hpp"
#include "bitscrape/event/event_bus.hpp"

#include <memory>
#include <vector>
#include <future>

namespace bitscrape::event {

/**
 * @brief Interface for event processors
 * 
 * EventProcessor is responsible for processing events from the event bus.
 * It can filter, transform, and route events to appropriate handlers.
 */
class EventProcessor {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~EventProcessor() = default;
    
    /**
     * @brief Start processing events
     * 
     * @param event_bus Event bus to process events from
     */
    virtual void start(EventBus& event_bus) = 0;
    
    /**
     * @brief Stop processing events
     */
    virtual void stop() = 0;
    
    /**
     * @brief Check if the processor is running
     * 
     * @return true if the processor is running, false otherwise
     */
    virtual bool is_running() const = 0;
    
    /**
     * @brief Process an event
     * 
     * @param event Event to process
     */
    virtual void process(const types::Event& event) = 0;
    
    /**
     * @brief Process an event asynchronously
     * 
     * @param event Event to process
     * @return Future that will be completed when the event has been processed
     */
    virtual std::future<void> process_async(const types::Event& event) = 0;
};

/**
 * @brief Create a new event processor
 * 
 * @return Unique pointer to a new event processor
 */
std::unique_ptr<EventProcessor> create_event_processor();

} // namespace bitscrape::event
