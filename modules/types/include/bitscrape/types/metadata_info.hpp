#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <future>

#include "bitscrape/types/info_hash.hpp"

namespace bitscrape::types {

/**
 * @brief Represents the metadata info dictionary from a torrent file
 * 
 * MetadataInfo encapsulates the metadata info dictionary from a torrent file,
 * including the name, piece length, pieces, and other properties.
 */
class MetadataInfo {
public:
    /**
     * @brief Default constructor creates an empty metadata info
     */
    MetadataInfo();
    
    /**
     * @brief Create a metadata info from the raw bencode data
     * 
     * @param data Raw bencode data
     */
    explicit MetadataInfo(const std::vector<uint8_t>& data);
    
    /**
     * @brief Copy constructor
     */
    MetadataInfo(const MetadataInfo& other) = default;
    
    /**
     * @brief Move constructor
     */
    MetadataInfo(MetadataInfo&& other) noexcept = default;
    
    /**
     * @brief Copy assignment operator
     */
    MetadataInfo& operator=(const MetadataInfo& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    MetadataInfo& operator=(MetadataInfo&& other) noexcept = default;
    
    /**
     * @brief Destructor
     */
    ~MetadataInfo() = default;
    
    /**
     * @brief Get the name of the torrent
     * 
     * @return Name of the torrent
     */
    const std::string& name() const { return name_; }
    
    /**
     * @brief Set the name of the torrent
     * 
     * @param name Name of the torrent
     */
    void set_name(const std::string& name) { name_ = name; }
    
    /**
     * @brief Get the piece length
     * 
     * @return Piece length in bytes
     */
    uint64_t piece_length() const { return piece_length_; }
    
    /**
     * @brief Set the piece length
     * 
     * @param length Piece length in bytes
     */
    void set_piece_length(uint64_t length) { piece_length_ = length; }
    
    /**
     * @brief Get the pieces (SHA-1 hashes of each piece)
     * 
     * @return Vector of piece hashes
     */
    const std::vector<std::vector<uint8_t>>& pieces() const { return pieces_; }
    
    /**
     * @brief Set the pieces
     * 
     * @param pieces Vector of piece hashes
     */
    void set_pieces(const std::vector<std::vector<uint8_t>>& pieces) { pieces_ = pieces; }
    
    /**
     * @brief Get the total size of the torrent
     * 
     * @return Total size in bytes
     */
    uint64_t total_size() const { return total_size_; }
    
    /**
     * @brief Set the total size
     * 
     * @param size Total size in bytes
     */
    void set_total_size(uint64_t size) { total_size_ = size; }
    
    /**
     * @brief Get the files in the torrent
     * 
     * @return Vector of file information (path, size)
     */
    const std::vector<std::pair<std::string, uint64_t>>& files() const { return files_; }
    
    /**
     * @brief Set the files
     * 
     * @param files Vector of file information (path, size)
     */
    void set_files(const std::vector<std::pair<std::string, uint64_t>>& files) { files_ = files; }
    
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
     * @brief Get the raw bencode data
     * 
     * @return Raw bencode data
     */
    const std::vector<uint8_t>& raw_data() const { return raw_data_; }
    
    /**
     * @brief Check if the metadata is valid
     * 
     * @return true if the metadata is valid, false otherwise
     */
    bool is_valid() const;
    
    /**
     * @brief Parse the metadata from raw bencode data
     * 
     * @param data Raw bencode data
     * @return true if parsing was successful, false otherwise
     */
    bool parse(const std::vector<uint8_t>& data);
    
    /**
     * @brief Parse the metadata from raw bencode data asynchronously
     * 
     * @param data Raw bencode data
     * @return Future containing true if parsing was successful, false otherwise
     */
    std::future<bool> parse_async(const std::vector<uint8_t>& data);
    
    /**
     * @brief Convert the metadata to a string representation
     * 
     * @return String representation of the metadata
     */
    std::string to_string() const;

private:
    std::string name_;                                ///< Name of the torrent
    uint64_t piece_length_ = 0;                       ///< Piece length in bytes
    std::vector<std::vector<uint8_t>> pieces_;        ///< SHA-1 hashes of each piece
    uint64_t total_size_ = 0;                         ///< Total size in bytes
    std::vector<std::pair<std::string, uint64_t>> files_; ///< Files in the torrent (path, size)
    InfoHash info_hash_;                              ///< Info hash
    std::vector<uint8_t> raw_data_;                   ///< Raw bencode data
};

} // namespace bitscrape::types
