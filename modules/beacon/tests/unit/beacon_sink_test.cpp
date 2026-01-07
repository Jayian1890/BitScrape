#include <bitscrape/testing.hpp>

#include "bitscrape/beacon/beacon_sink.hpp"
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

TEST(BeaconSinkTest, ShouldLog) {
    TestSink sink;
    
    // Default settings should log everything
    EXPECT_TRUE(sink.should_log(BeaconSeverity::DEBUG, BeaconCategory::GENERAL));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::INFO, BeaconCategory::GENERAL));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::GENERAL));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::ERROR, BeaconCategory::GENERAL));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::CRITICAL, BeaconCategory::GENERAL));
    
    // Set minimum severity to WARNING
    sink.set_min_severity(BeaconSeverity::WARNING);
    
    // Should only log WARNING and above
    EXPECT_FALSE(sink.should_log(BeaconSeverity::DEBUG, BeaconCategory::GENERAL));
    EXPECT_FALSE(sink.should_log(BeaconSeverity::INFO, BeaconCategory::GENERAL));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::GENERAL));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::ERROR, BeaconCategory::GENERAL));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::CRITICAL, BeaconCategory::GENERAL));
    
    // Set category filter to SYSTEM and NETWORK
    sink.set_categories({BeaconCategory::SYSTEM, BeaconCategory::NETWORK});
    
    // Should only log SYSTEM and NETWORK categories
    EXPECT_FALSE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::GENERAL));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::SYSTEM));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::NETWORK));
    EXPECT_FALSE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::DHT));
    
    // Clear category filter
    sink.clear_category_filter();
    
    // Should log all categories again
    EXPECT_TRUE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::GENERAL));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::SYSTEM));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::NETWORK));
    EXPECT_TRUE(sink.should_log(BeaconSeverity::WARNING, BeaconCategory::DHT));
}

TEST(BeaconSinkTest, Write) {
    TestSink sink;
    
    // Write a message
    sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Test message", std::source_location::current());
    
    // Should have one entry
    EXPECT_EQ(sink.entries.size(), 1);
    EXPECT_EQ(sink.entries[0].severity, BeaconSeverity::INFO);
    EXPECT_EQ(sink.entries[0].category, BeaconCategory::SYSTEM);
    EXPECT_EQ(sink.entries[0].message, "Test message");
    
    // Set minimum severity to ERROR
    sink.set_min_severity(BeaconSeverity::ERROR);
    
    // Write a message with lower severity
    sink.write(BeaconSeverity::WARNING, BeaconCategory::SYSTEM, "Warning message", std::source_location::current());
    
    // Should still have one entry
    EXPECT_EQ(sink.entries.size(), 1);
    
    // Write a message with higher severity
    sink.write(BeaconSeverity::ERROR, BeaconCategory::SYSTEM, "Error message", std::source_location::current());
    
    // Should have two entries
    EXPECT_EQ(sink.entries.size(), 2);
    EXPECT_EQ(sink.entries[1].severity, BeaconSeverity::ERROR);
    EXPECT_EQ(sink.entries[1].category, BeaconCategory::SYSTEM);
    EXPECT_EQ(sink.entries[1].message, "Error message");
}

TEST(BeaconSinkTest, WriteAsync) {
    TestSink sink;
    
    // Write a message asynchronously
    auto future = sink.write_async(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Async message", std::source_location::current());
    
    // Wait for the future to complete
    future.wait();
    
    // Should have one entry
    EXPECT_EQ(sink.entries.size(), 1);
    EXPECT_EQ(sink.entries[0].severity, BeaconSeverity::INFO);
    EXPECT_EQ(sink.entries[0].category, BeaconCategory::SYSTEM);
    EXPECT_EQ(sink.entries[0].message, "Async message");
}
