#include "bitscrape/event/event_processor.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace bitscrape::event {

/**
 * @brief Implementation of the event processor
 */
class EventProcessorImpl : public EventProcessor {
public:
  /**
   * @brief Constructor
   */
  EventProcessorImpl() : running_(false), token_(0) {}

  /**
   * @brief Destructor
   */
  ~EventProcessorImpl() override { stop(); }

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
    token_ = event_bus_->subscribe<types::Event>(
        [this](const types::Event &event) { this->process(event); });
  }

  /**
   * @brief Stop processing events
   */
  void stop() override {
    if (!running_) {
      return;
    }

    running_ = false;

    if (event_bus_) {
      event_bus_->unsubscribe(token_);
      event_bus_ = nullptr;
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

    // Make a copy of the handlers to avoid holding the lock while calling them
    std::vector<std::function<void(const types::Event &)>> handlers_to_call;

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
    for (const auto &handler : handlers_to_call) {
      handler(event);
    }
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
   * @brief Register a handler for a specific event type
   *
   * @tparam T Event type
   * @param handler Handler function
   */
  template <typename T>
  void register_handler(std::function<void(const T &)> handler) {
    static_assert(std::is_base_of<types::Event, T>::value,
                  "T must be derived from Event");

    std::lock_guard<std::mutex> lock(mutex_);

    // Create a type-erased handler that will call the typed handler
    auto type_erased_handler = [handler](const types::Event &event) {
      // Cast the event to the correct type and call the handler
      const T &typed_event = static_cast<const T &>(event);
      handler(typed_event);
    };

    // Store the handler
    std::type_index type_index = std::type_index(typeid(T));
    handlers_[type_index].push_back(type_erased_handler);
  }

private:
  std::atomic<bool> running_; ///< Flag indicating if the processor is running
  EventBus *event_bus_ = nullptr;  ///< Event bus to process events from
  types::SubscriptionToken token_; ///< Subscription token for the event bus
  std::unordered_map<std::type_index,
                     std::vector<std::function<void(const types::Event &)>>>
      handlers_;     ///< Event handlers by type
  std::mutex mutex_; ///< Mutex for thread safety
};

std::unique_ptr<EventProcessor> create_event_processor() {
  return std::make_unique<EventProcessorImpl>();
}

} // namespace bitscrape::event
