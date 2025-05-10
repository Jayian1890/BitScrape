#pragma once

#include "bitscrape/types/event_types.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/network/address.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace bitscrape::tracker {

/**
 * @brief Event types for tracker events
 */
enum class TrackerEventType : uint16_t {
    ANNOUNCE_REQUEST = 2000,
    ANNOUNCE_RESPONSE = 2001,
    SCRAPE_REQUEST = 2002,
    SCRAPE_RESPONSE = 2003,
    ERROR = 2004
};

/**
 * @brief Base class for all tracker events
 */
class TrackerEvent : public types::Event {
public:
    /**
     * @brief Constructor
     * 
     * @param type Tracker event type
     */
    explicit TrackerEvent(TrackerEventType type);
    
    /**
     * @brief Destructor
     */
    ~TrackerEvent() override = default;
    
    /**
     * @brief Get the tracker event type
     * 
     * @return Tracker event type
     */
    [[nodiscard]] TrackerEventType tracker_event_type() const;
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    [[nodiscard]] std::unique_ptr<types::Event> clone() const override = 0;
    
private:
    TrackerEventType tracker_event_type_; ///< Tracker event type
};

/**
 * @brief Event for tracker announce requests
 */
class AnnounceRequestEvent : public TrackerEvent {
public:
    /**
     * @brief Constructor
     * 
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
    AnnounceRequestEvent(
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
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    [[nodiscard]] std::unique_ptr<types::Event> clone() const override;
    
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
 * @brief Event for tracker announce responses
 */
class AnnounceResponseEvent : public TrackerEvent {
public:
    /**
     * @brief Constructor
     * 
     * @param info_hash Torrent info hash
     * @param interval Interval in seconds
     * @param min_interval Minimum interval in seconds
     * @param tracker_id Tracker ID
     * @param complete Number of seeders
     * @param incomplete Number of leechers
     * @param peers List of peers
     */
    AnnounceResponseEvent(
        const types::InfoHash& info_hash,
        int interval,
        int min_interval,
        const std::string& tracker_id,
        int complete,
        int incomplete,
        const std::vector<network::Address>& peers
    );
    
    /**
     * @brief Get the info hash
     * 
     * @return Info hash
     */
    [[nodiscard]] const types::InfoHash& info_hash() const;
    
    /**
     * @brief Get the interval
     * 
     * @return Interval in seconds
     */
    [[nodiscard]] int interval() const;
    
    /**
     * @brief Get the minimum interval
     * 
     * @return Minimum interval in seconds
     */
    [[nodiscard]] int min_interval() const;
    
    /**
     * @brief Get the tracker ID
     * 
     * @return Tracker ID
     */
    [[nodiscard]] const std::string& tracker_id() const;
    
    /**
     * @brief Get the number of seeders
     * 
     * @return Number of seeders
     */
    [[nodiscard]] int complete() const;
    
    /**
     * @brief Get the number of leechers
     * 
     * @return Number of leechers
     */
    [[nodiscard]] int incomplete() const;
    
    /**
     * @brief Get the list of peers
     * 
     * @return List of peers
     */
    [[nodiscard]] const std::vector<network::Address>& peers() const;
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    [[nodiscard]] std::unique_ptr<types::Event> clone() const override;
    
private:
    types::InfoHash info_hash_;           ///< Torrent info hash
    int interval_;                         ///< Interval in seconds
    int min_interval_;                     ///< Minimum interval in seconds
    std::string tracker_id_;               ///< Tracker ID
    int complete_;                         ///< Number of seeders
    int incomplete_;                       ///< Number of leechers
    std::vector<network::Address> peers_;  ///< List of peers
};

/**
 * @brief Event for tracker scrape requests
 */
class ScrapeRequestEvent : public TrackerEvent {
public:
    /**
     * @brief Constructor
     * 
     * @param info_hashes List of torrent info hashes
     */
    explicit ScrapeRequestEvent(const std::vector<types::InfoHash>& info_hashes);
    
    /**
     * @brief Get the list of info hashes
     * 
     * @return List of info hashes
     */
    [[nodiscard]] const std::vector<types::InfoHash>& info_hashes() const;
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    [[nodiscard]] std::unique_ptr<types::Event> clone() const override;
    
private:
    std::vector<types::InfoHash> info_hashes_; ///< List of torrent info hashes
};

/**
 * @brief Event for tracker scrape responses
 */
class ScrapeResponseEvent : public TrackerEvent {
public:
    /**
     * @brief Scrape data for a single torrent
     */
    struct ScrapeData {
        int complete;    ///< Number of seeders
        int downloaded;  ///< Number of downloads
        int incomplete;  ///< Number of leechers
        std::string name; ///< Torrent name (optional)
    };
    
    /**
     * @brief Constructor
     * 
     * @param files Map of info hashes to scrape data
     */
    explicit ScrapeResponseEvent(const std::map<types::InfoHash, ScrapeData>& files);
    
    /**
     * @brief Get the map of info hashes to scrape data
     * 
     * @return Map of info hashes to scrape data
     */
    [[nodiscard]] const std::map<types::InfoHash, ScrapeData>& files() const;
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    [[nodiscard]] std::unique_ptr<types::Event> clone() const override;
    
private:
    std::map<types::InfoHash, ScrapeData> files_; ///< Map of info hashes to scrape data
};

/**
 * @brief Event for tracker errors
 */
class TrackerErrorEvent : public TrackerEvent {
public:
    /**
     * @brief Constructor
     * 
     * @param error_message Error message
     */
    explicit TrackerErrorEvent(const std::string& error_message);
    
    /**
     * @brief Get the error message
     * 
     * @return Error message
     */
    [[nodiscard]] const std::string& error_message() const;
    
    /**
     * @brief Clone the event
     * 
     * @return A new heap-allocated copy of the event
     */
    [[nodiscard]] std::unique_ptr<types::Event> clone() const override;
    
private:
    std::string error_message_; ///< Error message
};

} // namespace bitscrape::tracker
