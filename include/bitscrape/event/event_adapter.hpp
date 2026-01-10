#pragma once

#include "bitscrape/types/event_types.hpp"
#include "bitscrape/event/event_bus.hpp"

#include <memory>
#include <vector>
#include <functional>

namespace bitscrape::event {

/**
 * @brief Interface for event adapters
 * 
 * EventAdapter connects components to the event bus by translating
 * component-specific events to system events and vice versa.
 */
class EventAdapter {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~EventAdapter() = default;
    
    /**
     * @brief Connect to the event bus
     * 
     * @param event_bus Event bus to connect to
     */
    virtual void connect(EventBus& event_bus) = 0;
    
    /**
     * @brief Disconnect from the event bus
     */
    virtual void disconnect() = 0;
    
    /**
     * @brief Check if the adapter is connected
     * 
     * @return true if the adapter is connected, false otherwise
     */
    virtual bool is_connected() const = 0;
};

/**
 * @brief Base implementation of an event adapter
 */
class EventAdapterBase : public EventAdapter {
public:
    /**
     * @brief Constructor
     */
    EventAdapterBase() : connected_(false), event_bus_(nullptr) {}
    
    /**
     * @brief Destructor
     */
    ~EventAdapterBase() override {
        disconnect();
    }
    
    /**
     * @brief Connect to the event bus
     * 
     * @param event_bus Event bus to connect to
     */
    void connect(EventBus& event_bus) override {
        if (connected_) {
            disconnect();
        }
        
        event_bus_ = &event_bus;
        connected_ = true;
        
        // Register event handlers
        register_handlers();
    }
    
    /**
     * @brief Disconnect from the event bus
     */
    void disconnect() override {
        if (!connected_) {
            return;
        }
        
        // Unsubscribe from all events
        for (const auto& token : tokens_) {
            event_bus_->unsubscribe(token);
        }
        
        tokens_.clear();
        event_bus_ = nullptr;
        connected_ = false;
    }
    
    /**
     * @brief Check if the adapter is connected
     * 
     * @return true if the adapter is connected, false otherwise
     */
    bool is_connected() const override {
        return connected_;
    }
    
protected:
    /**
     * @brief Register event handlers
     * 
     * This method should be overridden by derived classes to register
     * event handlers with the event bus.
     */
    virtual void register_handlers() = 0;
    
    /**
     * @brief Subscribe to events of a specific type
     * 
     * @tparam T Event type
     * @param handler Event handler function
     */
    template<typename T>
    void subscribe(types::EventHandler<T> handler) {
        if (!connected_ || !event_bus_) {
            return;
        }
        
        // Subscribe to the event and store the token
        auto token = event_bus_->subscribe<T>(handler);
        tokens_.push_back(token);
    }
    
    /**
     * @brief Publish an event
     * 
     * @tparam T Event type
     * @param event Event to publish
     */
    template<typename T>
    void publish(const T& event) {
        if (!connected_ || !event_bus_) {
            return;
        }
        
        // Publish the event
        event_bus_->publish(event);
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
        if (!connected_ || !event_bus_) {
            // Return a future that is already completed
            std::promise<void> promise;
            promise.set_value();
            return promise.get_future();
        }
        
        // Publish the event asynchronously
        return event_bus_->publish_async(event);
    }
    
private:
    bool connected_; ///< Flag indicating if the adapter is connected
    EventBus* event_bus_; ///< Event bus to connect to
    std::vector<types::SubscriptionToken> tokens_; ///< Subscription tokens
};

/**
 * @brief Create a new event adapter
 * 
 * @return Unique pointer to a new event adapter
 */
std::unique_ptr<EventAdapter> create_event_adapter();

} // namespace bitscrape::event
