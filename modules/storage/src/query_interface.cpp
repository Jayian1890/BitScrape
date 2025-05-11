#include <bitscrape/storage/query_interface.hpp>
#include <bitscrape/storage/database.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>

namespace bitscrape::storage {

class QueryInterface::Impl {
public:
    Impl(std::shared_ptr<Database> database)
        : database_(database) {
    }

    std::optional<NodeModel> get_node(const types::NodeID& node_id) {
        // Convert node_id to hex string for query
        std::string node_id_hex = node_id.to_hex();

        auto result = database_->execute(
            "SELECT * FROM nodes WHERE node_id = ?",
            {node_id_hex}
        );

        if (result.next()) {
            return NodeModel::from_db_result(result);
        }

        return std::nullopt;
    }

    std::future<std::optional<NodeModel>> get_node_async(const types::NodeID& node_id) {
        return std::async(std::launch::async, [this, node_id]() {
            return get_node(node_id);
        });
    }

    std::vector<NodeModel> get_nodes(const NodeQueryOptions& options) {
        // Build query
        std::stringstream ss;
        ss << "SELECT * FROM nodes WHERE 1=1";

        std::vector<std::string> params;

        // Add filters
        if (options.min_last_seen) {
            ss << " AND last_seen >= ?";
            params.push_back(time_point_to_string(*options.min_last_seen));
        }

        if (options.max_last_seen) {
            ss << " AND last_seen <= ?";
            params.push_back(time_point_to_string(*options.max_last_seen));
        }

        if (options.is_responsive) {
            ss << " AND is_responsive = ?";
            params.push_back(*options.is_responsive ? "1" : "0");
        }

        if (options.min_ping_count) {
            ss << " AND ping_count >= ?";
            params.push_back(std::to_string(*options.min_ping_count));
        }

        if (options.min_response_count) {
            ss << " AND response_count >= ?";
            params.push_back(std::to_string(*options.min_response_count));
        }

        // Add order by
        if (options.order_by) {
            ss << " ORDER BY " << *options.order_by;
            if (options.order_desc && *options.order_desc) {
                ss << " DESC";
            } else {
                ss << " ASC";
            }
        } else {
            ss << " ORDER BY last_seen DESC";
        }

        // Add limit and offset
        if (options.limit) {
            ss << " LIMIT ?";
            params.push_back(std::to_string(*options.limit));

            if (options.offset) {
                ss << " OFFSET ?";
                params.push_back(std::to_string(*options.offset));
            }
        }

        // Execute query
        auto result = database_->execute(ss.str(), params);

        // Parse results
        std::vector<NodeModel> nodes;
        while (result.next()) {
            nodes.push_back(NodeModel::from_db_result(result));
        }

        return nodes;
    }

    std::future<std::vector<NodeModel>> get_nodes_async(const NodeQueryOptions& options) {
        return std::async(std::launch::async, [this, options]() {
            return get_nodes(options);
        });
    }

    std::optional<InfoHashModel> get_infohash(const types::InfoHash& info_hash) {
        // Convert info_hash to hex string for query
        std::string info_hash_hex = info_hash.to_hex();

        auto result = database_->execute(
            "SELECT * FROM infohashes WHERE info_hash = ?",
            {info_hash_hex}
        );

        if (result.next()) {
            return InfoHashModel::from_db_result(result);
        }

        return std::nullopt;
    }

    std::future<std::optional<InfoHashModel>> get_infohash_async(const types::InfoHash& info_hash) {
        return std::async(std::launch::async, [this, info_hash]() {
            return get_infohash(info_hash);
        });
    }

    std::vector<InfoHashModel> get_infohashes(const InfoHashQueryOptions& options) {
        // Build query
        std::stringstream ss;
        ss << "SELECT * FROM infohashes WHERE 1=1";

        std::vector<std::string> params;

        // Add filters
        if (options.min_last_seen) {
            ss << " AND last_seen >= ?";
            params.push_back(time_point_to_string(*options.min_last_seen));
        }

        if (options.max_last_seen) {
            ss << " AND last_seen <= ?";
            params.push_back(time_point_to_string(*options.max_last_seen));
        }

        if (options.has_metadata) {
            ss << " AND has_metadata = ?";
            params.push_back(*options.has_metadata ? "1" : "0");
        }

        if (options.min_announce_count) {
            ss << " AND announce_count >= ?";
            params.push_back(std::to_string(*options.min_announce_count));
        }

        if (options.min_peer_count) {
            ss << " AND peer_count >= ?";
            params.push_back(std::to_string(*options.min_peer_count));
        }

        // Add order by
        if (options.order_by) {
            ss << " ORDER BY " << *options.order_by;
            if (options.order_desc && *options.order_desc) {
                ss << " DESC";
            } else {
                ss << " ASC";
            }
        } else {
            ss << " ORDER BY last_seen DESC";
        }

        // Add limit and offset
        if (options.limit) {
            ss << " LIMIT ?";
            params.push_back(std::to_string(*options.limit));

            if (options.offset) {
                ss << " OFFSET ?";
                params.push_back(std::to_string(*options.offset));
            }
        }

        // Execute query
        auto result = database_->execute(ss.str(), params);

        // Parse results
        std::vector<InfoHashModel> infohashes;
        while (result.next()) {
            infohashes.push_back(InfoHashModel::from_db_result(result));
        }

        return infohashes;
    }

