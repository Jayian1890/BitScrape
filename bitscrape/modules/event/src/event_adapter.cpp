#include "bitscrape/event/event_adapter.hpp"

namespace bitscrape::event {

/**
 * @brief Implementation of the event adapter
 */
class EventAdapterImpl : public EventAdapterBase {
public:
    /**
     * @brief Constructor
     */
    EventAdapterImpl() = default;
    
    /**
     * @brief Destructor
     */
    ~EventAdapterImpl() override = default;
    
protected:
    /**
     * @brief Register event handlers
     */
    void register_handlers() override {
        // This is a base implementation that doesn't register any handlers
        // Derived classes should override this method to register their handlers
    }
};

std::unique_ptr<EventAdapter> create_event_adapter() {
    return std::make_unique<EventAdapterImpl>();
}

} // namespace bitscrape::event
