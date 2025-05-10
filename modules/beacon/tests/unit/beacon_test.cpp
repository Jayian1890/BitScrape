#include <gtest/gtest.h>

#include "bitscrape/beacon/beacon.hpp"
#include <memory>
#include <algorithm>

using namespace bitscrape::beacon;
using namespace bitscrape::types;

// Test implementation of BeaconSink for testing
class TestSink : public BeaconSink {
public:
    struct LogEntry {
        BeaconSeverity severity;
        BeaconCategory category;
        std::string message;
        std::source_location location;
    };

    void write(BeaconSeverity severity, BeaconCategory category,
              const std::string& message, const std::source_location& location) override {
        if (should_log(severity, category)) {
            entries.push_back({severity, category, message, location});
        }
    }

    std::vector<LogEntry> entries;
};

TEST(BeaconTest, AddSink) {
    Beacon beacon;

    // Create a test sink
    auto sink = std::make_unique<TestSink>();
    auto sink_ptr = sink.get();

    // Add the sink to the beacon
    beacon.add_sink(std::move(sink));

    // Log a message
    beacon.info("Test message", BeaconCategory::SYSTEM);

    // Sink should have received the message
    EXPECT_EQ(sink_ptr->entries.size(), 1);
    EXPECT_EQ(sink_ptr->entries[0].severity, BeaconSeverity::INFO);
    EXPECT_EQ(sink_ptr->entries[0].category, BeaconCategory::SYSTEM);
    EXPECT_EQ(sink_ptr->entries[0].message, "Test message");
}

TEST(BeaconTest, LogLevels) {
    Beacon beacon;

    // Create a test sink
    auto sink = std::make_unique<TestSink>();
    auto sink_ptr = sink.get();

    // Add the sink to the beacon
    beacon.add_sink(std::move(sink));

    // Log messages at different levels
    beacon.debug("Debug message", BeaconCategory::SYSTEM);
    beacon.info("Info message", BeaconCategory::SYSTEM);
    beacon.warning("Warning message", BeaconCategory::SYSTEM);
    beacon.error("Error message", BeaconCategory::SYSTEM);
    beacon.critical("Critical message", BeaconCategory::SYSTEM);

    // Sink should have received all messages
    EXPECT_EQ(sink_ptr->entries.size(), 5);
    EXPECT_EQ(sink_ptr->entries[0].severity, BeaconSeverity::DEBUG);
    EXPECT_EQ(sink_ptr->entries[1].severity, BeaconSeverity::INFO);
    EXPECT_EQ(sink_ptr->entries[2].severity, BeaconSeverity::WARNING);
    EXPECT_EQ(sink_ptr->entries[3].severity, BeaconSeverity::ERROR);
    EXPECT_EQ(sink_ptr->entries[4].severity, BeaconSeverity::CRITICAL);
}

TEST(BeaconTest, AsyncLogging) {
    Beacon beacon;

    // Create a test sink
    auto sink = std::make_unique<TestSink>();
    auto sink_ptr = sink.get();

    // Add the sink to the beacon
    beacon.add_sink(std::move(sink));

    // Log messages asynchronously
    auto future1 = beacon.debug_async("Async debug", BeaconCategory::SYSTEM);
    auto future2 = beacon.info_async("Async info", BeaconCategory::SYSTEM);
    auto future3 = beacon.warning_async("Async warning", BeaconCategory::SYSTEM);
    auto future4 = beacon.error_async("Async error", BeaconCategory::SYSTEM);
    auto future5 = beacon.critical_async("Async critical", BeaconCategory::SYSTEM);

    // Wait for all futures to complete
    future1.wait();
    future2.wait();
    future3.wait();
    future4.wait();
    future5.wait();

    // Sink should have received all messages
    EXPECT_EQ(sink_ptr->entries.size(), 5);

    // Since async operations may complete in any order, we need to check that all severity levels are present
    // without assuming a specific order
    std::vector<BeaconSeverity> expected_severities = {
        BeaconSeverity::DEBUG,
        BeaconSeverity::INFO,
        BeaconSeverity::WARNING,
        BeaconSeverity::ERROR,
        BeaconSeverity::CRITICAL
    };

    std::vector<BeaconSeverity> actual_severities;
    for (const auto& entry : sink_ptr->entries) {
        actual_severities.push_back(entry.severity);
    }

    // Sort both vectors to compare them regardless of order
    std::sort(expected_severities.begin(), expected_severities.end());
    std::sort(actual_severities.begin(), actual_severities.end());

    EXPECT_EQ(actual_severities, expected_severities);
}

TEST(BeaconTest, MultipleSinks) {
    Beacon beacon;

    // Create two test sinks
    auto sink1 = std::make_unique<TestSink>();
    auto sink2 = std::make_unique<TestSink>();
    auto sink1_ptr = sink1.get();
    auto sink2_ptr = sink2.get();

    // Add the sinks to the beacon
    beacon.add_sink(std::move(sink1));
    beacon.add_sink(std::move(sink2));

    // Log a message
    beacon.info("Test message", BeaconCategory::SYSTEM);

    // Both sinks should have received the message
    EXPECT_EQ(sink1_ptr->entries.size(), 1);
    EXPECT_EQ(sink2_ptr->entries.size(), 1);
    EXPECT_EQ(sink1_ptr->entries[0].message, "Test message");
    EXPECT_EQ(sink2_ptr->entries[0].message, "Test message");
}
