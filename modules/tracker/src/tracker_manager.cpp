#include "bitscrape/tracker/tracker_manager.hpp"

#include <algorithm>
#include <future>
#include <stdexcept>

namespace bitscrape::tracker {

TrackerManager::TrackerManager(const types::InfoHash& info_hash)
    : info_hash_(info_hash),
      connection_timeout_ms_(30000),
      request_timeout_ms_(30000) {
}

bool TrackerManager::add_tracker(const std::string& url) {
    std::lock_guard<std::mutex> lock(trackers_mutex_);
    
    // Check if the tracker already exists
    if (http_trackers_.find(url) != http_trackers_.end() ||
        udp_trackers_.find(url) != udp_trackers_.end()) {
        return false;
    }
    
    // Determine the tracker type
    TrackerType type = determine_tracker_type(url);
    
    // Create the tracker
    if (type == TrackerType::HTTP) {
        auto tracker = std::make_unique<HTTPTracker>(url);
        tracker->set_connection_timeout(connection_timeout_ms_);
        tracker->set_request_timeout(request_timeout_ms_);
        http_trackers_[url] = std::move(tracker);
        return true;
    } else if (type == TrackerType::UDP) {
        auto tracker = std::make_unique<UDPTracker>(url);
        tracker->set_connection_timeout(connection_timeout_ms_);
        tracker->set_request_timeout(request_timeout_ms_);
        udp_trackers_[url] = std::move(tracker);
        return true;
    }
    
    return false;
}

bool TrackerManager::remove_tracker(const std::string& url) {
    std::lock_guard<std::mutex> lock(trackers_mutex_);
    
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

std::vector<std::string> TrackerManager::tracker_urls() const {
    std::lock_guard<std::mutex> lock(trackers_mutex_);
    
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

std::map<std::string, AnnounceResponse> TrackerManager::announce(
    const std::string& peer_id,
    uint16_t port,
    uint64_t uploaded,
    uint64_t downloaded,
    uint64_t left,
    const std::string& event
) {
    std::lock_guard<std::mutex> lock(trackers_mutex_);
    
    std::map<std::string, AnnounceResponse> responses;
    
    // Send announce to HTTP trackers
    for (const auto& [url, tracker] : http_trackers_) {
        AnnounceRequest request(
            url,
            info_hash_,
            peer_id,
            port,
            uploaded,
            downloaded,
            left,
            event
        );
        
        responses[url] = tracker->announce(request);
    }
    
    // Send announce to UDP trackers
    for (const auto& [url, tracker] : udp_trackers_) {
        AnnounceRequest request(
            url,
            info_hash_,
            peer_id,
            port,
            uploaded,
            downloaded,
            left,
            event
        );
        
        responses[url] = tracker->announce(request);
    }
    
    return responses;
}

std::future<std::map<std::string, AnnounceResponse>> TrackerManager::announce_async(
    const std::string& peer_id,
    uint16_t port,
    uint64_t uploaded,
    uint64_t downloaded,
    uint64_t left,
    const std::string& event
) {
    return std::async(std::launch::async, [this, peer_id, port, uploaded, downloaded, left, event]() {
        return announce(peer_id, port, uploaded, downloaded, left, event);
    });
}

std::map<std::string, ScrapeResponse> TrackerManager::scrape() {
    std::lock_guard<std::mutex> lock(trackers_mutex_);
    
    std::map<std::string, ScrapeResponse> responses;
    
    // Create a vector with just our info hash
    std::vector<types::InfoHash> info_hashes = {info_hash_};
    
    // Send scrape to HTTP trackers
    for (const auto& [url, tracker] : http_trackers_) {
        ScrapeRequest request(url, info_hashes);
        responses[url] = tracker->scrape(request);
    }
    
    // Send scrape to UDP trackers
    for (const auto& [url, tracker] : udp_trackers_) {
        ScrapeRequest request(url, info_hashes);
        responses[url] = tracker->scrape(request);
    }
    
    return responses;
}

std::future<std::map<std::string, ScrapeResponse>> TrackerManager::scrape_async() {
    return std::async(std::launch::async, [this]() {
        return scrape();
    });
}

const types::InfoHash& TrackerManager::info_hash() const {
    return info_hash_;
}

void TrackerManager::set_connection_timeout(int timeout_ms) {
    std::lock_guard<std::mutex> lock(trackers_mutex_);
    
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

void TrackerManager::set_request_timeout(int timeout_ms) {
    std::lock_guard<std::mutex> lock(trackers_mutex_);
    
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

TrackerType TrackerManager::determine_tracker_type(const std::string& url) {
    // Check if the URL starts with "http://" or "https://"
    if (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://") {
        return TrackerType::HTTP;
    }
    
    // Check if the URL starts with "udp://"
    if (url.substr(0, 6) == "udp://") {
        return TrackerType::UDP;
    }
    
    return TrackerType::UNKNOWN;
}

} // namespace bitscrape::tracker
