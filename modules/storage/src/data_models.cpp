#include <bitscrape/storage/database.hpp>
#include <bitscrape/storage/data_models.hpp>

#include <chrono>
#include <sstream>
#include <iomanip>

namespace bitscrape::storage {

// Helper functions for time conversion
namespace {
    std::string time_point_to_string(const std::chrono::system_clock::time_point& time_point) {
        auto time_t = std::chrono::system_clock::to_time_t(time_point);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    std::chrono::system_clock::time_point string_to_time_point(const std::string& str) {
        if (str.empty() || str == "Never" || str == "Unknown") {
            return std::chrono::system_clock::time_point();
        }
        std::tm tm = {};
        std::stringstream ss(str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) {
            return std::chrono::system_clock::time_point();
        }
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
}

// NodeModel implementation
NodeModel NodeModel::from_db_result(const detail::DatabaseResult& result) {
    NodeModel model;

    // Extract node_id from hex string
    std::string node_id_hex = result.get_string("node_id");
    if (!node_id_hex.empty()) {
        try {
            model.node_id = types::NodeID(node_id_hex);
        } catch (const std::exception&) {
            // Invalid hex, stays as default
        }
    }

    // Extract endpoint
    try {
        std::string ip = result.get_string("ip");
        int port = result.get_int("port");
        model.endpoint = types::Endpoint(ip, port);
    } catch (const std::exception&) {
        // Invalid endpoint in database, leave as default (invalid)
        // This prevents get_node from crashing on corrupted data
    }

    // Extract timestamps
    model.first_seen = string_to_time_point(result.get_string("first_seen"));
    model.last_seen = string_to_time_point(result.get_string("last_seen"));

    // Extract counters
    model.ping_count = static_cast<uint32_t>(result.get_int("ping_count"));
    model.query_count = static_cast<uint32_t>(result.get_int("query_count"));
    model.response_count = static_cast<uint32_t>(result.get_int("response_count"));
    model.last_rtt_ms = static_cast<uint32_t>(result.get_int("last_rtt_ms"));

    // Extract flags
    model.is_responsive = result.get_int("is_responsive") != 0;

    return model;
}

std::vector<std::string> NodeModel::to_sql_params() const {
    std::vector<std::string> params;

    // Convert node_id to hex string
    std::string node_id_hex = node_id.to_hex();
    params.push_back(node_id_hex);

    // Convert endpoint to string
    params.push_back(endpoint.address());
    params.push_back(std::to_string(endpoint.port()));

    // Convert timestamps to strings
    params.push_back(time_point_to_string(first_seen));
    params.push_back(time_point_to_string(last_seen));

    // Convert counters to strings
    params.push_back(std::to_string(ping_count));
    params.push_back(std::to_string(query_count));
    params.push_back(std::to_string(response_count));
    params.push_back(std::to_string(last_rtt_ms));

    // Convert flags to strings
    params.push_back(is_responsive ? "1" : "0");

    return params;
}

// InfoHashModel implementation
InfoHashModel InfoHashModel::from_db_result(const detail::DatabaseResult& result) {
    InfoHashModel model;

    // Extract info_hash from hex string
    std::string info_hash_hex = result.get_string("info_hash");
    if (!info_hash_hex.empty()) {
        try {
            model.info_hash = types::InfoHash(info_hash_hex);
        } catch (const std::exception&) {
            // Invalid hex, stays as default
        }
    }

    // Extract timestamps
    model.first_seen = string_to_time_point(result.get_string("first_seen"));
    model.last_seen = string_to_time_point(result.get_string("last_seen"));

    // Extract counters
    model.announce_count = static_cast<uint32_t>(result.get_int("announce_count"));
    model.peer_count = static_cast<uint32_t>(result.get_int("peer_count"));

    // Extract flags
    model.has_metadata = result.get_int("has_metadata") != 0;

    return model;
}

std::vector<std::string> InfoHashModel::to_sql_params() const {
    std::vector<std::string> params;

    // Convert info_hash to hex string
    std::string info_hash_hex = info_hash.to_hex();
    params.push_back(info_hash_hex);

    // Convert timestamps to strings
    params.push_back(time_point_to_string(first_seen));
    params.push_back(time_point_to_string(last_seen));

    // Convert counters to strings
    params.push_back(std::to_string(announce_count));
    params.push_back(std::to_string(peer_count));

    // Convert flags to strings
    params.push_back(has_metadata ? "1" : "0");

    return params;
}

// MetadataModel implementation
MetadataModel MetadataModel::from_db_result(const detail::DatabaseResult& result) {
    MetadataModel model;

    // Extract info_hash from hex string
    std::string info_hash_hex = result.get_string("info_hash");
    if (!info_hash_hex.empty()) {
        try {
            model.info_hash = types::InfoHash(info_hash_hex);
        } catch (const std::exception&) {
            // Invalid hex, stays as default
        }
    }

    // Extract metadata from raw_metadata blob
    auto raw_metadata_blob = result.get_blob("raw_metadata");
    if (!raw_metadata_blob.empty()) {
        // In a real implementation, we would deserialize the metadata from the blob
        // For now, we'll just set some basic properties
        model.metadata = types::MetadataInfo();
    }

    // Extract timestamps
    model.download_time = string_to_time_point(result.get_string("download_time"));

    // Extract basic properties
    model.name = result.get_string("name");
    model.total_size = static_cast<uint64_t>(result.get_int64("total_size"));
    model.piece_count = static_cast<uint32_t>(result.get_int("piece_count"));
    model.file_count = static_cast<uint32_t>(result.get_int("file_count"));
    model.comment = result.get_string("comment");
    model.created_by = result.get_string("created_by");

    // Extract optional creation date
    if (!result.is_null("creation_date")) {
        model.creation_date = string_to_time_point(result.get_string("creation_date"));
    }

    return model;
}

std::vector<std::string> MetadataModel::to_sql_params() const {
    std::vector<std::string> params;

    // Convert info_hash to hex string
    std::string info_hash_hex = info_hash.to_hex();
    params.push_back(info_hash_hex);

    // Convert timestamps to strings
    params.push_back(time_point_to_string(download_time));

    // Convert basic properties to strings
    params.push_back(name);
    params.push_back(std::to_string(total_size));
    params.push_back(std::to_string(piece_count));
    params.push_back(std::to_string(file_count));
    params.push_back(comment);
    params.push_back(created_by);

    // Convert optional creation date
    if (creation_date) {
        params.push_back(time_point_to_string(*creation_date));
    } else {
        params.push_back("");
    }

    // In a real implementation, we would serialize the metadata to a blob
    // For now, we'll just use a placeholder
    params.push_back("raw_metadata_placeholder");

    return params;
}

// FileModel implementation
FileModel FileModel::from_db_result(const detail::DatabaseResult& result) {
    FileModel model;

    // Extract info_hash from hex string
    std::string info_hash_hex = result.get_string("info_hash");
    if (!info_hash_hex.empty()) {
        try {
            model.info_hash = types::InfoHash(info_hash_hex);
        } catch (const std::exception&) {
            // Invalid hex, stays as default
        }
    }

    // Extract basic properties
    model.path = result.get_string("path");
    model.size = static_cast<uint64_t>(result.get_int64("size"));

    return model;
}

std::vector<std::string> FileModel::to_sql_params() const {
    std::vector<std::string> params;

    // Convert info_hash to hex string
    std::string info_hash_hex = info_hash.to_hex();
    params.push_back(info_hash_hex);

    // Convert basic properties to strings
    params.push_back(path);
    params.push_back(std::to_string(size));

    return params;
}

// TrackerModel implementation
TrackerModel TrackerModel::from_db_result(const detail::DatabaseResult& result) {
    TrackerModel model;

    // Extract info_hash from hex string
    std::string info_hash_hex = result.get_string("info_hash");
    if (!info_hash_hex.empty()) {
        try {
            model.info_hash = types::InfoHash(info_hash_hex);
        } catch (const std::exception&) {
            // Invalid hex, stays as default
        }
    }

    // Extract basic properties
    model.url = result.get_string("url");

    // Extract timestamps
    model.first_seen = string_to_time_point(result.get_string("first_seen"));
    model.last_seen = string_to_time_point(result.get_string("last_seen"));

    // Extract counters
    model.announce_count = static_cast<uint32_t>(result.get_int("announce_count"));
    model.scrape_count = static_cast<uint32_t>(result.get_int("scrape_count"));

    return model;
}

std::vector<std::string> TrackerModel::to_sql_params() const {
    std::vector<std::string> params;

    // Convert info_hash to hex string
    std::string info_hash_hex = info_hash.to_hex();
    params.push_back(info_hash_hex);

    // Convert basic properties to strings
    params.push_back(url);

    // Convert timestamps to strings
    params.push_back(time_point_to_string(first_seen));
    params.push_back(time_point_to_string(last_seen));

    // Convert counters to strings
    params.push_back(std::to_string(announce_count));
    params.push_back(std::to_string(scrape_count));

    return params;
}

// PeerModel implementation
PeerModel PeerModel::from_db_result(const detail::DatabaseResult& result) {
    PeerModel model;

    // Extract info_hash from hex string
    std::string info_hash_hex = result.get_string("info_hash");
    if (!info_hash_hex.empty()) {
        try {
            model.info_hash = types::InfoHash(info_hash_hex);
        } catch (const std::exception&) {
            // Invalid hex, stays as default
        }
    }

    // Extract endpoint
    std::string ip = result.get_string("ip");
    int port = result.get_int("port");
    model.endpoint = types::Endpoint(ip, port);

    // Extract peer_id from hex string if not null
    if (!result.is_null("peer_id")) {
        std::string peer_id_hex = result.get_string("peer_id");
        if (!peer_id_hex.empty()) {
            try {
                model.peer_id = types::NodeID(peer_id_hex);
            } catch (const std::exception&) {
                // Invalid hex, stays as nullopt
            }
        }
    }

    // Extract timestamps
    model.first_seen = string_to_time_point(result.get_string("first_seen"));
    model.last_seen = string_to_time_point(result.get_string("last_seen"));

    // Extract flags
    model.supports_dht = result.get_int("supports_dht") != 0;
    model.supports_extension_protocol = result.get_int("supports_extension_protocol") != 0;
    model.supports_fast_protocol = result.get_int("supports_fast_protocol") != 0;

    return model;
}

std::vector<std::string> PeerModel::to_sql_params() const {
    std::vector<std::string> params;

    // Convert info_hash to hex string
    std::string info_hash_hex = info_hash.to_hex();
    params.push_back(info_hash_hex);

    // Convert endpoint to string
    params.push_back(endpoint.address());
    params.push_back(std::to_string(endpoint.port()));

    // Convert peer_id to hex string if available
    if (peer_id) {
        params.push_back(peer_id->to_hex());
    } else {
        params.push_back("");
    }

    // Convert timestamps to strings
    params.push_back(time_point_to_string(first_seen));
    params.push_back(time_point_to_string(last_seen));

    // Convert flags to strings
    params.push_back(supports_dht ? "1" : "0");
    params.push_back(supports_extension_protocol ? "1" : "0");
    params.push_back(supports_fast_protocol ? "1" : "0");

    return params;
}

} // namespace bitscrape::storage
