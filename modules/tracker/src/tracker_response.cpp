#include "bitscrape/tracker/tracker_response.hpp"

namespace bitscrape::tracker {

// TrackerResponse implementation
TrackerResponse::TrackerResponse() = default;

bool TrackerResponse::has_error() const {
    return !error_message_.empty();
}

const std::string& TrackerResponse::error_message() const {
    return error_message_;
}

void TrackerResponse::set_error_message(const std::string& error_message) {
    error_message_ = error_message;
}

const std::string& TrackerResponse::warning_message() const {
    return warning_message_;
}

void TrackerResponse::set_warning_message(const std::string& warning_message) {
    warning_message_ = warning_message;
}

// AnnounceResponse implementation
AnnounceResponse::AnnounceResponse()
    : interval_(0),
      min_interval_(0),
      complete_(0),
      incomplete_(0) {
}

int AnnounceResponse::interval() const {
    return interval_;
}

void AnnounceResponse::set_interval(int interval) {
    interval_ = interval;
}

int AnnounceResponse::min_interval() const {
    return min_interval_;
}

void AnnounceResponse::set_min_interval(int min_interval) {
    min_interval_ = min_interval;
}

const std::string& AnnounceResponse::tracker_id() const {
    return tracker_id_;
}

void AnnounceResponse::set_tracker_id(const std::string& tracker_id) {
    tracker_id_ = tracker_id;
}

int AnnounceResponse::complete() const {
    return complete_;
}

void AnnounceResponse::set_complete(int complete) {
    complete_ = complete;
}

int AnnounceResponse::incomplete() const {
    return incomplete_;
}

void AnnounceResponse::set_incomplete(int incomplete) {
    incomplete_ = incomplete;
}

const std::vector<network::Address>& AnnounceResponse::peers() const {
    return peers_;
}

void AnnounceResponse::add_peer(const network::Address& peer) {
    peers_.push_back(peer);
}

void AnnounceResponse::set_peers(const std::vector<network::Address>& peers) {
    peers_ = peers;
}

// ScrapeResponse implementation
ScrapeResponse::ScrapeResponse() = default;

const std::map<types::InfoHash, ScrapeResponse::ScrapeData>& ScrapeResponse::files() const {
    return files_;
}

void ScrapeResponse::add_file(const types::InfoHash& info_hash, const ScrapeData& data) {
    files_[info_hash] = data;
}

void ScrapeResponse::set_files(const std::map<types::InfoHash, ScrapeData>& files) {
    files_ = files;
}

} // namespace bitscrape::tracker
