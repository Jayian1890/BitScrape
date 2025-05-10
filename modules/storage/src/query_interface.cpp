#include <bitscrape/storage/query_interface.hpp>

#include <iostream>
#include <sstream>

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

// TODO: Implement remaining methods

} // namespace bitscrape::storage