    std::future<std::vector<InfoHashModel>> get_infohashes_async(const InfoHashQueryOptions& options) {
        return std::async(std::launch::async, [this, options]() {
            return get_infohashes(options);
        });
    }

    std::optional<MetadataModel> get_metadata(const types::InfoHash& info_hash) {
        // Convert info_hash to hex string for query
        std::string info_hash_hex = info_hash.to_hex();

        auto result = database_->execute(
            "SELECT * FROM metadata WHERE info_hash = ?",
            {info_hash_hex}
        );

        if (result.next()) {
            return MetadataModel::from_db_result(result);
        }

        return std::nullopt;
    }

    std::future<std::optional<MetadataModel>> get_metadata_async(const types::InfoHash& info_hash) {
        return std::async(std::launch::async, [this, info_hash]() {
            return get_metadata(info_hash);
        });
    }

    std::vector<MetadataModel> get_metadata_list(const MetadataQueryOptions& options) {
        // Build query
        std::stringstream ss;
        ss << "SELECT * FROM metadata WHERE 1=1";

        std::vector<std::string> params;

        // Add filters
        if (options.name_contains) {
            ss << " AND name LIKE ?";
            params.push_back("%" + *options.name_contains + "%");
        }

        if (options.min_size) {
            ss << " AND total_size >= ?";
            params.push_back(std::to_string(*options.min_size));
        }

        if (options.max_size) {
            ss << " AND total_size <= ?";
            params.push_back(std::to_string(*options.max_size));
        }

        if (options.min_file_count) {
            ss << " AND file_count >= ?";
            params.push_back(std::to_string(*options.min_file_count));
        }

        if (options.min_download_time) {
            ss << " AND download_time >= ?";
            params.push_back(time_point_to_string(*options.min_download_time));
        }

        if (options.max_download_time) {
            ss << " AND download_time <= ?";
            params.push_back(time_point_to_string(*options.max_download_time));
        }

        // Add order by
        if (options.order_by) {
            ss << " ORDER BY " << *options.order_by;
            if (options.order_desc && *options.order_desc) {
                ss << " DESC";
            } else {
                ss << " ASC";
            }
        } else {
            ss << " ORDER BY download_time DESC";
        }

        // Add limit and offset
        if (options.limit) {
            ss << " LIMIT ?";
            params.push_back(std::to_string(*options.limit));

            if (options.offset) {
                ss << " OFFSET ?";
                params.push_back(std::to_string(*options.offset));
            }
        }

        // Execute query
        auto result = database_->execute(ss.str(), params);

        // Parse results
        std::vector<MetadataModel> metadata_list;
        while (result.next()) {
            metadata_list.push_back(MetadataModel::from_db_result(result));
        }

        return metadata_list;
    }

    std::future<std::vector<MetadataModel>> get_metadata_list_async(const MetadataQueryOptions& options) {
        return std::async(std::launch::async, [this, options]() {
            return get_metadata_list(options);
        });
    }

    std::vector<FileModel> get_files(const types::InfoHash& info_hash) {
        // Convert info_hash to hex string for query
        std::string info_hash_hex = info_hash.to_hex();

        auto result = database_->execute(
            "SELECT * FROM files WHERE info_hash = ? ORDER BY path",
            {info_hash_hex}
        );

        std::vector<FileModel> files;
        while (result.next()) {
            files.push_back(FileModel::from_db_result(result));
        }

        return files;
    }

    std::future<std::vector<FileModel>> get_files_async(const types::InfoHash& info_hash) {
        return std::async(std::launch::async, [this, info_hash]() {
            return get_files(info_hash);
        });
    }

    std::vector<TrackerModel> get_trackers(const types::InfoHash& info_hash) {
        // Convert info_hash to hex string for query
        std::string info_hash_hex = info_hash.to_hex();

        auto result = database_->execute(
            "SELECT * FROM trackers WHERE info_hash = ? ORDER BY url",
            {info_hash_hex}
        );

        std::vector<TrackerModel> trackers;
        while (result.next()) {
            trackers.push_back(TrackerModel::from_db_result(result));
        }

        return trackers;
    }

