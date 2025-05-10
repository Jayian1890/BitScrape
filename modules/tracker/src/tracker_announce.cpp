#include "bitscrape/tracker/tracker_announce.hpp"

#include <algorithm>
#include <future>
#include <stdexcept>

namespace bitscrape::tracker {

TrackerAnnounce::TrackerAnnounce(
    const types::InfoHash& info_hash,
    const std::string& peer_id,
    uint16_t port
)
    : info_hash_(info_hash),
      peer_id_(peer_id),
      port_(port),
      uploaded_(0),
      downloaded_(0),
      left_(0),
      manager_(std::make_unique<TrackerManager>(info_hash)) {
}

bool TrackerAnnounce::add_tracker(const std::string& url) {
    return manager_->add_tracker(url);
}

bool TrackerAnnounce::remove_tracker(const std::string& url) {
    return manager_->remove_tracker(url);
}

std::vector<std::string> TrackerAnnounce::tracker_urls() const {
    return manager_->tracker_urls();
}

std::map<std::string, AnnounceResponse> TrackerAnnounce::announce_started() {
    return announce("started");
}

std::future<std::map<std::string, AnnounceResponse>> TrackerAnnounce::announce_started_async() {
    return announce_async("started");
}

std::map<std::string, AnnounceResponse> TrackerAnnounce::announce_stopped() {
    return announce("stopped");
}

std::future<std::map<std::string, AnnounceResponse>> TrackerAnnounce::announce_stopped_async() {
    return announce_async("stopped");
}

std::map<std::string, AnnounceResponse> TrackerAnnounce::announce_completed() {
    return announce("completed");
}

std::future<std::map<std::string, AnnounceResponse>> TrackerAnnounce::announce_completed_async() {
    return announce_async("completed");
}

std::map<std::string, AnnounceResponse> TrackerAnnounce::announce_regular() {
    return announce("");
}

std::future<std::map<std::string, AnnounceResponse>> TrackerAnnounce::announce_regular_async() {
    return announce_async("");
}

void TrackerAnnounce::set_uploaded(uint64_t uploaded) {
    uploaded_ = uploaded;
}

void TrackerAnnounce::set_downloaded(uint64_t downloaded) {
    downloaded_ = downloaded;
}

void TrackerAnnounce::set_left(uint64_t left) {
    left_ = left;
}

void TrackerAnnounce::set_connection_timeout(int timeout_ms) {
    manager_->set_connection_timeout(timeout_ms);
}

void TrackerAnnounce::set_request_timeout(int timeout_ms) {
    manager_->set_request_timeout(timeout_ms);
}

std::map<std::string, AnnounceResponse> TrackerAnnounce::announce(const std::string& event) {
    return manager_->announce(
        peer_id_,
        port_,
        uploaded_,
        downloaded_,
        left_,
        event
    );
}

std::future<std::map<std::string, AnnounceResponse>> TrackerAnnounce::announce_async(const std::string& event) {
    return manager_->announce_async(
        peer_id_,
        port_,
        uploaded_,
        downloaded_,
        left_,
        event
    );
}

} // namespace bitscrape::tracker
