#include <gtest/gtest.h>

#include "bitscrape/network/network_bandwidth_event.hpp"
#include <memory>
#include <string>

using namespace bitscrape::network;
using namespace bitscrape::types;

TEST(NetworkBandwidthEventTest, BandwidthUsageEventConstruction) {
    BandwidthUsageEvent event(1000, 2000, 5000);
    
    EXPECT_EQ(event.type(), Event::Type::USER_DEFINED);
    EXPECT_EQ(event.custom_type_id(), static_cast<uint32_t>(BandwidthEventType::BANDWIDTH_USAGE));
    EXPECT_EQ(event.bandwidth_event_type(), BandwidthEventType::BANDWIDTH_USAGE);
    EXPECT_EQ(event.bytes_sent(), 1000);
    EXPECT_EQ(event.bytes_received(), 2000);
    EXPECT_EQ(event.period_ms(), 5000);
    EXPECT_DOUBLE_EQ(event.upload_bandwidth(), 200.0);
    EXPECT_DOUBLE_EQ(event.download_bandwidth(), 400.0);
}

TEST(NetworkBandwidthEventTest, BandwidthLimitReachedEventConstruction) {
    BandwidthLimitReachedEvent event("upload", 1500.0, 1000.0);
    
    EXPECT_EQ(event.type(), Event::Type::USER_DEFINED);
    EXPECT_EQ(event.custom_type_id(), static_cast<uint32_t>(BandwidthEventType::BANDWIDTH_LIMIT_REACHED));
    EXPECT_EQ(event.bandwidth_event_type(), BandwidthEventType::BANDWIDTH_LIMIT_REACHED);
    EXPECT_EQ(event.limit_type(), "upload");
    EXPECT_DOUBLE_EQ(event.current_bandwidth(), 1500.0);
    EXPECT_DOUBLE_EQ(event.limit_bandwidth(), 1000.0);
}

TEST(NetworkBandwidthEventTest, BandwidthUsageEventClone) {
    BandwidthUsageEvent event(1000, 2000, 5000);
    auto clone = event.clone();
    
    EXPECT_NE(clone.get(), &event);
    
    auto* bandwidth_event = dynamic_cast<BandwidthUsageEvent*>(clone.get());
    EXPECT_NE(bandwidth_event, nullptr);
    EXPECT_EQ(bandwidth_event->bandwidth_event_type(), BandwidthEventType::BANDWIDTH_USAGE);
    EXPECT_EQ(bandwidth_event->bytes_sent(), 1000);
    EXPECT_EQ(bandwidth_event->bytes_received(), 2000);
    EXPECT_EQ(bandwidth_event->period_ms(), 5000);
    EXPECT_DOUBLE_EQ(bandwidth_event->upload_bandwidth(), 200.0);
    EXPECT_DOUBLE_EQ(bandwidth_event->download_bandwidth(), 400.0);
}

TEST(NetworkBandwidthEventTest, BandwidthLimitReachedEventClone) {
    BandwidthLimitReachedEvent event("upload", 1500.0, 1000.0);
    auto clone = event.clone();
    
    EXPECT_NE(clone.get(), &event);
    
    auto* bandwidth_event = dynamic_cast<BandwidthLimitReachedEvent*>(clone.get());
    EXPECT_NE(bandwidth_event, nullptr);
    EXPECT_EQ(bandwidth_event->bandwidth_event_type(), BandwidthEventType::BANDWIDTH_LIMIT_REACHED);
    EXPECT_EQ(bandwidth_event->limit_type(), "upload");
    EXPECT_DOUBLE_EQ(bandwidth_event->current_bandwidth(), 1500.0);
    EXPECT_DOUBLE_EQ(bandwidth_event->limit_bandwidth(), 1000.0);
}

TEST(NetworkBandwidthEventTest, BandwidthUsageEventToString) {
    BandwidthUsageEvent event(1000, 2000, 5000);
    std::string str = event.to_string();
    
    // Check that the string contains the event type and data
    EXPECT_NE(str.find("BANDWIDTH_USAGE"), std::string::npos);
    EXPECT_NE(str.find("Sent: 1000"), std::string::npos);
    EXPECT_NE(str.find("Received: 2000"), std::string::npos);
    EXPECT_NE(str.find("Period: 5000"), std::string::npos);
    EXPECT_NE(str.find("Upload: 200"), std::string::npos);
    EXPECT_NE(str.find("Download: 400"), std::string::npos);
}

TEST(NetworkBandwidthEventTest, BandwidthLimitReachedEventToString) {
    BandwidthLimitReachedEvent event("upload", 1500.0, 1000.0);
    std::string str = event.to_string();
    
    // Check that the string contains the event type and data
    EXPECT_NE(str.find("BANDWIDTH_LIMIT_REACHED"), std::string::npos);
    EXPECT_NE(str.find("Type: upload"), std::string::npos);
    EXPECT_NE(str.find("Current: 1500"), std::string::npos);
    EXPECT_NE(str.find("Limit: 1000"), std::string::npos);
}
