#pragma once

#include <unordered_map>
#include <typeindex>
#include <functional>

#include "bitscrape/event/event_adapter.hpp"
#include "bitscrape/beacon/beacon.hpp"

namespace bitscrape::beacon {

/**
 * @brief Adapter for connecting the event system to the beacon system
 *
 * BeaconAdapter connects the event system to the beacon system by subscribing to events
 * and logging them using the beacon.
 */
class BeaconAdapter : public event::EventAdapterBase {
public:
    /**
     * @brief Create a beacon adapter
     *
     * @param beacon Beacon to log events to
     */
    explicit BeaconAdapter(Beacon& beacon);

    /**
     * @brief Destructor
     */
    ~BeaconAdapter() override = default;

    /**
     * @brief Add a mapping from an event type to a beacon severity and category
     *
     * @tparam T Event type
     * @param severity Beacon severity level
     * @param category Beacon category
     */
    template<typename T>
    void add_event_mapping(types::BeaconSeverity severity, types::BeaconCategory category) {
        static_assert(std::is_base_of<types::Event, T>::value, "T must be derived from Event");

        std::type_index type_index = std::type_index(typeid(T));
        event_mappings_[type_index] = std::make_pair(severity, category);
    }

    /**
     * @brief Add a formatter for an event type
     *
     * @tparam T Event type
     * @param formatter Function that formats an event of type T to a string
     */
    template<typename T>
    void add_event_formatter(std::function<std::string(const T&)> formatter) {
        static_assert(std::is_base_of<types::Event, T>::value, "T must be derived from Event");

        std::type_index type_index = std::type_index(typeid(T));
        event_formatters_[type_index] = [formatter](const types::Event& event) {
            return formatter(static_cast<const T&>(event));
        };
    }

    /**
     * @brief Handle an event
     *
     * @param event Event to handle
     */
    void handle_event(const types::Event& event);

protected:
    /**
     * @brief Register event handlers
     */
    void register_handlers() override;

private:
    Beacon& beacon_; ///< Beacon to log events to
    std::unordered_map<std::type_index, std::pair<types::BeaconSeverity, types::BeaconCategory>> event_mappings_; ///< Event type to beacon severity and category mappings
    std::unordered_map<std::type_index, std::function<std::string(const types::Event&)>> event_formatters_; ///< Event type to formatter mappings
};

/**
 * @brief Create a new beacon adapter
 *
 * @param beacon Beacon to log events to
 * @return Unique pointer to a new beacon adapter
 */
std::unique_ptr<BeaconAdapter> create_beacon_adapter(Beacon& beacon);

} // namespace bitscrape::beacon
