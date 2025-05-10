#include "bitscrape/tracker/tracker_scrape.hpp"

#include <algorithm>
#include <future>
#include <stdexcept>
#include <vector>

namespace bitscrape::tracker {

TrackerScrape::TrackerScrape(const types::InfoHash& info_hash)
    : info_hashes_({info_hash}),
      connection_timeout_ms_(30000),
      request_timeout_ms_(30000) {
}

TrackerScrape::TrackerScrape(const std::vector<types::InfoHash>& info_hashes)
    : info_hashes_(info_hashes),
      connection_timeout_ms_(30000),
      request_timeout_ms_(30000) {
}

bool TrackerScrape::add_tracker(const std::string& url) {
    // Determine the tracker type
    TrackerType type = TrackerType::UNKNOWN;

    // Check if the URL starts with "http://" or "https://"
    if (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://") {
        type = TrackerType::HTTP;
    }
    // Check if the URL starts with "udp://"
    else if (url.substr(0, 6) == "udp://") {
        type = TrackerType::UDP;
    }

    // Create the tracker
    if (type == TrackerType::HTTP) {
        auto tracker = std::make_unique<HTTPTracker>(url);
        tracker->set_connection_timeout(connection_timeout_ms_);
        tracker->set_request_timeout(request_timeout_ms_);
        http_trackers_[url] = std::move(tracker);
        return true;
    }

    if (type == TrackerType::UDP) {
        auto tracker = std::make_unique<UDPTracker>(url);
        tracker->set_connection_timeout(connection_timeout_ms_);
        tracker->set_request_timeout(request_timeout_ms_);
        udp_trackers_[url] = std::move(tracker);
        return true;
    }

    return false;
}

bool TrackerScrape::remove_tracker(const std::string& url) {
    // Try to remove from HTTP trackers
    auto http_it = http_trackers_.find(url);
    if (http_it != http_trackers_.end()) {
        http_trackers_.erase(http_it);
        return true;
    }

    // Try to remove from UDP trackers
    auto udp_it = udp_trackers_.find(url);
    if (udp_it != udp_trackers_.end()) {
        udp_trackers_.erase(udp_it);
        return true;
    }

    return false;
}

std::vector<std::string> TrackerScrape::tracker_urls() const {
    std::vector<std::string> urls;
    urls.reserve(http_trackers_.size() + udp_trackers_.size());

    // Add HTTP tracker URLs
    for (const auto& [url, _] : http_trackers_) {
        urls.push_back(url);
    }

    // Add UDP tracker URLs
    for (const auto& [url, _] : udp_trackers_) {
        urls.push_back(url);
    }

    return urls;
}

void TrackerScrape::add_info_hash(const types::InfoHash& info_hash) {
    // Check if the info hash already exists
    auto it = std::find(info_hashes_.begin(), info_hashes_.end(), info_hash);
    if (it == info_hashes_.end()) {
        info_hashes_.push_back(info_hash);
    }
}

void TrackerScrape::remove_info_hash(const types::InfoHash& info_hash) {
    // Remove the info hash if it exists
    auto it = std::remove(info_hashes_.begin(), info_hashes_.end(), info_hash);
    if (it != info_hashes_.end()) {
        info_hashes_.erase(it, info_hashes_.end());
    }
}

const std::vector<types::InfoHash>& TrackerScrape::info_hashes() const {
    return info_hashes_;
}

std::map<std::string, ScrapeResponse> TrackerScrape::scrape() {
    std::map<std::string, ScrapeResponse> responses;

    // Send scrape to HTTP trackers
    for (const auto& [url, tracker] : http_trackers_) {
        ScrapeRequest request(url, info_hashes_);
        responses[url] = tracker->scrape(request);
    }

    // Send scrape to UDP trackers
    for (const auto& [url, tracker] : udp_trackers_) {
        ScrapeRequest request(url, info_hashes_);
        responses[url] = tracker->scrape(request);
    }

    return responses;
}

std::future<std::map<std::string, ScrapeResponse>> TrackerScrape::scrape_async() {
    return std::async(std::launch::async, [this]() {
        return scrape();
    });
}

void TrackerScrape::set_connection_timeout(int timeout_ms) {
    connection_timeout_ms_ = timeout_ms;

    // Update HTTP trackers
    for (auto& [_, tracker] : http_trackers_) {
        tracker->set_connection_timeout(timeout_ms);
    }

    // Update UDP trackers
    for (auto& [_, tracker] : udp_trackers_) {
        tracker->set_connection_timeout(timeout_ms);
    }
}

void TrackerScrape::set_request_timeout(int timeout_ms) {
    request_timeout_ms_ = timeout_ms;

    // Update HTTP trackers
    for (auto& [_, tracker] : http_trackers_) {
        tracker->set_request_timeout(timeout_ms);
    }

    // Update UDP trackers
    for (auto& [_, tracker] : udp_trackers_) {
        tracker->set_request_timeout(timeout_ms);
    }
}

} // namespace bitscrape::tracker
