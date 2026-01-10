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
 * @brief Utility class for announcing to trackers
 * 
 * This class provides high-level functionality for announcing to trackers,
 * including managing multiple trackers and handling tracker responses.
 */
class TrackerAnnounce {
public:
    /**
     * @brief Constructor
     * 
     * @param info_hash Torrent info hash
     * @param peer_id Peer ID
     * @param port Port
     */
    TrackerAnnounce(
        const types::InfoHash& info_hash,
        const std::string& peer_id,
        uint16_t port
    );
    
    /**
     * @brief Destructor
     */
    ~TrackerAnnounce() = default;
    
    /**
     * @brief TrackerAnnounce is non-copyable
     */
    TrackerAnnounce(const TrackerAnnounce&) = delete;
    TrackerAnnounce& operator=(const TrackerAnnounce&) = delete;
    
    /**
     * @brief TrackerAnnounce is movable
     */
    TrackerAnnounce(TrackerAnnounce&&) noexcept = default;
    TrackerAnnounce& operator=(TrackerAnnounce&&) noexcept = default;
    
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
     * @brief Send a started announce to all trackers
     * 
     * @return Map of tracker URLs to announce responses
     */
    std::map<std::string, AnnounceResponse> announce_started();
    
    /**
     * @brief Send a started announce to all trackers asynchronously
     * 
     * @return Future containing a map of tracker URLs to announce responses
     */
    std::future<std::map<std::string, AnnounceResponse>> announce_started_async();
    
    /**
     * @brief Send a stopped announce to all trackers
     * 
     * @return Map of tracker URLs to announce responses
     */
    std::map<std::string, AnnounceResponse> announce_stopped();
    
    /**
     * @brief Send a stopped announce to all trackers asynchronously
     * 
     * @return Future containing a map of tracker URLs to announce responses
     */
    std::future<std::map<std::string, AnnounceResponse>> announce_stopped_async();
    
    /**
     * @brief Send a completed announce to all trackers
     * 
     * @return Map of tracker URLs to announce responses
     */
    std::map<std::string, AnnounceResponse> announce_completed();
    
    /**
     * @brief Send a completed announce to all trackers asynchronously
     * 
     * @return Future containing a map of tracker URLs to announce responses
     */
    std::future<std::map<std::string, AnnounceResponse>> announce_completed_async();
    
    /**
     * @brief Send a regular announce to all trackers
     * 
     * @return Map of tracker URLs to announce responses
     */
    std::map<std::string, AnnounceResponse> announce_regular();
    
    /**
     * @brief Send a regular announce to all trackers asynchronously
     * 
     * @return Future containing a map of tracker URLs to announce responses
     */
    std::future<std::map<std::string, AnnounceResponse>> announce_regular_async();
    
    /**
     * @brief Set the uploaded bytes
     * 
     * @param uploaded Uploaded bytes
     */
    void set_uploaded(uint64_t uploaded);
    
    /**
     * @brief Set the downloaded bytes
     * 
     * @param downloaded Downloaded bytes
     */
    void set_downloaded(uint64_t downloaded);
    
    /**
     * @brief Set the bytes left
     * 
     * @param left Bytes left
     */
    void set_left(uint64_t left);
    
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
     * @brief Send an announce to all trackers
     * 
     * @param event Event type (started, stopped, completed, or empty for regular)
     * @return Map of tracker URLs to announce responses
     */
    std::map<std::string, AnnounceResponse> announce(const std::string& event);
    
    /**
     * @brief Send an announce to all trackers asynchronously
     * 
     * @param event Event type (started, stopped, completed, or empty for regular)
     * @return Future containing a map of tracker URLs to announce responses
     */
    std::future<std::map<std::string, AnnounceResponse>> announce_async(const std::string& event);
    
    types::InfoHash info_hash_;                ///< Torrent info hash
    std::string peer_id_;                      ///< Peer ID
    uint16_t port_;                            ///< Port
    uint64_t uploaded_;                        ///< Bytes uploaded
    uint64_t downloaded_;                      ///< Bytes downloaded
    uint64_t left_;                            ///< Bytes left
    std::unique_ptr<TrackerManager> manager_;  ///< Tracker manager
};

} // namespace bitscrape::tracker
