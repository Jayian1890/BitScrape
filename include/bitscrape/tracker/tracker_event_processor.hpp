#pragma once

#include "bitscrape/event/event_processor.hpp"
#include "bitscrape/types/event_types.hpp"
#include "bitscrape/tracker/tracker_event.hpp"
#include "bitscrape/tracker/tracker_manager.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <future>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace bitscrape::tracker {

/**
 * @brief Event processor for tracker events
 * 
 * This class processes tracker events and manages tracker managers for different torrents.
 */
class TrackerEventProcessor : public event::EventProcessor {
public:
    /**
     * @brief Constructor
     */
    TrackerEventProcessor();
    
    /**
     * @brief Destructor
     */
    ~TrackerEventProcessor() override;
    
    /**
     * @brief Start processing events
     * 
     * @param event_bus Event bus to process events from
     */
    void start(event::EventBus& event_bus) override;
    
    /**
     * @brief Stop processing events
     */
    void stop() override;
    
    /**
     * @brief Check if the processor is running
     * 
     * @return true if the processor is running, false otherwise
     */
    [[nodiscard]] bool is_running() const override;
    
    /**
     * @brief Process an event
     * 
     * @param event Event to process
     */
    void process(const types::Event& event) override;
    
    /**
     * @brief Process an event asynchronously
     * 
     * @param event Event to process
     * @return Future that will be completed when the event has been processed
     */
    std::future<void> process_async(const types::Event& event) override;
    
    /**
     * @brief Add a tracker for a torrent
     * 
     * @param info_hash Torrent info hash
     * @param tracker_url Tracker URL
     * @return true if the tracker was added, false otherwise
     */
    bool add_tracker(const types::InfoHash& info_hash, const std::string& tracker_url);
    
    /**
     * @brief Remove a tracker for a torrent
     * 
     * @param info_hash Torrent info hash
     * @param tracker_url Tracker URL
     * @return true if the tracker was removed, false otherwise
     */
    bool remove_tracker(const types::InfoHash& info_hash, const std::string& tracker_url);
    
    /**
     * @brief Get the list of tracker URLs for a torrent
     * 
     * @param info_hash Torrent info hash
     * @return List of tracker URLs
     */
    [[nodiscard]] std::vector<std::string> tracker_urls(const types::InfoHash& info_hash) const;
    
    /**
     * @brief Set the connection timeout for all trackers
     * 
     * @param timeout_ms Timeout in milliseconds
     */
    void set_connection_timeout(int timeout_ms);
    
    /**
     * @brief Set the request timeout for all trackers
     * 
     * @param timeout_ms Timeout in milliseconds
     */
    void set_request_timeout(int timeout_ms);
    
private:
    /**
     * @brief Process a tracker event
     * 
     * @param event Event to process
     * @return true if the event was processed, false otherwise
     */
    bool process_event(const types::Event& event);
    
    /**
     * @brief Process an announce request event
     * 
     * @param event Announce request event
     */
    void process_announce_request(const AnnounceRequestEvent& event);
    
    /**
     * @brief Process a scrape request event
     * 
     * @param event Scrape request event
     */
    void process_scrape_request(const ScrapeRequestEvent& event);
    
    /**
     * @brief Get or create a tracker manager for a torrent
     * 
     * @param info_hash Torrent info hash
     * @return Tracker manager
     */
    std::shared_ptr<TrackerManager> get_or_create_tracker_manager(const types::InfoHash& info_hash);
    
    std::unordered_map<std::string, std::shared_ptr<TrackerManager>> tracker_managers_; ///< Tracker managers by info hash
    mutable std::mutex tracker_managers_mutex_; ///< Mutex for tracker managers map
    
    event::EventBus* event_bus_; ///< Event bus
    types::SubscriptionToken token_; ///< Event subscription token
    std::atomic<bool> running_; ///< Whether the processor is running
    
    int connection_timeout_ms_; ///< Connection timeout in milliseconds
    int request_timeout_ms_; ///< Request timeout in milliseconds
};

} // namespace bitscrape::tracker
