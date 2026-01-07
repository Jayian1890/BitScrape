#include <bitscrape/testing.hpp>

#include "bitscrape/beacon/beacon.hpp"
#include <memory>

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
    EXPECT_EQ(sink_ptr->entries.size(), 1UL);
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
    EXPECT_EQ(sink_ptr->entries.size(), 5UL);

    // Find the entry with each severity level
    bool found_debug = false;
    bool found_info = false;
    bool found_warning = false;
    bool found_error = false;
    bool found_critical = false;

    for (const auto& entry : sink_ptr->entries) {
        if (entry.severity == BeaconSeverity::DEBUG) found_debug = true;
        if (entry.severity == BeaconSeverity::INFO) found_info = true;
        if (entry.severity == BeaconSeverity::WARNING) found_warning = true;
        if (entry.severity == BeaconSeverity::ERROR) found_error = true;
        if (entry.severity == BeaconSeverity::CRITICAL) found_critical = true;
    }

    EXPECT_TRUE(found_debug);
    EXPECT_TRUE(found_info);
    EXPECT_TRUE(found_warning);
    EXPECT_TRUE(found_error);
    EXPECT_TRUE(found_critical);
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
    EXPECT_EQ(sink1_ptr->entries.size(), 1UL);
    EXPECT_EQ(sink2_ptr->entries.size(), 1UL);
    EXPECT_EQ(sink1_ptr->entries[0].message, "Test message");
    EXPECT_EQ(sink2_ptr->entries[0].message, "Test message");
}
