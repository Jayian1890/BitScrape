#include <bitscrape/testing.hpp>

#include "bitscrape/tracker/tracker_statistics_event.hpp"
#include "bitscrape/types/info_hash.hpp"
#include <memory>
#include <string>

using namespace bitscrape::tracker;
using namespace bitscrape::types;

TEST(TrackerStatisticsEventTest, TrackerStatsUpdateEventConstruction) {
    TrackerStatsUpdateEvent event(
        "http://tracker.example.com:6969/announce",
        10, 5, 100, 50, 20, 10, 250.5
    );
    
    EXPECT_EQ(event.type(), Event::Type::USER_DEFINED);
    EXPECT_EQ(event.custom_type_id(), static_cast<uint32_t>(TrackerStatisticsEventType::TRACKER_STATS_UPDATE));
    EXPECT_EQ(event.statistics_event_type(), TrackerStatisticsEventType::TRACKER_STATS_UPDATE);
    EXPECT_EQ(event.tracker_url(), "http://tracker.example.com:6969/announce");
    EXPECT_EQ(event.active_announces(), 10);
    EXPECT_EQ(event.active_scrapes(), 5);
    EXPECT_EQ(event.successful_announces(), 100);
    EXPECT_EQ(event.successful_scrapes(), 50);
    EXPECT_EQ(event.failed_announces(), 20);
    EXPECT_EQ(event.failed_scrapes(), 10);
    EXPECT_DOUBLE_EQ(event.average_response_time_ms(), 250.5);
}

TEST(TrackerStatisticsEventTest, TrackerSwarmSizeThresholdEventConstruction) {
    InfoHash info_hash = InfoHash::random();
    TrackerSwarmSizeThresholdEvent event(
        info_hash,
        "http://tracker.example.com:6969/announce",
        1000, 500, 800, 200
    );
    
    EXPECT_EQ(event.type(), Event::Type::USER_DEFINED);
    EXPECT_EQ(event.custom_type_id(), static_cast<uint32_t>(TrackerStatisticsEventType::TRACKER_SWARM_SIZE_THRESHOLD));
    EXPECT_EQ(event.statistics_event_type(), TrackerStatisticsEventType::TRACKER_SWARM_SIZE_THRESHOLD);
    EXPECT_EQ(event.info_hash(), info_hash);
    EXPECT_EQ(event.tracker_url(), "http://tracker.example.com:6969/announce");
    EXPECT_EQ(event.swarm_size(), 1000);
    EXPECT_EQ(event.threshold(), 500);
    EXPECT_EQ(event.seeders(), 800);
    EXPECT_EQ(event.leechers(), 200);
}

TEST(TrackerStatisticsEventTest, TrackerStatsUpdateEventClone) {
    TrackerStatsUpdateEvent event(
        "http://tracker.example.com:6969/announce",
        10, 5, 100, 50, 20, 10, 250.5
    );
    auto clone = event.clone();
    
    EXPECT_NE(clone.get(), &event);
    
    auto* stats_event = dynamic_cast<TrackerStatsUpdateEvent*>(clone.get());
    EXPECT_NE(stats_event, nullptr);
    EXPECT_EQ(stats_event->statistics_event_type(), TrackerStatisticsEventType::TRACKER_STATS_UPDATE);
    EXPECT_EQ(stats_event->tracker_url(), "http://tracker.example.com:6969/announce");
    EXPECT_EQ(stats_event->active_announces(), 10);
    EXPECT_EQ(stats_event->active_scrapes(), 5);
    EXPECT_EQ(stats_event->successful_announces(), 100);
    EXPECT_EQ(stats_event->successful_scrapes(), 50);
    EXPECT_EQ(stats_event->failed_announces(), 20);
    EXPECT_EQ(stats_event->failed_scrapes(), 10);
    EXPECT_DOUBLE_EQ(stats_event->average_response_time_ms(), 250.5);
}

TEST(TrackerStatisticsEventTest, TrackerSwarmSizeThresholdEventClone) {
    InfoHash info_hash = InfoHash::random();
    TrackerSwarmSizeThresholdEvent event(
        info_hash,
        "http://tracker.example.com:6969/announce",
        1000, 500, 800, 200
    );
    auto clone = event.clone();
    
    EXPECT_NE(clone.get(), &event);
    
    auto* stats_event = dynamic_cast<TrackerSwarmSizeThresholdEvent*>(clone.get());
    EXPECT_NE(stats_event, nullptr);
    EXPECT_EQ(stats_event->statistics_event_type(), TrackerStatisticsEventType::TRACKER_SWARM_SIZE_THRESHOLD);
    EXPECT_EQ(stats_event->info_hash(), info_hash);
    EXPECT_EQ(stats_event->tracker_url(), "http://tracker.example.com:6969/announce");
    EXPECT_EQ(stats_event->swarm_size(), 1000);
    EXPECT_EQ(stats_event->threshold(), 500);
    EXPECT_EQ(stats_event->seeders(), 800);
    EXPECT_EQ(stats_event->leechers(), 200);
}

TEST(TrackerStatisticsEventTest, TrackerStatsUpdateEventToString) {
    TrackerStatsUpdateEvent event(
        "http://tracker.example.com:6969/announce",
        10, 5, 100, 50, 20, 10, 250.5
    );
    std::string str = event.to_string();
    
    // Check that the string contains the event type and data
    EXPECT_NE(str.find("TRACKER_STATS_UPDATE"), std::string::npos);
    EXPECT_NE(str.find("Tracker: http://tracker.example.com:6969/announce"), std::string::npos);
    EXPECT_NE(str.find("Active Announces: 10"), std::string::npos);
    EXPECT_NE(str.find("Active Scrapes: 5"), std::string::npos);
    EXPECT_NE(str.find("Successful Announces: 100"), std::string::npos);
    EXPECT_NE(str.find("Successful Scrapes: 50"), std::string::npos);
    EXPECT_NE(str.find("Failed Announces: 20"), std::string::npos);
    EXPECT_NE(str.find("Failed Scrapes: 10"), std::string::npos);
    EXPECT_NE(str.find("Avg Response Time: 250.5"), std::string::npos);
}

TEST(TrackerStatisticsEventTest, TrackerSwarmSizeThresholdEventToString) {
    InfoHash info_hash = InfoHash::random();
    TrackerSwarmSizeThresholdEvent event(
        info_hash,
        "http://tracker.example.com:6969/announce",
        1000, 500, 800, 200
    );
    std::string str = event.to_string();
    
    // Check that the string contains the event type and data
    EXPECT_NE(str.find("TRACKER_SWARM_SIZE_THRESHOLD"), std::string::npos);
    EXPECT_NE(str.find("InfoHash: " + info_hash.to_string()), std::string::npos);
    EXPECT_NE(str.find("Tracker: http://tracker.example.com:6969/announce"), std::string::npos);
    EXPECT_NE(str.find("Swarm Size: 1000"), std::string::npos);
    EXPECT_NE(str.find("Threshold: 500"), std::string::npos);
    EXPECT_NE(str.find("Seeders: 800"), std::string::npos);
    EXPECT_NE(str.find("Leechers: 200"), std::string::npos);
}
