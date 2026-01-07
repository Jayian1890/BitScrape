#include <bitscrape/testing.hpp>

#include "bitscrape/types/beacon_types.hpp"
#include <regex>

using namespace bitscrape::types;

TEST(BeaconSeverityTest, ColorCodes) {
    // Test that each severity level has a unique color code
    std::string debug_color = get_severity_color(BeaconSeverity::DEBUG);
    std::string info_color = get_severity_color(BeaconSeverity::INFO);
    std::string warning_color = get_severity_color(BeaconSeverity::WARNING);
    std::string error_color = get_severity_color(BeaconSeverity::ERROR);
    std::string critical_color = get_severity_color(BeaconSeverity::CRITICAL);
    
    EXPECT_NE(debug_color, info_color);
    EXPECT_NE(debug_color, warning_color);
    EXPECT_NE(debug_color, error_color);
    EXPECT_NE(debug_color, critical_color);
    EXPECT_NE(info_color, warning_color);
    EXPECT_NE(info_color, error_color);
    EXPECT_NE(info_color, critical_color);
    EXPECT_NE(warning_color, error_color);
    EXPECT_NE(warning_color, critical_color);
    EXPECT_NE(error_color, critical_color);
    
    // Test that the reset color code is not empty
    std::string reset_color = get_reset_color();
    EXPECT_FALSE(reset_color.empty());
}

TEST(BeaconSeverityTest, ToString) {
    EXPECT_EQ(severity_to_string(BeaconSeverity::DEBUG), "DEBUG");
    EXPECT_EQ(severity_to_string(BeaconSeverity::INFO), "INFO");
    EXPECT_EQ(severity_to_string(BeaconSeverity::WARNING), "WARNING");
    EXPECT_EQ(severity_to_string(BeaconSeverity::ERROR), "ERROR");
    EXPECT_EQ(severity_to_string(BeaconSeverity::CRITICAL), "CRITICAL");
}

TEST(BeaconCategoryTest, ToString) {
    EXPECT_EQ(category_to_string(BeaconCategory::GENERAL), "GENERAL");
    EXPECT_EQ(category_to_string(BeaconCategory::SYSTEM), "SYSTEM");
    EXPECT_EQ(category_to_string(BeaconCategory::NETWORK), "NETWORK");
    EXPECT_EQ(category_to_string(BeaconCategory::DHT), "DHT");
    EXPECT_EQ(category_to_string(BeaconCategory::BITTORRENT), "BITTORRENT");
    EXPECT_EQ(category_to_string(BeaconCategory::TRACKER), "TRACKER");
    EXPECT_EQ(category_to_string(BeaconCategory::DATABASE), "DATABASE");
    EXPECT_EQ(category_to_string(BeaconCategory::UI), "UI");
}

TEST(BeaconEventTest, Construction) {
    BeaconEvent event(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Test message");
    
    EXPECT_EQ(event.severity(), BeaconSeverity::INFO);
    EXPECT_EQ(event.category(), BeaconCategory::SYSTEM);
    EXPECT_EQ(event.message(), "Test message");
    EXPECT_EQ(event.type(), Event::Type::USER_DEFINED);
    EXPECT_EQ(event.custom_type_id(), 1001);
}

TEST(BeaconEventTest, Clone) {
    BeaconEvent event(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Test message");
    
    auto clone = event.clone();
    
    EXPECT_EQ(clone->type(), event.type());
    EXPECT_EQ(clone->custom_type_id(), event.custom_type_id());
    
    // Cast to BeaconEvent to check the specific properties
    auto beacon_clone = dynamic_cast<BeaconEvent*>(clone.get());
    EXPECT_NE(beacon_clone, nullptr);
    EXPECT_EQ(beacon_clone->severity(), event.severity());
    EXPECT_EQ(beacon_clone->category(), event.category());
    EXPECT_EQ(beacon_clone->message(), event.message());
}

TEST(BeaconEventTest, ToString) {
    BeaconEvent event(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Test message");
    
    std::string str = event.to_string();
    
    // Check that the string contains the event type, severity, category, and message
    EXPECT_NE(str.find("USER_DEFINED"), std::string::npos);
    EXPECT_NE(str.find("INFO"), std::string::npos);
    EXPECT_NE(str.find("SYSTEM"), std::string::npos);
    EXPECT_NE(str.find("Test message"), std::string::npos);
    
    // Check that the string contains a timestamp in the format YYYY-MM-DD HH:MM:SS.mmm
    std::regex timestamp_regex("\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d{3}");
    EXPECT_TRUE(std::regex_search(str, timestamp_regex));
    
    // Check that the string contains source location information
    EXPECT_NE(str.find("beacon_types_test.cpp"), std::string::npos);
}
