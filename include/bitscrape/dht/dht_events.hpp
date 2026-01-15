#pragma once

#include "bitscrape/types/event_types.hpp"
#include "bitscrape/types/info_hash.hpp"
#include <string>

namespace bitscrape::dht {

/**
 * @brief Base class for DHT events
 */
class DHTEvent : public types::Event {
public:
    using types::Event::Event;
    ~DHTEvent() override = default;
};

/**
 * @brief Event emitted when an infohash is found via DHT
 */
class DHTInfoHashFoundEvent : public DHTEvent {
public:
    explicit DHTInfoHashFoundEvent(const types::InfoHash& info_hash)
        : DHTEvent(types::Event::Type::DHT_INFOHASH_FOUND), info_hash_(info_hash) {}

    const types::InfoHash& info_hash() const { return info_hash_; }

    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<DHTInfoHashFoundEvent>(*this);
    }

    std::string to_string() const override {
        return DHTEvent::to_string() + ": " + info_hash_.to_hex();
    }

private:
    types::InfoHash info_hash_;
};

} // namespace bitscrape::dht
