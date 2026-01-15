            {"dht.bootstrap_trackers", "udp://tracker.opentrackr.org:1337/announce,udp://tracker.torrent.eu.org:451/announce"},
            {"bittorrent.max_connections", "50"},
            {"bittorrent.connection_timeout", "10"},
            {"bittorrent.download_timeout", "30"},
            {"tracker.announce_interval", "1800"},
            {"tracker.max_trackers", "20"},
            {"tracker.default_trackers", "udp://tracker.opentrackr.org:1337/announce,udp://tracker.torrent.eu.org:451/announce"},
            {"log.level", "debug"},
            {"log.file", (std::filesystem::path(base_dir) / "bitscrape.log").string()},
            {"log.max_size", "10485760"},
            {"log.max_files", "5"},
            {"web.auto_start", "true"},
            {"web.port", "8080"},
            {"web.static_dir", "public"},
            {"crawler.random_discovery", "true"}
        };

        for (const auto& [key, value] : defaults) {
            if (config_.find(key) == config_.end()) {
                config_[key] = value;
            }
        }
    }

    void create_default_configuration() {
        std::string base_dir = Configuration::get_default_base_dir();
        // Set default values
        config_["database.path"] = (std::filesystem::path(base_dir) / "bitscrape.db").string();
        config_["dht.bootstrap_nodes"] = "dht.aelitis.com:6881,router.utorrent.com:6881,router.bittorrent.com:6881"; // Default public routers
        config_["dht.port"] = "6881";
        config_["dht.node_id"] = ""; // Will be generated randomly if empty
        config_["dht.max_nodes"] = "1000";
        config_["dht.ping_interval"] = "300"; // 5 minutes
        config_["dht.bootstrap_infohash"] = "d2474e86c95b19b8bcfdb92bc12c9d44667cfa36"; // OpenOffice 3.3 swarm
        config_["dht.bootstrap_trackers"] = "udp://tracker.opentrackr.org:1337/announce,udp://tracker.torrent.eu.org:451/announce";
        config_["bittorrent.max_connections"] = "50";
        config_["bittorrent.connection_timeout"] = "10"; // 10 seconds
        config_["bittorrent.download_timeout"] = "30"; // 30 seconds
        config_["tracker.announce_interval"] = "1800"; // 30 minutes
        config_["tracker.max_trackers"] = "20";
        config_["log.level"] = "debug";
        config_["log.file"] = (std::filesystem::path(base_dir) / "bitscrape.log").string();
        config_["log.max_size"] = "10485760"; // 10 MB
        config_["log.max_files"] = "5";
        config_["web.auto_start"] = "true"; // Auto-start web interface by default
        config_["web.port"] = "8080"; // Default web interface port
        config_["web.static_dir"] = "public"; // Default static files directory

        save();
    }

    std::string config_path_;
    std::unordered_map<std::string, std::string> config_;
    uint64_t config_resource_id_; // Resource ID for the configuration
};

// Configuration implementation

Configuration::Configuration(const std::string& config_path)
    : impl_(std::make_unique<Impl>(config_path)) {
}
