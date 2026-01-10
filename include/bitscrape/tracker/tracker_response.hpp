#pragma once

#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/network/address.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace bitscrape::tracker {

/**
 * @brief Base class for tracker responses
 */
class TrackerResponse {
public:
    /**
     * @brief Constructor
     */
    TrackerResponse();
    
    /**
     * @brief Destructor
     */
    virtual ~TrackerResponse() = default;
    
    /**
     * @brief Check if the response has an error
     * 
     * @return true if the response has an error, false otherwise
     */
    [[nodiscard]] bool has_error() const;
    
    /**
     * @brief Get the error message
     * 
     * @return Error message
     */
    [[nodiscard]] const std::string& error_message() const;
    
    /**
     * @brief Set the error message
     * 
     * @param error_message Error message
     */
    void set_error_message(const std::string& error_message);
    
    /**
     * @brief Get the warning message
     * 
     * @return Warning message
     */
    [[nodiscard]] const std::string& warning_message() const;
    
    /**
     * @brief Set the warning message
     * 
     * @param warning_message Warning message
     */
    void set_warning_message(const std::string& warning_message);
    
private:
    std::string error_message_;    ///< Error message
    std::string warning_message_;  ///< Warning message
};

/**
 * @brief Announce response from a tracker
 */
class AnnounceResponse : public TrackerResponse {
public:
    /**
     * @brief Constructor
     */
    AnnounceResponse();
    
    /**
     * @brief Get the interval
     * 
     * @return Interval in seconds
     */
    [[nodiscard]] int interval() const;
    
    /**
     * @brief Set the interval
     * 
     * @param interval Interval in seconds
     */
    void set_interval(int interval);
    
    /**
     * @brief Get the minimum interval
     * 
     * @return Minimum interval in seconds
     */
    [[nodiscard]] int min_interval() const;
    
    /**
     * @brief Set the minimum interval
     * 
     * @param min_interval Minimum interval in seconds
     */
    void set_min_interval(int min_interval);
    
    /**
     * @brief Get the tracker ID
     * 
     * @return Tracker ID
     */
    [[nodiscard]] const std::string& tracker_id() const;
    
    /**
     * @brief Set the tracker ID
     * 
     * @param tracker_id Tracker ID
     */
    void set_tracker_id(const std::string& tracker_id);
    
    /**
     * @brief Get the number of seeders
     * 
     * @return Number of seeders
     */
    [[nodiscard]] int complete() const;
    
    /**
     * @brief Set the number of seeders
     * 
     * @param complete Number of seeders
     */
    void set_complete(int complete);
    
    /**
     * @brief Get the number of leechers
     * 
     * @return Number of leechers
     */
    [[nodiscard]] int incomplete() const;
    
    /**
     * @brief Set the number of leechers
     * 
     * @param incomplete Number of leechers
     */
    void set_incomplete(int incomplete);
    
    /**
     * @brief Get the list of peers
     * 
     * @return List of peers
     */
    [[nodiscard]] const std::vector<network::Address>& peers() const;
    
    /**
     * @brief Add a peer
     * 
     * @param peer Peer address
     */
    void add_peer(const network::Address& peer);
    
    /**
     * @brief Set the list of peers
     * 
     * @param peers List of peers
     */
    void set_peers(const std::vector<network::Address>& peers);
    
private:
    int interval_;                         ///< Interval in seconds
    int min_interval_;                     ///< Minimum interval in seconds
    std::string tracker_id_;               ///< Tracker ID
    int complete_;                         ///< Number of seeders
    int incomplete_;                       ///< Number of leechers
    std::vector<network::Address> peers_;  ///< List of peers
};

/**
 * @brief Scrape response from a tracker
 */
class ScrapeResponse : public TrackerResponse {
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
     */
    ScrapeResponse();
    
    /**
     * @brief Get the map of info hashes to scrape data
     * 
     * @return Map of info hashes to scrape data
     */
    [[nodiscard]] const std::map<types::InfoHash, ScrapeData>& files() const;
    
    /**
     * @brief Add scrape data for a torrent
     * 
     * @param info_hash Torrent info hash
     * @param data Scrape data
     */
    void add_file(const types::InfoHash& info_hash, const ScrapeData& data);
    
    /**
     * @brief Set the map of info hashes to scrape data
     * 
     * @param files Map of info hashes to scrape data
     */
    void set_files(const std::map<types::InfoHash, ScrapeData>& files);
    
private:
    std::map<types::InfoHash, ScrapeData> files_; ///< Map of info hashes to scrape data
};

} // namespace bitscrape::tracker
