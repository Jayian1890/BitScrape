#pragma once

#include "bitscrape/types/event_types.hpp"

#include <memory>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <mutex>
#include <future>
#include <functional>

namespace bitscrape::event {

/**
 * @brief Interface for the event bus
 * 
 * The EventBus is the central component of the event system, responsible for
 * registering event handlers, dispatching events to registered handlers,
 * and managing subscriptions.
 */
class EventBus {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~EventBus() = default;
    
    /**
     * @brief Subscribe to events of a specific type
     * 
     * @tparam T Event type
     * @param handler Event handler function
     * @return Subscription token that can be used to unsubscribe
     */
    template<typename T>
    types::SubscriptionToken subscribe(types::EventHandler<T> handler) {
        static_assert(std::is_base_of<types::Event, T>::value, "T must be derived from Event");
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Create a type-erased handler that will call the typed handler
        auto type_erased_handler = [handler](const types::Event& event) {
            // Cast the event to the correct type and call the handler
            const T& typed_event = static_cast<const T&>(event);
            handler(typed_event);
        };
        
        // Generate a new token ID
        uint64_t token_id = next_token_id_++;
        types::SubscriptionToken token(token_id);
        
        // Store the handler with the token
        std::type_index type_index = std::type_index(typeid(T));
        handlers_[type_index].emplace_back(token, type_erased_handler);
        
        return token;
    }
    
    /**
     * @brief Unsubscribe from events
     * 
     * @param token Subscription token returned from subscribe
     * @return true if the subscription was found and removed, false otherwise
     */
    virtual bool unsubscribe(const types::SubscriptionToken& token) = 0;
    
    /**
     * @brief Publish an event
     * 
     * @tparam T Event type
     * @param event Event to publish
     */
    template<typename T>
    void publish(const T& event) {
        static_assert(std::is_base_of<types::Event, T>::value, "T must be derived from Event");
        
        // Call the virtual publish_event method with the event
        publish_event(event);
    }
    
    /**
     * @brief Publish an event asynchronously
     * 
     * @tparam T Event type
     * @param event Event to publish
     * @return Future that will be completed when the event has been published
     */
    template<typename T>
    std::future<void> publish_async(const T& event) {
        static_assert(std::is_base_of<types::Event, T>::value, "T must be derived from Event");
        
        // Use std::async to publish the event asynchronously
        return std::async(std::launch::async, [this, event]() {
            this->publish(event);
        });
    }
    
protected:
    /**
     * @brief Publish an event to all registered handlers
     * 
     * @param event Event to publish
     */
    virtual void publish_event(const types::Event& event) = 0;
    
    /**
     * @brief Handler entry
     */
    struct HandlerEntry {
        types::SubscriptionToken token;
        std::function<void(const types::Event&)> handler;
        
        HandlerEntry(types::SubscriptionToken t, std::function<void(const types::Event&)> h)
            : token(t), handler(h) {}
    };
    
    std::unordered_map<std::type_index, std::vector<HandlerEntry>> handlers_; ///< Event handlers by type
    uint64_t next_token_id_ = 1; ///< Next token ID
    std::mutex mutex_; ///< Mutex for thread safety
};

/**
 * @brief Create a new event bus
 * 
 * @return Unique pointer to a new event bus
 */
std::unique_ptr<EventBus> create_event_bus();

} // namespace bitscrape::event
