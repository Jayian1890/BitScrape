        if (peer_manager->start()) {
        // Create a metadata exchange for this infohash
        auto metadata_exchange = std::make_shared<bittorrent::MetadataExchange>(
            peer_manager->protocol(), beacon_);
        metadata_exchange->initialize();

        // Set callback to publish event when metadata is received
        metadata_exchange->set_metadata_received_callback([this, info_hash](const types::MetadataInfo& metadata) {
            event_bus_->publish(bittorrent::MetadataReceivedEvent(info_hash, metadata));
        });

        // Create a tracker manager for this infohash
        auto tracker_manager =
            std::make_shared<tracker::TrackerManager>(info_hash);

        // Set timeouts from configuration
        int connection_timeout =
            config_->get_int("bittorrent.connection_timeout", 10) *
            1000; // Convert to ms
        int request_timeout =
            config_->get_int("bittorrent.download_timeout", 30) * 1000;
        // Convert to ms
        tracker_manager->set_connection_timeout(connection_timeout);
        tracker_manager->set_request_timeout(request_timeout);
