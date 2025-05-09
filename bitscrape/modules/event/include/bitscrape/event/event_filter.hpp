#pragma once

#include "bitscrape/types/event_types.hpp"

#include <memory>
#include <functional>

namespace bitscrape::event {

/**
 * @brief Interface for event filters
 * 
 * EventFilter is used to filter events based on various criteria.
 * It can be used to selectively process events based on type, content, or other factors.
 */
class EventFilter {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~EventFilter() = default;
    
    /**
     * @brief Check if an event passes the filter
     * 
     * @param event Event to check
     * @return true if the event passes the filter, false otherwise
     */
    virtual bool passes(const types::Event& event) const = 0;
};

/**
 * @brief Filter events by type
 */
class TypeFilter : public EventFilter {
public:
    /**
     * @brief Create a filter for a specific event type
     * 
     * @param type Event type to filter for
     */
    explicit TypeFilter(types::Event::Type type) : type_(type) {}
    
    /**
     * @brief Check if an event passes the filter
     * 
     * @param event Event to check
     * @return true if the event is of the specified type, false otherwise
     */
    bool passes(const types::Event& event) const override {
        return event.type() == type_;
    }
    
private:
    types::Event::Type type_; ///< Event type to filter for
};

/**
 * @brief Filter events using a predicate function
 */
class PredicateFilter : public EventFilter {
public:
    /**
     * @brief Create a filter with a predicate function
     * 
     * @param predicate Function that returns true for events that pass the filter
     */
    explicit PredicateFilter(std::function<bool(const types::Event&)> predicate)
        : predicate_(std::move(predicate)) {}
    
    /**
     * @brief Check if an event passes the filter
     * 
     * @param event Event to check
     * @return true if the predicate returns true for the event, false otherwise
     */
    bool passes(const types::Event& event) const override {
        return predicate_(event);
    }
    
private:
    std::function<bool(const types::Event&)> predicate_; ///< Predicate function
};

/**
 * @brief Combine multiple filters with logical AND
 */
class AndFilter : public EventFilter {
public:
    /**
     * @brief Create a filter that combines two filters with logical AND
     * 
     * @param filter1 First filter
     * @param filter2 Second filter
     */
    AndFilter(std::unique_ptr<EventFilter> filter1, std::unique_ptr<EventFilter> filter2)
        : filter1_(std::move(filter1)), filter2_(std::move(filter2)) {}
    
    /**
     * @brief Check if an event passes the filter
     * 
     * @param event Event to check
     * @return true if the event passes both filters, false otherwise
     */
    bool passes(const types::Event& event) const override {
        return filter1_->passes(event) && filter2_->passes(event);
    }
    
private:
    std::unique_ptr<EventFilter> filter1_; ///< First filter
    std::unique_ptr<EventFilter> filter2_; ///< Second filter
};

/**
 * @brief Combine multiple filters with logical OR
 */
class OrFilter : public EventFilter {
public:
    /**
     * @brief Create a filter that combines two filters with logical OR
     * 
     * @param filter1 First filter
     * @param filter2 Second filter
     */
    OrFilter(std::unique_ptr<EventFilter> filter1, std::unique_ptr<EventFilter> filter2)
        : filter1_(std::move(filter1)), filter2_(std::move(filter2)) {}
    
    /**
     * @brief Check if an event passes the filter
     * 
     * @param event Event to check
     * @return true if the event passes either filter, false otherwise
     */
    bool passes(const types::Event& event) const override {
        return filter1_->passes(event) || filter2_->passes(event);
    }
    
private:
    std::unique_ptr<EventFilter> filter1_; ///< First filter
    std::unique_ptr<EventFilter> filter2_; ///< Second filter
};

/**
 * @brief Negate a filter
 */
class NotFilter : public EventFilter {
public:
    /**
     * @brief Create a filter that negates another filter
     * 
     * @param filter Filter to negate
     */
    explicit NotFilter(std::unique_ptr<EventFilter> filter)
        : filter_(std::move(filter)) {}
    
    /**
     * @brief Check if an event passes the filter
     * 
     * @param event Event to check
     * @return true if the event does not pass the inner filter, false otherwise
     */
    bool passes(const types::Event& event) const override {
        return !filter_->passes(event);
    }
    
private:
    std::unique_ptr<EventFilter> filter_; ///< Filter to negate
};

/**
 * @brief Create a type filter
 * 
 * @param type Event type to filter for
 * @return Unique pointer to a type filter
 */
std::unique_ptr<EventFilter> create_type_filter(types::Event::Type type);

/**
 * @brief Create a predicate filter
 * 
 * @param predicate Function that returns true for events that pass the filter
 * @return Unique pointer to a predicate filter
 */
std::unique_ptr<EventFilter> create_predicate_filter(std::function<bool(const types::Event&)> predicate);

/**
 * @brief Create an AND filter
 * 
 * @param filter1 First filter
 * @param filter2 Second filter
 * @return Unique pointer to an AND filter
 */
std::unique_ptr<EventFilter> create_and_filter(std::unique_ptr<EventFilter> filter1, std::unique_ptr<EventFilter> filter2);

/**
 * @brief Create an OR filter
 * 
 * @param filter1 First filter
 * @param filter2 Second filter
 * @return Unique pointer to an OR filter
 */
std::unique_ptr<EventFilter> create_or_filter(std::unique_ptr<EventFilter> filter1, std::unique_ptr<EventFilter> filter2);

/**
 * @brief Create a NOT filter
 * 
 * @param filter Filter to negate
 * @return Unique pointer to a NOT filter
 */
std::unique_ptr<EventFilter> create_not_filter(std::unique_ptr<EventFilter> filter);

} // namespace bitscrape::event