    std::future<std::vector<TrackerModel>> get_trackers_async(const types::InfoHash& info_hash) {
        return std::async(std::launch::async, [this, info_hash]() {
            return get_trackers(info_hash);
        });
    }

    std::vector<PeerModel> get_peers(const types::InfoHash& info_hash) {
        // Convert info_hash to hex string for query
        std::string info_hash_hex = info_hash.to_hex();

        auto result = database_->execute(
            "SELECT * FROM peers WHERE info_hash = ? ORDER BY last_seen DESC",
            {info_hash_hex}
        );

        std::vector<PeerModel> peers;
        while (result.next()) {
            peers.push_back(PeerModel::from_db_result(result));
        }

        return peers;
    }

    std::future<std::vector<PeerModel>> get_peers_async(const types::InfoHash& info_hash) {
        return std::async(std::launch::async, [this, info_hash]() {
            return get_peers(info_hash);
        });
    }

    uint64_t count_nodes(const NodeQueryOptions& options) {
        // Build query
        std::stringstream ss;
        ss << "SELECT COUNT(*) AS count FROM nodes WHERE 1=1";

        std::vector<std::string> params;

        // Add filters
        if (options.min_last_seen) {
            ss << " AND last_seen >= ?";
            params.push_back(time_point_to_string(*options.min_last_seen));
        }

        if (options.max_last_seen) {
            ss << " AND last_seen <= ?";
            params.push_back(time_point_to_string(*options.max_last_seen));
        }

        if (options.is_responsive) {
            ss << " AND is_responsive = ?";
            params.push_back(*options.is_responsive ? "1" : "0");
        }

        if (options.min_ping_count) {
            ss << " AND ping_count >= ?";
            params.push_back(std::to_string(*options.min_ping_count));
        }

        if (options.min_response_count) {
            ss << " AND response_count >= ?";
            params.push_back(std::to_string(*options.min_response_count));
        }

        // Execute query
        auto result = database_->execute(ss.str(), params);

        if (result.next()) {
            return static_cast<uint64_t>(result.get_int64("count"));
        }

        return 0;
    }

    std::future<uint64_t> count_nodes_async(const NodeQueryOptions& options) {
        return std::async(std::launch::async, [this, options]() {
            return count_nodes(options);
        });
    }

    uint64_t count_infohashes(const InfoHashQueryOptions& options) {
        // Build query
        std::stringstream ss;
        ss << "SELECT COUNT(*) AS count FROM infohashes WHERE 1=1";

        std::vector<std::string> params;

        // Add filters
        if (options.min_last_seen) {
            ss << " AND last_seen >= ?";
            params.push_back(time_point_to_string(*options.min_last_seen));
        }

        if (options.max_last_seen) {
            ss << " AND last_seen <= ?";
            params.push_back(time_point_to_string(*options.max_last_seen));
        }

        if (options.has_metadata) {
            ss << " AND has_metadata = ?";
            params.push_back(*options.has_metadata ? "1" : "0");
        }

        if (options.min_announce_count) {
            ss << " AND announce_count >= ?";
            params.push_back(std::to_string(*options.min_announce_count));
        }

        if (options.min_peer_count) {
            ss << " AND peer_count >= ?";
            params.push_back(std::to_string(*options.min_peer_count));
        }

        // Execute query
        auto result = database_->execute(ss.str(), params);

        if (result.next()) {
            return static_cast<uint64_t>(result.get_int64("count"));
        }

        return 0;
    }

    std::future<uint64_t> count_infohashes_async(const InfoHashQueryOptions& options) {
        return std::async(std::launch::async, [this, options]() {
            return count_infohashes(options);
        });
    }

    uint64_t count_metadata(const MetadataQueryOptions& options) {
        // Build query
        std::stringstream ss;
        ss << "SELECT COUNT(*) AS count FROM metadata WHERE 1=1";

        std::vector<std::string> params;

        // Add filters
        if (options.name_contains) {
            ss << " AND name LIKE ?";
            params.push_back("%" + *options.name_contains + "%");
        }

        if (options.min_size) {
            ss << " AND total_size >= ?";
            params.push_back(std::to_string(*options.min_size));
        }

        if (options.max_size) {
            ss << " AND total_size <= ?";
            params.push_back(std::to_string(*options.max_size));
        }

        if (options.min_file_count) {
            ss << " AND file_count >= ?";
            params.push_back(std::to_string(*options.min_file_count));
        }

        if (options.min_download_time) {
            ss << " AND download_time >= ?";
            params.push_back(time_point_to_string(*options.min_download_time));
        }

        if (options.max_download_time) {
            ss << " AND download_time <= ?";
            params.push_back(time_point_to_string(*options.max_download_time));
        }

        // Execute query
        auto result = database_->execute(ss.str(), params);

        if (result.next()) {
            return static_cast<uint64_t>(result.get_int64("count"));
        }

        return 0;
    }

