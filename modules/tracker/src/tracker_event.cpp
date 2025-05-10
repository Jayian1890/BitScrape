#include "bitscrape/tracker/tracker_event.hpp"

#include <sstream>

namespace bitscrape::tracker {

// TrackerEvent implementation
TrackerEvent::TrackerEvent(TrackerEventType type)
    : types::Event(types::Event::Type::USER_DEFINED, static_cast<uint32_t>(type)),
      tracker_event_type_(type) {
}

TrackerEventType TrackerEvent::tracker_event_type() const {
    return tracker_event_type_;
}

// AnnounceRequestEvent implementation
AnnounceRequestEvent::AnnounceRequestEvent(
    const types::InfoHash& info_hash,
    const std::string& peer_id,
    uint16_t port,
    uint64_t uploaded,
    uint64_t downloaded,
    uint64_t left,
    const std::string& event,
    const std::string& ip,
    int numwant,
    const std::string& key,
    const std::string& tracker_id
)
    : TrackerEvent(TrackerEventType::ANNOUNCE_REQUEST),
      info_hash_(info_hash),
      peer_id_(peer_id),
      port_(port),
      uploaded_(uploaded),
      downloaded_(downloaded),
      left_(left),
      event_(event),
      ip_(ip),
      numwant_(numwant),
      key_(key),
      tracker_id_(tracker_id) {
}

const types::InfoHash& AnnounceRequestEvent::info_hash() const {
    return info_hash_;
}

const std::string& AnnounceRequestEvent::peer_id() const {
    return peer_id_;
}

uint16_t AnnounceRequestEvent::port() const {
    return port_;
}

uint64_t AnnounceRequestEvent::uploaded() const {
    return uploaded_;
}

uint64_t AnnounceRequestEvent::downloaded() const {
    return downloaded_;
}

uint64_t AnnounceRequestEvent::left() const {
    return left_;
}

const std::string& AnnounceRequestEvent::event() const {
    return event_;
}

const std::string& AnnounceRequestEvent::ip() const {
    return ip_;
}

int AnnounceRequestEvent::numwant() const {
    return numwant_;
}

const std::string& AnnounceRequestEvent::key() const {
    return key_;
}

const std::string& AnnounceRequestEvent::tracker_id() const {
    return tracker_id_;
}

std::unique_ptr<types::Event> AnnounceRequestEvent::clone() const {
    return std::make_unique<AnnounceRequestEvent>(*this);
}

// AnnounceResponseEvent implementation
AnnounceResponseEvent::AnnounceResponseEvent(
    const types::InfoHash& info_hash,
    int interval,
    int min_interval,
    const std::string& tracker_id,
    int complete,
    int incomplete,
    const std::vector<network::Address>& peers
)
    : TrackerEvent(TrackerEventType::ANNOUNCE_RESPONSE),
      info_hash_(info_hash),
      interval_(interval),
      min_interval_(min_interval),
      tracker_id_(tracker_id),
      complete_(complete),
      incomplete_(incomplete),
      peers_(peers) {
}

const types::InfoHash& AnnounceResponseEvent::info_hash() const {
    return info_hash_;
}

int AnnounceResponseEvent::interval() const {
    return interval_;
}

int AnnounceResponseEvent::min_interval() const {
    return min_interval_;
}

const std::string& AnnounceResponseEvent::tracker_id() const {
    return tracker_id_;
}

int AnnounceResponseEvent::complete() const {
    return complete_;
}

int AnnounceResponseEvent::incomplete() const {
    return incomplete_;
}

const std::vector<network::Address>& AnnounceResponseEvent::peers() const {
    return peers_;
}

std::unique_ptr<types::Event> AnnounceResponseEvent::clone() const {
    return std::make_unique<AnnounceResponseEvent>(*this);
}

// ScrapeRequestEvent implementation
ScrapeRequestEvent::ScrapeRequestEvent(const std::vector<types::InfoHash>& info_hashes)
    : TrackerEvent(TrackerEventType::SCRAPE_REQUEST),
      info_hashes_(info_hashes) {
}

const std::vector<types::InfoHash>& ScrapeRequestEvent::info_hashes() const {
    return info_hashes_;
}

std::unique_ptr<types::Event> ScrapeRequestEvent::clone() const {
    return std::make_unique<ScrapeRequestEvent>(*this);
}

// ScrapeResponseEvent implementation
ScrapeResponseEvent::ScrapeResponseEvent(const std::map<types::InfoHash, ScrapeData>& files)
    : TrackerEvent(TrackerEventType::SCRAPE_RESPONSE),
      files_(files) {
}

const std::map<types::InfoHash, ScrapeResponseEvent::ScrapeData>& ScrapeResponseEvent::files() const {
    return files_;
}

std::unique_ptr<types::Event> ScrapeResponseEvent::clone() const {
    return std::make_unique<ScrapeResponseEvent>(*this);
}

// TrackerErrorEvent implementation
TrackerErrorEvent::TrackerErrorEvent(const std::string& error_message)
    : TrackerEvent(TrackerEventType::ERROR),
      error_message_(error_message) {
}

const std::string& TrackerErrorEvent::error_message() const {
    return error_message_;
}

std::unique_ptr<types::Event> TrackerErrorEvent::clone() const {
    return std::make_unique<TrackerErrorEvent>(*this);
}

} // namespace bitscrape::tracker
