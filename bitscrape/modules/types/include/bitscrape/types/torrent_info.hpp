#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <future>
#include <chrono>

#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/types/metadata_info.hpp"

namespace bitscrape::types {

/**
 * @brief Represents the complete torrent information
 * 
 * TorrentInfo encapsulates the complete torrent information, including
 * the metadata info, announce URLs, creation date, and other properties.
 */
class TorrentInfo {
public:
    /**
     * @brief Default constructor creates an empty torrent info
     */
    TorrentInfo();
    
    /**
     * @brief Create a torrent info from the raw bencode data
     * 
     * @param data Raw bencode data
     */
    explicit TorrentInfo(const std::vector<uint8_t>& data);
    
    /**
     * @brief Create a torrent info from an info hash and metadata
     * 
     * @param info_hash Info hash
     * @param metadata Metadata info
     */
    TorrentInfo(const InfoHash& info_hash, const MetadataInfo& metadata);
    
    /**
     * @brief Copy constructor
     */
    TorrentInfo(const TorrentInfo& other) = default;
    
    /**
     * @brief Move constructor
     */
    TorrentInfo(TorrentInfo&& other) noexcept = default;
    
    /**
     * @brief Copy assignment operator
     */
    TorrentInfo& operator=(const TorrentInfo& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    TorrentInfo& operator=(TorrentInfo&& other) noexcept = default;
    
    /**
     * @brief Destructor
     */
    ~TorrentInfo() = default;
    
    /**
     * @brief Get the info hash
     * 
     * @return Info hash
     */
    const InfoHash& info_hash() const { return info_hash_; }
    
    /**
     * @brief Set the info hash
     * 
     * @param hash Info hash
     */
    void set_info_hash(const InfoHash& hash) { info_hash_ = hash; }
    
    /**
     * @brief Get the metadata info
     * 
     * @return Metadata info
     */
    const MetadataInfo& metadata() const { return metadata_; }
    
    /**
     * @brief Set the metadata info
     * 
     * @param metadata Metadata info
     */
    void set_metadata(const MetadataInfo& metadata) { metadata_ = metadata; }
    
    /**
     * @brief Get the announce URL
     * 
     * @return Announce URL
     */
    const std::string& announce() const { return announce_; }
    
    /**
     * @brief Set the announce URL
     * 
     * @param url Announce URL
     */
    void set_announce(const std::string& url) { announce_ = url; }
    
    /**
     * @brief Get the announce list
     * 
     * @return Announce list
     */
    const std::vector<std::string>& announce_list() const { return announce_list_; }
    
    /**
     * @brief Set the announce list
     * 
     * @param list Announce list
     */
    void set_announce_list(const std::vector<std::string>& list) { announce_list_ = list; }
    
    /**
     * @brief Get the creation date
     * 
     * @return Creation date
     */
    std::optional<std::chrono::system_clock::time_point> creation_date() const { return creation_date_; }
    
    /**
     * @brief Set the creation date
     * 
     * @param date Creation date
     */
    void set_creation_date(const std::chrono::system_clock::time_point& date) { creation_date_ = date; }
    
    /**
     * @brief Get the comment
     * 
     * @return Comment
     */
    const std::string& comment() const { return comment_; }
    
    /**
     * @brief Set the comment
     * 
     * @param comment Comment
     */
    void set_comment(const std::string& comment) { comment_ = comment; }
    
    /**
     * @brief Get the created by
     * 
     * @return Created by
     */
    const std::string& created_by() const { return created_by_; }
    
    /**
     * @brief Set the created by
     * 
     * @param created_by Created by
     */
    void set_created_by(const std::string& created_by) { created_by_ = created_by; }
    
    /**
     * @brief Get the raw bencode data
     * 
     * @return Raw bencode data
     */
    const std::vector<uint8_t>& raw_data() const { return raw_data_; }
    
    /**
     * @brief Check if the torrent info is valid
     * 
     * @return true if the torrent info is valid, false otherwise
     */
    bool is_valid() const;
    
    /**
     * @brief Parse the torrent info from raw bencode data
     * 
     * @param data Raw bencode data
     * @return true if parsing was successful, false otherwise
     */
    bool parse(const std::vector<uint8_t>& data);
    
    /**
     * @brief Parse the torrent info from raw bencode data asynchronously
     * 
     * @param data Raw bencode data
     * @return Future containing true if parsing was successful, false otherwise
     */
    std::future<bool> parse_async(const std::vector<uint8_t>& data);
    
    /**
     * @brief Convert the torrent info to a string representation
     * 
     * @return String representation of the torrent info
     */
    std::string to_string() const;

private:
    InfoHash info_hash_;                              ///< Info hash
    MetadataInfo metadata_;                           ///< Metadata info
    std::string announce_;                            ///< Announce URL
    std::vector<std::string> announce_list_;          ///< Announce list
    std::optional<std::chrono::system_clock::time_point> creation_date_; ///< Creation date
    std::string comment_;                             ///< Comment
    std::string created_by_;                          ///< Created by
    std::vector<uint8_t> raw_data_;                   ///< Raw bencode data
};

} // namespace bitscrape::types
