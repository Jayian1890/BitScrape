#include "bitscrape/types/metadata_info.hpp"

#include <sstream>

namespace bitscrape::types {

MetadataInfo::MetadataInfo() {
    // Create an empty metadata info
}

MetadataInfo::MetadataInfo(const std::vector<uint8_t>& data) {
    parse(data);
}

bool MetadataInfo::is_valid() const {
    return !name_.empty() && piece_length_ > 0 && !pieces_.empty() && total_size_ > 0;
}

bool MetadataInfo::parse(const std::vector<uint8_t>& data) {
    // Store the raw data
    raw_data_ = data;
    
    // In a real implementation, we would parse the bencode data here
    // For now, we'll just set some dummy values for testing
    
    // Calculate the info hash
    info_hash_ = InfoHash::from_bencode(data);
    
    // This is a simplified implementation
    // In a real implementation, we would parse the bencode data and extract
    // the name, piece length, pieces, files, etc.
    
    // For now, return true to indicate successful parsing
    return true;
}

std::future<bool> MetadataInfo::parse_async(const std::vector<uint8_t>& data) {
    return std::async(std::launch::async, [this, data]() {
        return this->parse(data);
    });
}

std::string MetadataInfo::to_string() const {
    std::ostringstream oss;
    
    oss << "MetadataInfo[name=" << name_ << ", ";
    oss << "piece_length=" << piece_length_ << ", ";
    oss << "pieces=" << pieces_.size() << ", ";
    oss << "total_size=" << total_size_ << ", ";
    oss << "files=" << files_.size() << ", ";
    oss << "info_hash=" << info_hash_.to_hex() << "]";
    
    return oss.str();
}

} // namespace bitscrape::types
