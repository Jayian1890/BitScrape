#include "bitscrape/event/event_bus.hpp"

#include <algorithm>

namespace bitscrape::event {

/**
 * @brief Implementation of the event bus
 */
class EventBusImpl : public EventBus {
public:
    /**
     * @brief Constructor
     */
    EventBusImpl() = default;
    
    /**
     * @brief Destructor
     */
    ~EventBusImpl() override = default;
    
    /**
     * @brief Unsubscribe from events
     * 
     * @param token Subscription token returned from subscribe
     * @return true if the subscription was found and removed, false otherwise
     */
    bool unsubscribe(const types::SubscriptionToken& token) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Search for the token in all handler lists
        for (auto& [type, handlers] : handlers_) {
            auto it = std::find_if(handlers.begin(), handlers.end(),
                [&token](const HandlerEntry& entry) {
                    return entry.token == token;
                });
            
            if (it != handlers.end()) {
                // Remove the handler
                handlers.erase(it);
                return true;
            }
        }
        
        return false;
    }
    
protected:
    /**
     * @brief Publish an event to all registered handlers
     * 
     * @param event Event to publish
     */
    void publish_event(const types::Event& event) override {
        // Make a copy of the handlers to avoid holding the lock while calling them
        std::vector<HandlerEntry> handlers_to_call;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            // Get the handlers for this event type
            std::type_index type_index = std::type_index(typeid(event));
            auto it = handlers_.find(type_index);
            
            if (it != handlers_.end()) {
                // Copy the handlers
                handlers_to_call = it->second;
            }
        }
        
        // Call all handlers
        for (const auto& entry : handlers_to_call) {
            entry.handler(event);
        }
    }
};

std::unique_ptr<EventBus> create_event_bus() {
    return std::make_unique<EventBusImpl>();
}

} // namespace bitscrape::event
