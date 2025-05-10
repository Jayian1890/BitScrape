#include "bitscrape/tracker/tracker_event_processor.hpp"

#include <algorithm>
#include <future>
#include <stdexcept>

namespace bitscrape::tracker {

TrackerEventProcessor::TrackerEventProcessor()
    : event_bus_(nullptr),
      token_(0),  // Initialize with a default value
      running_(false),
      connection_timeout_ms_(30000),
      request_timeout_ms_(30000) {
}

TrackerEventProcessor::~TrackerEventProcessor() {
    stop();
}

void TrackerEventProcessor::start(event::EventBus& event_bus) {
    if (running_) {
        return;
    }

    event_bus_ = &event_bus;

    // Subscribe to tracker events
    token_ = event_bus_->subscribe<types::Event>([this](const types::Event& event) {
        process_event(event);
    });

    running_ = true;
}

void TrackerEventProcessor::stop() {
    if (!running_) {
        return;
    }

    // Unsubscribe from events
    if (event_bus_) {
        event_bus_->unsubscribe(token_);
    }

    running_ = false;
    event_bus_ = nullptr;
}

bool TrackerEventProcessor::is_running() const {
    return running_;
}

void TrackerEventProcessor::process(const types::Event& event) {
    if (!running_) {
        throw std::runtime_error("TrackerEventProcessor is not running");
    }

    process_event(event);
}

std::future<void> TrackerEventProcessor::process_async(const types::Event& event) {
    return std::async(std::launch::async, [this, &event]() {
        process(event);
    });
}

bool TrackerEventProcessor::add_tracker(const types::InfoHash& info_hash, const std::string& tracker_url) {
    auto manager = get_or_create_tracker_manager(info_hash);
    return manager->add_tracker(tracker_url);
}

bool TrackerEventProcessor::remove_tracker(const types::InfoHash& info_hash, const std::string& tracker_url) {
    std::lock_guard<std::mutex> lock(tracker_managers_mutex_);

    auto it = tracker_managers_.find(info_hash.to_string());
    if (it == tracker_managers_.end()) {
        return false;
    }

    return it->second->remove_tracker(tracker_url);
}

std::vector<std::string> TrackerEventProcessor::tracker_urls(const types::InfoHash& info_hash) const {
    std::lock_guard<std::mutex> lock(tracker_managers_mutex_);

    auto it = tracker_managers_.find(info_hash.to_string());
    if (it == tracker_managers_.end()) {
        return {};
    }

    return it->second->tracker_urls();
}

void TrackerEventProcessor::set_connection_timeout(int timeout_ms) {
    std::lock_guard<std::mutex> lock(tracker_managers_mutex_);

    connection_timeout_ms_ = timeout_ms;

    // Update all tracker managers
    for (auto& [_, manager] : tracker_managers_) {
        manager->set_connection_timeout(timeout_ms);
    }
}

void TrackerEventProcessor::set_request_timeout(int timeout_ms) {
    std::lock_guard<std::mutex> lock(tracker_managers_mutex_);

    request_timeout_ms_ = timeout_ms;

    // Update all tracker managers
    for (auto& [_, manager] : tracker_managers_) {
        manager->set_request_timeout(timeout_ms);
    }
}

bool TrackerEventProcessor::process_event(const types::Event& event) {
    if (!running_) {
        return false;
    }

    // Check if this is a tracker event
    if (event.type() != types::Event::Type::USER_DEFINED) {
        return false;
    }

    // Process the event based on its subtype
    uint32_t subtype = event.subtype();

    if (subtype == static_cast<uint32_t>(TrackerEventType::ANNOUNCE_REQUEST)) {
        const auto& announce_event = static_cast<const AnnounceRequestEvent&>(event);
        process_announce_request(announce_event);
        return true;
    } else if (subtype == static_cast<uint32_t>(TrackerEventType::SCRAPE_REQUEST)) {
        const auto& scrape_event = static_cast<const ScrapeRequestEvent&>(event);
        process_scrape_request(scrape_event);
        return true;
    }

    return false;
}

void TrackerEventProcessor::process_announce_request(const AnnounceRequestEvent& event) {
    if (!event_bus_) {
        return;
    }

    // Get the tracker manager for this info hash
    auto manager = get_or_create_tracker_manager(event.info_hash());

    // Send the announce request asynchronously
    manager->announce_async(
        event.peer_id(),
        event.port(),
        event.uploaded(),
        event.downloaded(),
        event.left(),
        event.event()
    ).then([this, info_hash = event.info_hash()](std::map<std::string, AnnounceResponse> responses) {
        // Process each response
        for (const auto& [url, response] : responses) {
            if (response.has_error()) {
                // Send error event
                event_bus_->publish(TrackerErrorEvent(
                    "Tracker error from " + url + ": " + response.error_message()
                ));
            } else {
                // Send announce response event
                event_bus_->publish(AnnounceResponseEvent(
                    info_hash,
                    response.interval(),
                    response.min_interval(),
                    response.tracker_id(),
                    response.complete(),
                    response.incomplete(),
                    response.peers()
                ));
            }
        }
    });
}

void TrackerEventProcessor::process_scrape_request(const ScrapeRequestEvent& event) {
    if (!event_bus_) {
        return;
    }

    // Process each info hash
    for (const auto& info_hash : event.info_hashes()) {
        // Get the tracker manager for this info hash
        auto manager = get_or_create_tracker_manager(info_hash);

        // Send the scrape request asynchronously
        manager->scrape_async().then([this, info_hash](std::map<std::string, ScrapeResponse> responses) {
            // Combine all responses
            std::map<types::InfoHash, ScrapeResponseEvent::ScrapeData> files;

            // Process each response
            for (const auto& [url, response] : responses) {
                if (response.has_error()) {
                    // Send error event
                    event_bus_->publish(TrackerErrorEvent(
                        "Tracker error from " + url + ": " + response.error_message()
                    ));
                } else {
                    // Add files to the combined map
                    for (const auto& [hash, data] : response.files()) {
                        // If the hash is already in the map, use the response with the most seeders
                        auto it = files.find(hash);
                        if (it == files.end() || data.complete > it->second.complete) {
                            files[hash] = {
                                data.complete,
                                data.downloaded,
                                data.incomplete,
                                data.name
                            };
                        }
                    }
                }
            }

            // Send scrape response event
            event_bus_->publish(ScrapeResponseEvent(files));
        });
    }
}

std::shared_ptr<TrackerManager> TrackerEventProcessor::get_or_create_tracker_manager(const types::InfoHash& info_hash) {
    std::lock_guard<std::mutex> lock(tracker_managers_mutex_);

    std::string hash_str = info_hash.to_string();

    // Check if the manager already exists
    auto it = tracker_managers_.find(hash_str);
    if (it != tracker_managers_.end()) {
        return it->second;
    }

    // Create a new manager
    auto manager = std::make_shared<TrackerManager>(info_hash);
    manager->set_connection_timeout(connection_timeout_ms_);
    manager->set_request_timeout(request_timeout_ms_);

    tracker_managers_[hash_str] = manager;

    return manager;
}

} // namespace bitscrape::tracker
