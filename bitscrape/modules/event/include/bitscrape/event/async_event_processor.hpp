#pragma once

#include "bitscrape/event/event_processor.hpp"
#include "bitscrape/event/event_filter.hpp"

#include <memory>
#include <vector>
#include <future>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace bitscrape::event {

/**
 * @brief Asynchronous event processor
 * 
 * AsyncEventProcessor processes events asynchronously using a thread pool.
 * It can filter, transform, and route events to appropriate handlers.
 */
class AsyncEventProcessor : public EventProcessor {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~AsyncEventProcessor() = default;
    
    /**
     * @brief Set the number of worker threads
     * 
     * @param num_threads Number of worker threads
     */
    virtual void set_num_threads(size_t num_threads) = 0;
    
    /**
     * @brief Get the number of worker threads
     * 
     * @return Number of worker threads
     */
    virtual size_t num_threads() const = 0;
    
    /**
     * @brief Set the event filter
     * 
     * @param filter Event filter
     */
    virtual void set_filter(std::unique_ptr<EventFilter> filter) = 0;
    
    /**
     * @brief Clear the event filter
     */
    virtual void clear_filter() = 0;
    
    /**
     * @brief Get the number of events in the queue
     * 
     * @return Number of events in the queue
     */
    virtual size_t queue_size() const = 0;
    
    /**
     * @brief Wait for all events to be processed
     * 
     * @param timeout_ms Timeout in milliseconds (0 means wait indefinitely)
     * @return true if all events were processed, false if the timeout expired
     */
    virtual bool wait_for_empty_queue(uint64_t timeout_ms = 0) = 0;
};

/**
 * @brief Create a new asynchronous event processor
 * 
 * @param num_threads Number of worker threads (0 means use the number of hardware threads)
 * @return Unique pointer to a new asynchronous event processor
 */
std::unique_ptr<AsyncEventProcessor> create_async_event_processor(size_t num_threads = 0);

} // namespace bitscrape::event
