#pragma once

#include "bitscrape/types/info_hash.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <future>

namespace bitscrape::tracker {

/**
 * @brief Base class for tracker requests
 */
class TrackerRequest {
public:
    /**
     * @brief Constructor
     * 
     * @param url Tracker URL
     */
    explicit TrackerRequest(std::string url);
    
    /**
     * @brief Destructor
     */
    virtual ~TrackerRequest() = default;
    
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
     * @brief Get the request parameters
     * 
     * @return Request parameters
     */
    [[nodiscard]] virtual std::map<std::string, std::string> parameters() const = 0;
    
    /**
     * @brief Build the request URL with parameters
     * 
     * @return Full request URL
     */
    [[nodiscard]] std::string build_url() const;
    
private:
    std::string url_; ///< Tracker URL
};

/**
 * @brief Announce request to a tracker
 */
class AnnounceRequest : public TrackerRequest {
public:
    /**
     * @brief Constructor
     * 
     * @param url Tracker URL
     * @param info_hash Torrent info hash
     * @param peer_id Peer ID
     * @param port Port
     * @param uploaded Bytes uploaded
     * @param downloaded Bytes downloaded
     * @param left Bytes left
     * @param event Event type (started, stopped, completed)
     * @param ip IP address (optional)
     * @param numwant Number of peers wanted (optional)
     * @param key Key for the request (optional)
     * @param tracker_id Tracker ID (optional)
     */
    AnnounceRequest(
        const std::string& url,
        const types::InfoHash& info_hash,
        const std::string& peer_id,
        uint16_t port,
        uint64_t uploaded,
        uint64_t downloaded,
        uint64_t left,
        const std::string& event = "",
        const std::string& ip = "",
        int numwant = -1,
        const std::string& key = "",
        const std::string& tracker_id = ""
    );
    
    /**
     * @brief Get the info hash
     * 
     * @return Info hash
     */
    [[nodiscard]] const types::InfoHash& info_hash() const;
    
    /**
     * @brief Get the peer ID
     * 
     * @return Peer ID
     */
    [[nodiscard]] const std::string& peer_id() const;
    
    /**
     * @brief Get the port
     * 
     * @return Port
     */
    [[nodiscard]] uint16_t port() const;
    
    /**
     * @brief Get the uploaded bytes
     * 
     * @return Uploaded bytes
     */
    [[nodiscard]] uint64_t uploaded() const;
    
    /**
     * @brief Get the downloaded bytes
     * 
     * @return Downloaded bytes
     */
    [[nodiscard]] uint64_t downloaded() const;
    
    /**
     * @brief Get the bytes left
     * 
     * @return Bytes left
     */
    [[nodiscard]] uint64_t left() const;
    
    /**
     * @brief Get the event
     * 
     * @return Event
     */
    [[nodiscard]] const std::string& event() const;
    
    /**
     * @brief Get the IP address
     * 
     * @return IP address
     */
    [[nodiscard]] const std::string& ip() const;
    
    /**
     * @brief Get the number of peers wanted
     * 
     * @return Number of peers wanted
     */
    [[nodiscard]] int numwant() const;
    
    /**
     * @brief Get the key
     * 
     * @return Key
     */
    [[nodiscard]] const std::string& key() const;
    
    /**
     * @brief Get the tracker ID
     * 
     * @return Tracker ID
     */
    [[nodiscard]] const std::string& tracker_id() const;
    
    /**
     * @brief Get the request parameters
     * 
     * @return Request parameters
     */
    [[nodiscard]] std::map<std::string, std::string> parameters() const override;
    
private:
    types::InfoHash info_hash_; ///< Torrent info hash
    std::string peer_id_;       ///< Peer ID
    uint16_t port_;             ///< Port
    uint64_t uploaded_;         ///< Bytes uploaded
    uint64_t downloaded_;       ///< Bytes downloaded
    uint64_t left_;             ///< Bytes left
    std::string event_;         ///< Event type (started, stopped, completed)
    std::string ip_;            ///< IP address (optional)
    int numwant_;               ///< Number of peers wanted (optional)
    std::string key_;           ///< Key for the request (optional)
    std::string tracker_id_;    ///< Tracker ID (optional)
};

/**
 * @brief Scrape request to a tracker
 */
class ScrapeRequest : public TrackerRequest {
public:
    /**
     * @brief Constructor
     * 
     * @param url Tracker URL
     * @param info_hashes List of torrent info hashes
     */
    ScrapeRequest(
        const std::string& url,
        const std::vector<types::InfoHash>& info_hashes
    );
    
    /**
     * @brief Get the list of info hashes
     * 
     * @return List of info hashes
     */
    [[nodiscard]] const std::vector<types::InfoHash>& info_hashes() const;
    
    /**
     * @brief Get the request parameters
     * 
     * @return Request parameters
     */
    [[nodiscard]] std::map<std::string, std::string> parameters() const override;
    
private:
    std::vector<types::InfoHash> info_hashes_; ///< List of torrent info hashes
};

} // namespace bitscrape::tracker
