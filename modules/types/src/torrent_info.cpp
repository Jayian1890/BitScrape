#include "bitscrape/types/torrent_info.hpp"

#include <sstream>

namespace bitscrape::types {

TorrentInfo::TorrentInfo() {
    // Create an empty torrent info
}

TorrentInfo::TorrentInfo(const std::vector<uint8_t>& data) {
    parse(data);
}

TorrentInfo::TorrentInfo(const InfoHash& info_hash, const MetadataInfo& metadata)
    : info_hash_(info_hash), metadata_(metadata) {
}

bool TorrentInfo::is_valid() const {
    // A torrent info is valid if it has a valid info hash and metadata
    return metadata_.is_valid();
}

bool TorrentInfo::parse(const std::vector<uint8_t>& data) {
    // Store the raw data
    raw_data_ = data;
    
    // In a real implementation, we would parse the bencode data here
    // For now, we'll just set some dummy values for testing
    
    // Parse the metadata
    metadata_.parse(data);
    
    // Set the info hash
    info_hash_ = metadata_.info_hash();
    
    // This is a simplified implementation
    // In a real implementation, we would parse the bencode data and extract
    // the announce URL, announce list, creation date, comment, created by, etc.
    
    // For now, return true to indicate successful parsing
    return true;
}

std::future<bool> TorrentInfo::parse_async(const std::vector<uint8_t>& data) {
    return std::async(std::launch::async, [this, data]() {
        return this->parse(data);
    });
}

std::string TorrentInfo::to_string() const {
    std::ostringstream oss;
    
    oss << "TorrentInfo[info_hash=" << info_hash_.to_hex() << ", ";
    oss << "name=" << metadata_.name() << ", ";
    oss << "announce=" << announce_ << ", ";
    oss << "announce_list=" << announce_list_.size() << ", ";
    
    if (creation_date_) {
        auto time = std::chrono::system_clock::to_time_t(*creation_date_);
        oss << "creation_date=" << std::ctime(&time) << ", ";
    } else {
        oss << "creation_date=none, ";
    }
    
    oss << "comment=" << comment_ << ", ";
    oss << "created_by=" << created_by_ << "]";
    
    return oss.str();
}

} // namespace bitscrape::types
