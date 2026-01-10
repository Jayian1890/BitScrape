#pragma once

#include "bitscrape/tracker/tracker_request.hpp"
#include "bitscrape/tracker/tracker_response.hpp"
#include "bitscrape/network/http_client.hpp"
#include "bitscrape/bencode/bencode_decoder.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <future>

namespace bitscrape::tracker {

/**
 * @brief HTTP tracker implementation
 * 
 * This class provides functionality for communicating with HTTP trackers.
 */
class HTTPTracker {
public:
    /**
     * @brief Constructor
     * 
     * @param url Tracker URL
     */
    explicit HTTPTracker(std::string url);
    
    /**
     * @brief Destructor
     */
    ~HTTPTracker() = default;
    
    /**
     * @brief HTTPTracker is non-copyable
     */
    HTTPTracker(const HTTPTracker&) = delete;
    HTTPTracker& operator=(const HTTPTracker&) = delete;
    
    /**
     * @brief HTTPTracker is movable
     */
    HTTPTracker(HTTPTracker&&) noexcept = default;
    HTTPTracker& operator=(HTTPTracker&&) noexcept = default;
    
    /**
     * @brief Get the tracker URL
     * 
     * @return Tracker URL
     */
    [[nodiscard]] const std::string& url() const;
    
    /**
     * @brief Set the tracker URL
     * 
     * @param url Tracker URL
     */
    void set_url(const std::string& url);
    
    /**
     * @brief Send an announce request to the tracker
     * 
     * @param request Announce request
     * @return Announce response
     */
    AnnounceResponse announce(const AnnounceRequest& request);
    
    /**
     * @brief Send an announce request to the tracker asynchronously
     * 
     * @param request Announce request
     * @return Future containing the announce response
     */
    std::future<AnnounceResponse> announce_async(const AnnounceRequest& request);
    
    /**
     * @brief Send a scrape request to the tracker
     * 
     * @param request Scrape request
     * @return Scrape response
     */
    ScrapeResponse scrape(const ScrapeRequest& request);
    
    /**
     * @brief Send a scrape request to the tracker asynchronously
     * 
     * @param request Scrape request
     * @return Future containing the scrape response
     */
    std::future<ScrapeResponse> scrape_async(const ScrapeRequest& request);
    
    /**
     * @brief Set the connection timeout
     * 
     * @param timeout_ms Timeout in milliseconds
     */
    void set_connection_timeout(int timeout_ms);
    
    /**
     * @brief Set the request timeout
     * 
     * @param timeout_ms Timeout in milliseconds
     */
    void set_request_timeout(int timeout_ms);
    
private:
    /**
     * @brief Parse an announce response
     * 
     * @param data Response data
     * @return Announce response
     */
    AnnounceResponse parse_announce_response(const network::Buffer& data);
    
    /**
     * @brief Parse a scrape response
     * 
     * @param data Response data
     * @return Scrape response
     */
    ScrapeResponse parse_scrape_response(const network::Buffer& data);
    
    /**
     * @brief Convert a tracker URL to a scrape URL
     * 
     * @param announce_url Announce URL
     * @return Scrape URL
     */
    static std::string announce_to_scrape_url(const std::string& announce_url);
    
    std::string url_;                                  ///< Tracker URL
    network::HTTPClient http_client_;                  ///< HTTP client
    std::unique_ptr<bencode::BencodeDecoder> decoder_; ///< Bencode decoder
};

} // namespace bitscrape::tracker
