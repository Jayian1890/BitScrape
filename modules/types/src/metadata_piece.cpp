#include "bitscrape/types/metadata_piece.hpp"

#include <sstream>

namespace bitscrape::types {

MetadataPiece::MetadataPiece() {
    // Create an empty metadata piece
}

MetadataPiece::MetadataPiece(uint32_t index, const std::vector<uint8_t>& data, uint32_t total_size)
    : index_(index), data_(data), total_size_(total_size) {
}

bool MetadataPiece::is_valid() const {
    // A piece is valid if it has data and the total size is greater than 0
    return !data_.empty() && total_size_ > 0;
}

std::string MetadataPiece::to_string() const {
    std::ostringstream oss;
    
    oss << "MetadataPiece[index=" << index_ << ", ";
    oss << "data_size=" << data_.size() << ", ";
    oss << "total_size=" << total_size_ << "]";
    
    return oss.str();
}

bool MetadataPiece::operator==(const MetadataPiece& other) const {
    return index_ == other.index_ && data_ == other.data_ && total_size_ == other.total_size_;
}

bool MetadataPiece::operator!=(const MetadataPiece& other) const {
    return !(*this == other);
}

} // namespace bitscrape::types