    std::future<uint64_t> count_metadata_async(const MetadataQueryOptions& options) {
        return std::async(std::launch::async, [this, options]() {
            return count_metadata(options);
        });
    }

    // Helper function to convert time_point to string
    std::string time_point_to_string(const std::chrono::system_clock::time_point& time_point) {
        auto time_t = std::chrono::system_clock::to_time_t(time_point);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

private:
    std::shared_ptr<Database> database_;
};

// QueryInterface public methods

QueryInterface::QueryInterface(std::shared_ptr<Database> database)
    : impl_(std::make_unique<Impl>(database)) {
}

QueryInterface::~QueryInterface() = default;

std::optional<NodeModel> QueryInterface::get_node(const types::NodeID& node_id) {
    return impl_->get_node(node_id);
}

std::future<std::optional<NodeModel>> QueryInterface::get_node_async(const types::NodeID& node_id) {
    return impl_->get_node_async(node_id);
}

std::vector<NodeModel> QueryInterface::get_nodes(const NodeQueryOptions& options) {
    return impl_->get_nodes(options);
}

std::future<std::vector<NodeModel>> QueryInterface::get_nodes_async(const NodeQueryOptions& options) {
    return impl_->get_nodes_async(options);
}

std::optional<InfoHashModel> QueryInterface::get_infohash(const types::InfoHash& info_hash) {
    return impl_->get_infohash(info_hash);
}

std::future<std::optional<InfoHashModel>> QueryInterface::get_infohash_async(const types::InfoHash& info_hash) {
    return impl_->get_infohash_async(info_hash);
}

std::vector<InfoHashModel> QueryInterface::get_infohashes(const InfoHashQueryOptions& options) {
    return impl_->get_infohashes(options);
}

std::future<std::vector<InfoHashModel>> QueryInterface::get_infohashes_async(const InfoHashQueryOptions& options) {
    return impl_->get_infohashes_async(options);
}

std::optional<MetadataModel> QueryInterface::get_metadata(const types::InfoHash& info_hash) {
    return impl_->get_metadata(info_hash);
}

std::future<std::optional<MetadataModel>> QueryInterface::get_metadata_async(const types::InfoHash& info_hash) {
    return impl_->get_metadata_async(info_hash);
}

std::vector<MetadataModel> QueryInterface::get_metadata_list(const MetadataQueryOptions& options) {
    return impl_->get_metadata_list(options);
}

std::future<std::vector<MetadataModel>> QueryInterface::get_metadata_list_async(const MetadataQueryOptions& options) {
    return impl_->get_metadata_list_async(options);
}

std::vector<FileModel> QueryInterface::get_files(const types::InfoHash& info_hash) {
    return impl_->get_files(info_hash);
}

std::future<std::vector<FileModel>> QueryInterface::get_files_async(const types::InfoHash& info_hash) {
    return impl_->get_files_async(info_hash);
}

std::vector<TrackerModel> QueryInterface::get_trackers(const types::InfoHash& info_hash) {
    return impl_->get_trackers(info_hash);
}

std::future<std::vector<TrackerModel>> QueryInterface::get_trackers_async(const types::InfoHash& info_hash) {
    return impl_->get_trackers_async(info_hash);
}

std::vector<PeerModel> QueryInterface::get_peers(const types::InfoHash& info_hash) {
    return impl_->get_peers(info_hash);
}

std::future<std::vector<PeerModel>> QueryInterface::get_peers_async(const types::InfoHash& info_hash) {
    return impl_->get_peers_async(info_hash);
}

uint64_t QueryInterface::count_nodes(const NodeQueryOptions& options) {
    return impl_->count_nodes(options);
}

std::future<uint64_t> QueryInterface::count_nodes_async(const NodeQueryOptions& options) {
    return impl_->count_nodes_async(options);
}

uint64_t QueryInterface::count_infohashes(const InfoHashQueryOptions& options) {
    return impl_->count_infohashes(options);
}

std::future<uint64_t> QueryInterface::count_infohashes_async(const InfoHashQueryOptions& options) {
    return impl_->count_infohashes_async(options);
}

uint64_t QueryInterface::count_metadata(const MetadataQueryOptions& options) {
    return impl_->count_metadata(options);
}

std::future<uint64_t> QueryInterface::count_metadata_async(const MetadataQueryOptions& options) {
    return impl_->count_metadata_async(options);
}

} // namespace bitscrape::storage
