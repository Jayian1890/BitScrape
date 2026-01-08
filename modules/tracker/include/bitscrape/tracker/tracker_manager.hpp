#pragma once

#include "bitscrape/tracker/http_tracker.hpp"
#include "bitscrape/tracker/udp_tracker.hpp"
#include "bitscrape/tracker/tracker_request.hpp"
#include "bitscrape/tracker/tracker_response.hpp"
#include "bitscrape/types/info_hash.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <future>
#include <mutex>

namespace bitscrape::tracker {

/**
 * @brief Tracker type enumeration
 */
enum class TrackerType {
    HTTP,
    UDP,
    UNKNOWN
};

/**
 * @brief Manages multiple trackers for a torrent
 * 
 * This class provides functionality for managing multiple trackers for a torrent,
 * including sending announce and scrape requests to all trackers.
 */
class TrackerManager {
public:
    /**
     * @brief Constructor
     * 
     * @param info_hash Torrent info hash
     */
    explicit TrackerManager(const types::InfoHash& info_hash);
    
    /**
     * @brief Destructor
     */
    ~TrackerManager() = default;
    
    /**
     * @brief TrackerManager is non-copyable
     */
    TrackerManager(const TrackerManager&) = delete;
    TrackerManager& operator=(const TrackerManager&) = delete;
    
    /**
     * @brief TrackerManager is movable
     */
    TrackerManager(TrackerManager&&) noexcept = delete;
    TrackerManager& operator=(TrackerManager&&) noexcept = delete;
    
    /**
     * @brief Add a tracker
     * 
     * @param url Tracker URL
     * @return true if the tracker was added, false otherwise
     */
    bool add_tracker(const std::string& url);
    
    /**
     * @brief Remove a tracker
     * 
     * @param url Tracker URL
     * @return true if the tracker was removed, false otherwise
     */
    bool remove_tracker(const std::string& url);
    
    /**
     * @brief Get the list of tracker URLs
     * 
     * @return List of tracker URLs
     */
    [[nodiscard]] std::vector<std::string> tracker_urls() const;
    
    /**
     * @brief Send an announce request to all trackers
     * 
     * @param peer_id Peer ID
     * @param port Port
     * @param uploaded Bytes uploaded
     * @param downloaded Bytes downloaded
     * @param left Bytes left
     * @param event Event type (started, stopped, completed)
     * @return Map of tracker URLs to announce responses
     */
    std::map<std::string, AnnounceResponse> announce(
        const std::string& peer_id,
        uint16_t port,
        uint64_t uploaded,
        uint64_t downloaded,
        uint64_t left,
        const std::string& event = ""
    );
    
    /**
     * @brief Send an announce request to all trackers asynchronously
     * 
     * @param peer_id Peer ID
     * @param port Port
     * @param uploaded Bytes uploaded
     * @param downloaded Bytes downloaded
     * @param left Bytes left
     * @param event Event type (started, stopped, completed)
     * @return Future containing a map of tracker URLs to announce responses
     */
    std::future<std::map<std::string, AnnounceResponse>> announce_async(
        const std::string& peer_id,
        uint16_t port,
        uint64_t uploaded,
        uint64_t downloaded,
        uint64_t left,
        const std::string& event = ""
    );
    
    /**
     * @brief Send a scrape request to all trackers
     * 
     * @return Map of tracker URLs to scrape responses
     */
    std::map<std::string, ScrapeResponse> scrape();
    
    /**
     * @brief Send a scrape request to all trackers asynchronously
     * 
     * @return Future containing a map of tracker URLs to scrape responses
     */
    std::future<std::map<std::string, ScrapeResponse>> scrape_async();
    
    /**
     * @brief Get the torrent info hash
     * 
     * @return Torrent info hash
     */
    [[nodiscard]] const types::InfoHash& info_hash() const;
    
    /**
     * @brief Set the connection timeout for all trackers
     * 
     * @param timeout_ms Timeout in milliseconds
     */
    void set_connection_timeout(int timeout_ms);
    
    /**
     * @brief Set the request timeout for all trackers
     * 
     * @param timeout_ms Timeout in milliseconds
     */
    void set_request_timeout(int timeout_ms);
    
private:
    /**
     * @brief Determine the tracker type from a URL
     * 
     * @param url Tracker URL
     * @return Tracker type
     */
    static TrackerType determine_tracker_type(const std::string& url);
    
    types::InfoHash info_hash_;                                ///< Torrent info hash
    std::map<std::string, std::unique_ptr<HTTPTracker>> http_trackers_; ///< HTTP trackers
    std::map<std::string, std::unique_ptr<UDPTracker>> udp_trackers_;   ///< UDP trackers
    mutable std::mutex trackers_mutex_;                        ///< Mutex for trackers maps
    
    int connection_timeout_ms_;                                ///< Connection timeout in milliseconds
    int request_timeout_ms_;                                   ///< Request timeout in milliseconds
};

} // namespace bitscrape::tracker
