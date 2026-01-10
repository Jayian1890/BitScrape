#pragma once

#include "bitscrape/types/event_types.hpp"
#include "bitscrape/types/info_hash.hpp"
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <chrono>

namespace bitscrape::tracker {

/**
 * @brief Tracker statistics event types
 */
enum class TrackerStatisticsEventType : uint16_t {
    TRACKER_STATS_UPDATE = 2500,     ///< Periodic tracker statistics update
    TRACKER_PERFORMANCE_REPORT,      ///< Tracker performance report
    TRACKER_AVAILABILITY_CHANGE,     ///< Tracker availability status change
    TRACKER_SWARM_SIZE_THRESHOLD     ///< Swarm size threshold reached for an infohash
};

/**
 * @brief Base class for tracker statistics events
 */
class TrackerStatisticsEvent : public types::Event {
public:
    /**
     * @brief Create a new tracker statistics event
     *
     * @param type Statistics event type
     */
    explicit TrackerStatisticsEvent(TrackerStatisticsEventType type)
        : types::Event(types::Event::Type::USER_DEFINED, static_cast<uint32_t>(type)),
          statistics_event_type_(type) {}

    /**
     * @brief Get the statistics event type
     *
     * @return Statistics event type
     */
    TrackerStatisticsEventType statistics_event_type() const { return statistics_event_type_; }

    /**
     * @brief Clone the event
     *
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<TrackerStatisticsEvent>(*this);
    }

    /**
     * @brief Convert the event to a string representation
     *
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = types::Event::to_string();
        std::string type;

        switch (statistics_event_type_) {
            case TrackerStatisticsEventType::TRACKER_STATS_UPDATE:
                type = "TRACKER_STATS_UPDATE";
                break;
            case TrackerStatisticsEventType::TRACKER_PERFORMANCE_REPORT:
                type = "TRACKER_PERFORMANCE_REPORT";
                break;
            case TrackerStatisticsEventType::TRACKER_AVAILABILITY_CHANGE:
                type = "TRACKER_AVAILABILITY_CHANGE";
                break;
            case TrackerStatisticsEventType::TRACKER_SWARM_SIZE_THRESHOLD:
                type = "TRACKER_SWARM_SIZE_THRESHOLD";
                break;
            default:
                type = "UNKNOWN";
                break;
        }

        return base + " [TrackerStatisticsEvent: " + type + "]";
    }

private:
    TrackerStatisticsEventType statistics_event_type_; ///< Statistics event type
};

/**
 * @brief Event for tracker statistics update
 */
class TrackerStatsUpdateEvent : public TrackerStatisticsEvent {
public:
    /**
     * @brief Create a new tracker statistics update event
     *
     * @param tracker_url URL of the tracker
     * @param active_announces Number of active announce requests
     * @param active_scrapes Number of active scrape requests
     * @param successful_announces Number of successful announce requests
     * @param successful_scrapes Number of successful scrape requests
     * @param failed_announces Number of failed announce requests
     * @param failed_scrapes Number of failed scrape requests
     * @param average_response_time_ms Average response time in milliseconds
     */
    TrackerStatsUpdateEvent(
        const std::string& tracker_url,
        uint32_t active_announces,
        uint32_t active_scrapes,
        uint32_t successful_announces,
        uint32_t successful_scrapes,
        uint32_t failed_announces,
        uint32_t failed_scrapes,
        double average_response_time_ms
    )
        : TrackerStatisticsEvent(TrackerStatisticsEventType::TRACKER_STATS_UPDATE),
          tracker_url_(tracker_url),
          active_announces_(active_announces),
          active_scrapes_(active_scrapes),
          successful_announces_(successful_announces),
          successful_scrapes_(successful_scrapes),
          failed_announces_(failed_announces),
          failed_scrapes_(failed_scrapes),
          average_response_time_ms_(average_response_time_ms) {}

    /**
     * @brief Get the tracker URL
     *
     * @return Tracker URL
     */
    const std::string& tracker_url() const { return tracker_url_; }

    /**
     * @brief Get the number of active announce requests
     *
     * @return Number of active announce requests
     */
    uint32_t active_announces() const { return active_announces_; }

    /**
     * @brief Get the number of active scrape requests
     *
     * @return Number of active scrape requests
     */
    uint32_t active_scrapes() const { return active_scrapes_; }

    /**
     * @brief Get the number of successful announce requests
     *
     * @return Number of successful announce requests
     */
    uint32_t successful_announces() const { return successful_announces_; }

    /**
     * @brief Get the number of successful scrape requests
     *
     * @return Number of successful scrape requests
     */
    uint32_t successful_scrapes() const { return successful_scrapes_; }

    /**
     * @brief Get the number of failed announce requests
     *
     * @return Number of failed announce requests
     */
    uint32_t failed_announces() const { return failed_announces_; }

