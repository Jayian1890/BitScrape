#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <future>

namespace bitscrape::types {

/**
 * @brief Represents a piece of metadata during the metadata exchange
 * 
 * MetadataPiece encapsulates a piece of metadata during the metadata exchange protocol.
 * It includes the piece index, data, and total size.
 */
class MetadataPiece {
public:
    /**
     * @brief Default constructor creates an empty metadata piece
     */
    MetadataPiece();
    
    /**
     * @brief Create a metadata piece with the specified index and data
     * 
     * @param index Piece index
     * @param data Piece data
     * @param total_size Total size of the metadata
     */
    MetadataPiece(uint32_t index, const std::vector<uint8_t>& data, uint32_t total_size);
    
    /**
     * @brief Copy constructor
     */
    MetadataPiece(const MetadataPiece& other) = default;
    
    /**
     * @brief Move constructor
     */
    MetadataPiece(MetadataPiece&& other) noexcept = default;
    
    /**
     * @brief Copy assignment operator
     */
    MetadataPiece& operator=(const MetadataPiece& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    MetadataPiece& operator=(MetadataPiece&& other) noexcept = default;
    
    /**
     * @brief Destructor
     */
    ~MetadataPiece() = default;
    
    /**
     * @brief Get the piece index
     * 
     * @return Piece index
     */
    uint32_t index() const { return index_; }
    
    /**
     * @brief Set the piece index
     * 
     * @param index Piece index
     */
    void set_index(uint32_t index) { index_ = index; }
    
    /**
     * @brief Get the piece data
     * 
     * @return Piece data
     */
    const std::vector<uint8_t>& data() const { return data_; }
    
    /**
     * @brief Set the piece data
     * 
     * @param data Piece data
     */
    void set_data(const std::vector<uint8_t>& data) { data_ = data; }
    
    /**
     * @brief Get the total size of the metadata
     * 
     * @return Total size in bytes
     */
    uint32_t total_size() const { return total_size_; }
    
    /**
     * @brief Set the total size of the metadata
     * 
     * @param size Total size in bytes
     */
    void set_total_size(uint32_t size) { total_size_ = size; }
    
    /**
     * @brief Check if the piece is valid
     * 
     * @return true if the piece is valid, false otherwise
     */
    bool is_valid() const;
    
    /**
     * @brief Convert the piece to a string representation
     * 
     * @return String representation of the piece
     */
    std::string to_string() const;
    
    /**
     * @brief Equality operator
     */
    bool operator==(const MetadataPiece& other) const;
    
    /**
     * @brief Inequality operator
     */
    bool operator!=(const MetadataPiece& other) const;

private:
    uint32_t index_ = 0;                 ///< Piece index
    std::vector<uint8_t> data_;          ///< Piece data
    uint32_t total_size_ = 0;            ///< Total size of the metadata
};

} // namespace bitscrape::types
