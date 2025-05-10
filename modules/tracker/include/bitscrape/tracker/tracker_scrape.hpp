#pragma once

#include "bitscrape/tracker/tracker_request.hpp"
#include "bitscrape/tracker/tracker_response.hpp"
#include "bitscrape/tracker/tracker_manager.hpp"
#include "bitscrape/types/info_hash.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <future>

namespace bitscrape::tracker {

/**
 * @brief Utility class for scraping trackers
 * 
 * This class provides high-level functionality for scraping trackers,
 * including managing multiple trackers and handling tracker responses.
 */
class TrackerScrape {
public:
    /**
     * @brief Constructor for scraping a single torrent
     * 
     * @param info_hash Torrent info hash
     */
    explicit TrackerScrape(const types::InfoHash& info_hash);
    
    /**
     * @brief Constructor for scraping multiple torrents
     * 
     * @param info_hashes List of torrent info hashes
     */
    explicit TrackerScrape(const std::vector<types::InfoHash>& info_hashes);
    
    /**
     * @brief Destructor
     */
    ~TrackerScrape() = default;
    
    /**
     * @brief TrackerScrape is non-copyable
     */
    TrackerScrape(const TrackerScrape&) = delete;
    TrackerScrape& operator=(const TrackerScrape&) = delete;
    
    /**
     * @brief TrackerScrape is movable
     */
    TrackerScrape(TrackerScrape&&) noexcept = default;
    TrackerScrape& operator=(TrackerScrape&&) noexcept = default;
    
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
     * @brief Add a torrent info hash
     * 
     * @param info_hash Torrent info hash
     */
    void add_info_hash(const types::InfoHash& info_hash);
    
    /**
     * @brief Remove a torrent info hash
     * 
     * @param info_hash Torrent info hash
     */
    void remove_info_hash(const types::InfoHash& info_hash);
    
    /**
     * @brief Get the list of torrent info hashes
     * 
     * @return List of torrent info hashes
     */
    [[nodiscard]] const std::vector<types::InfoHash>& info_hashes() const;
    
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
    std::vector<types::InfoHash> info_hashes_;                ///< List of torrent info hashes
    std::map<std::string, std::unique_ptr<HTTPTracker>> http_trackers_; ///< HTTP trackers
    std::map<std::string, std::unique_ptr<UDPTracker>> udp_trackers_;   ///< UDP trackers
    
    int connection_timeout_ms_;                                ///< Connection timeout in milliseconds
    int request_timeout_ms_;                                   ///< Request timeout in milliseconds
};

} // namespace bitscrape::tracker