    /**
     * @brief Get the number of failed scrape requests
     *
     * @return Number of failed scrape requests
     */
    uint32_t failed_scrapes() const { return failed_scrapes_; }

    /**
     * @brief Get the average response time in milliseconds
     *
     * @return Average response time in milliseconds
     */
    double average_response_time_ms() const { return average_response_time_ms_; }

    /**
     * @brief Clone the event
     *
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<TrackerStatsUpdateEvent>(*this);
    }

    /**
     * @brief Convert the event to a string representation
     *
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = TrackerStatisticsEvent::to_string();
        std::ostringstream oss;
        oss << base << " [Tracker: " << tracker_url_
            << ", Active Announces: " << active_announces_
            << ", Active Scrapes: " << active_scrapes_
            << ", Successful Announces: " << successful_announces_
            << ", Successful Scrapes: " << successful_scrapes_
            << ", Failed Announces: " << failed_announces_
            << ", Failed Scrapes: " << failed_scrapes_
            << ", Avg Response Time: " << average_response_time_ms_ << " ms]";
        return oss.str();
    }

private:
    std::string tracker_url_;              ///< URL of the tracker
    uint32_t active_announces_;            ///< Number of active announce requests
    uint32_t active_scrapes_;              ///< Number of active scrape requests
    uint32_t successful_announces_;        ///< Number of successful announce requests
    uint32_t successful_scrapes_;          ///< Number of successful scrape requests
    uint32_t failed_announces_;            ///< Number of failed announce requests
    uint32_t failed_scrapes_;              ///< Number of failed scrape requests
    double average_response_time_ms_;      ///< Average response time in milliseconds
};

/**
 * @brief Event for tracker swarm size threshold reached
 */
class TrackerSwarmSizeThresholdEvent : public TrackerStatisticsEvent {
public:
    /**
     * @brief Create a new tracker swarm size threshold event
     *
     * @param info_hash InfoHash of the torrent
     * @param tracker_url URL of the tracker
     * @param swarm_size Current swarm size (number of peers)
     * @param threshold Threshold that was reached
     * @param seeders Number of seeders
     * @param leechers Number of leechers
     */
    TrackerSwarmSizeThresholdEvent(
        const types::InfoHash& info_hash,
        const std::string& tracker_url,
        uint32_t swarm_size,
        uint32_t threshold,
        uint32_t seeders,
        uint32_t leechers
    )
        : TrackerStatisticsEvent(TrackerStatisticsEventType::TRACKER_SWARM_SIZE_THRESHOLD),
          info_hash_(info_hash),
          tracker_url_(tracker_url),
          swarm_size_(swarm_size),
          threshold_(threshold),
          seeders_(seeders),
          leechers_(leechers) {}

    /**
     * @brief Get the InfoHash
     *
     * @return InfoHash of the torrent
     */
    const types::InfoHash& info_hash() const { return info_hash_; }

    /**
     * @brief Get the tracker URL
     *
     * @return Tracker URL
     */
    const std::string& tracker_url() const { return tracker_url_; }

    /**
     * @brief Get the swarm size
     *
     * @return Current swarm size (number of peers)
     */
    uint32_t swarm_size() const { return swarm_size_; }

    /**
     * @brief Get the threshold
     *
     * @return Threshold that was reached
     */
    uint32_t threshold() const { return threshold_; }

    /**
     * @brief Get the number of seeders
     *
     * @return Number of seeders
     */
    uint32_t seeders() const { return seeders_; }

    /**
     * @brief Get the number of leechers
     *
     * @return Number of leechers
     */
    uint32_t leechers() const { return leechers_; }

    /**
     * @brief Clone the event
     *
     * @return A new heap-allocated copy of the event
     */
    std::unique_ptr<types::Event> clone() const override {
        return std::make_unique<TrackerSwarmSizeThresholdEvent>(*this);
    }

    /**
     * @brief Convert the event to a string representation
     *
     * @return String representation of the event
     */
    std::string to_string() const override {
        std::string base = TrackerStatisticsEvent::to_string();
        std::ostringstream oss;
        oss << base << " [InfoHash: " << info_hash_.to_hex()
            << ", Tracker: " << tracker_url_
            << ", Swarm Size: " << swarm_size_
            << ", Threshold: " << threshold_
            << ", Seeders: " << seeders_
            << ", Leechers: " << leechers_ << "]";
        return oss.str();
    }

private:
    types::InfoHash info_hash_;    ///< InfoHash of the torrent
    std::string tracker_url_;      ///< URL of the tracker
    uint32_t swarm_size_;          ///< Current swarm size (number of peers)
    uint32_t threshold_;           ///< Threshold that was reached
    uint32_t seeders_;             ///< Number of seeders
    uint32_t leechers_;            ///< Number of leechers
};

} // namespace bitscrape::tracker
