#include "bitscrape/event/async_event_processor.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace bitscrape::event {

/**
 * @brief Implementation of the asynchronous event processor
 */
class AsyncEventProcessorImpl : public AsyncEventProcessor {
public:
  /**
   * @brief Constructor
   *
   * @param num_threads Number of worker threads (0 means use the number of
   * hardware threads)
   */
  explicit AsyncEventProcessorImpl(size_t num_threads = 0)
      : running_(false), event_bus_(nullptr), token_(0) {
    // If num_threads is 0, use the number of hardware threads
    if (num_threads == 0) {
      num_threads = std::thread::hardware_concurrency();
      // If hardware_concurrency returns 0, use 2 threads
      if (num_threads == 0) {
        num_threads = 2;
      }
    }

    set_num_threads(num_threads);
  }

  /**
   * @brief Destructor
   */
  ~AsyncEventProcessorImpl() override { stop(); }

  /**
   * @brief Start processing events
   *
   * @param event_bus Event bus to process events from
   */
  void start(EventBus &event_bus) override {
    if (running_) {
      return;
    }

    running_ = true;
    event_bus_ = &event_bus;

    // Subscribe to all events
    token_ =
        event_bus_->subscribe<types::Event>([this](const types::Event &event) {
          // Add the event to the queue
          this->enqueue_event(event.clone());
        });

    // Start worker threads
    workers_.resize(num_threads_);
    for (size_t i = 0; i < num_threads_; ++i) {
      workers_[i] = std::thread(&AsyncEventProcessorImpl::worker_thread, this);
    }
  }

  /**
   * @brief Stop processing events
   */
  void stop() override {
    if (!running_) {
      return;
    }

    running_ = false;

    // Unsubscribe from the event bus
    if (event_bus_) {
      event_bus_->unsubscribe(token_);
      event_bus_ = nullptr;
    }

    // Wake up all worker threads
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      queue_cv_.notify_all();
    }

    // Wait for all worker threads to finish
    for (auto &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }

    workers_.clear();

    // Clear the event queue
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      while (!event_queue_.empty()) {
        event_queue_.pop();
      }
    }
  }

  /**
   * @brief Check if the processor is running
   *
   * @return true if the processor is running, false otherwise
   */
  bool is_running() const override { return running_; }

  /**
   * @brief Process an event
   *
   * @param event Event to process
   */
  void process(const types::Event &event) override {
    if (!running_) {
      return;
    }

    // Check if the event passes the filter
    bool passes = true;
    {
      std::lock_guard<std::mutex> lock(filter_mutex_);
      if (filter_) {
        passes = filter_->passes(event);
      }
    }

    if (!passes) {
      return;
    }

    // Process the event
    process_event(event);
  }

  /**
   * @brief Process an event asynchronously
   *
   * @param event Event to process
   * @return Future that will be completed when the event has been processed
   */
  std::future<void> process_async(const types::Event &event) override {
    return std::async(std::launch::async, [this, event = event.clone()]() {
      this->process(*event);
    });
  }

  /**
   * @brief Set the number of worker threads
   *
   * @param num_threads Number of worker threads
   */
  void set_num_threads(size_t num_threads) override {
    if (running_) {
      // Stop and restart with the new number of threads
      stop();
      num_threads_ = num_threads;
      if (event_bus_) {
        start(*event_bus_);
      }
    } else {
      num_threads_ = num_threads;
    }
  }

  /**
   * @brief Get the number of worker threads
   *
   * @return Number of worker threads
   */
  size_t num_threads() const override { return num_threads_; }

  /**
   * @brief Set the event filter
   *
   * @param filter Event filter
   */
  void set_filter(std::unique_ptr<EventFilter> filter) override {
    std::lock_guard<std::mutex> lock(filter_mutex_);
    filter_ = std::move(filter);
  }

  /**
   * @brief Clear the event filter
   */
  void clear_filter() override {
    std::lock_guard<std::mutex> lock(filter_mutex_);
    filter_.reset();
  }

  /**
   * @brief Get the number of events in the queue
   *
   * @return Number of events in the queue
   */
  size_t queue_size() const override {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return event_queue_.size();
  }

  /**
   * @brief Wait for all events to be processed
   *
   * @param timeout_ms Timeout in milliseconds (0 means wait indefinitely)
   * @return true if all events were processed, false if the timeout expired
   */
  bool wait_for_empty_queue(uint64_t timeout_ms = 0) override {
    if (!running_) {
      return true;
    }

    std::unique_lock<std::mutex> lock(queue_mutex_);

    if (timeout_ms == 0) {
      // Wait indefinitely
      queue_empty_cv_.wait(lock, [this]() { return event_queue_.empty(); });
      return true;
    } else {
      // Wait with timeout
      return queue_empty_cv_.wait_for(
          lock, std::chrono::milliseconds(timeout_ms),
          [this]() { return event_queue_.empty(); });
    }
  }

private:
  /**
   * @brief Worker thread function
   */
  void worker_thread() {
    while (running_) {
      // Get an event from the queue
      std::unique_ptr<types::Event> event;

      {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // Wait for an event or for the processor to stop
        queue_cv_.wait(lock,
                       [this]() { return !running_ || !event_queue_.empty(); });

        // If the processor is stopping, exit
        if (!running_) {
          break;
        }

        // Get the event from the queue
        if (!event_queue_.empty()) {
          event = std::move(event_queue_.front());
          event_queue_.pop();

          // Notify if the queue is empty
          if (event_queue_.empty()) {
            queue_empty_cv_.notify_all();
          }
        }
      }

      // Process the event
      if (event) {
        // Check if the event passes the filter
        bool passes = true;
        {
          std::lock_guard<std::mutex> lock(filter_mutex_);
          if (filter_) {
            passes = filter_->passes(*event);
          }
        }

        if (passes) {
          process_event(*event);
        }
      }
    }
  }

  /**
   * @brief Enqueue an event for processing
   *
   * @param event Event to enqueue
   */
  void enqueue_event(std::unique_ptr<types::Event> event) {
    if (!running_) {
      return;
    }

    // Check if the event passes the filter
    bool passes = true;
    {
      std::lock_guard<std::mutex> lock(filter_mutex_);
      if (filter_) {
        passes = filter_->passes(*event);
      }
    }

    if (!passes) {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      event_queue_.push(std::move(event));
    }

    // Notify a worker thread
    queue_cv_.notify_one();
  }

  /**
   * @brief Process an event
   *
   * @param event Event to process
   */
  void process_event(const types::Event &event) {
    // This is a base implementation that doesn't do anything with the event
    // Derived classes can override this method to process events
  }

  std::atomic<bool> running_; ///< Flag indicating if the processor is running
  EventBus *event_bus_;       ///< Event bus to process events from
  types::SubscriptionToken token_;   ///< Subscription token for the event bus
  size_t num_threads_;               ///< Number of worker threads
  std::vector<std::thread> workers_; ///< Worker threads
  std::queue<std::unique_ptr<types::Event>> event_queue_; ///< Event queue
  mutable std::mutex queue_mutex_;   ///< Mutex for the event queue
  std::condition_variable queue_cv_; ///< Condition variable for the event queue
  std::condition_variable
      queue_empty_cv_; ///< Condition variable for an empty queue
  std::unique_ptr<EventFilter> filter_; ///< Event filter
  mutable std::mutex filter_mutex_;     ///< Mutex for the event filter
};

std::unique_ptr<AsyncEventProcessor>
create_async_event_processor(size_t num_threads) {
  return std::make_unique<AsyncEventProcessorImpl>(num_threads);
}

} // namespace bitscrape::event
