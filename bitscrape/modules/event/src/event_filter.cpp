#include "bitscrape/event/event_filter.hpp"

namespace bitscrape::event {

std::unique_ptr<EventFilter> create_type_filter(types::Event::Type type) {
    return std::make_unique<TypeFilter>(type);
}

std::unique_ptr<EventFilter> create_predicate_filter(std::function<bool(const types::Event&)> predicate) {
    return std::make_unique<PredicateFilter>(std::move(predicate));
}

std::unique_ptr<EventFilter> create_and_filter(std::unique_ptr<EventFilter> filter1, std::unique_ptr<EventFilter> filter2) {
    return std::make_unique<AndFilter>(std::move(filter1), std::move(filter2));
}

std::unique_ptr<EventFilter> create_or_filter(std::unique_ptr<EventFilter> filter1, std::unique_ptr<EventFilter> filter2) {
    return std::make_unique<OrFilter>(std::move(filter1), std::move(filter2));
}

std::unique_ptr<EventFilter> create_not_filter(std::unique_ptr<EventFilter> filter) {
    return std::make_unique<NotFilter>(std::move(filter));
}

} // namespace bitscrape::event
