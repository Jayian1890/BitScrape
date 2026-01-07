#include <bitscrape/testing.hpp>

#include "bitscrape/beacon/console_sink.hpp"
#include <sstream>

using namespace bitscrape::beacon;
using namespace bitscrape::types;

TEST(ConsoleSinkTest, WriteToStream) {
    // Create a string stream to capture output
    std::stringstream stream;
    
    // Create a console sink that writes to the stream
    ConsoleSink sink(true, stream);
    
    // Write a message
    sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Test message", std::source_location::current());
    
    // Check that the stream contains the message
    std::string output = stream.str();
    EXPECT_NE(output.find("INFO"), std::string::npos);
    EXPECT_NE(output.find("SYSTEM"), std::string::npos);
    EXPECT_NE(output.find("Test message"), std::string::npos);
}

TEST(ConsoleSinkTest, ColorOutput) {
    // Create a string stream to capture output
    std::stringstream stream;
    
    // Create a console sink with colors
    ConsoleSink sink(true, stream);
    
    // Write a message
    sink.write(BeaconSeverity::ERROR, BeaconCategory::SYSTEM, "Error message", std::source_location::current());
    
    // Check that the stream contains color codes
    std::string output = stream.str();
    std::string color_code = get_severity_color(BeaconSeverity::ERROR);
    std::string reset_code = get_reset_color();
    
    EXPECT_NE(output.find(color_code), std::string::npos);
    EXPECT_NE(output.find(reset_code), std::string::npos);
    
    // Create a new stream
    std::stringstream stream2;
    
    // Create a console sink without colors
    ConsoleSink sink2(false, stream2);
    
    // Write a message
    sink2.write(BeaconSeverity::ERROR, BeaconCategory::SYSTEM, "Error message", std::source_location::current());
    
    // Check that the stream does not contain color codes
    std::string output2 = stream2.str();
    EXPECT_EQ(output2.find(color_code), std::string::npos);
    EXPECT_EQ(output2.find(reset_code), std::string::npos);
}

TEST(ConsoleSinkTest, SetUseColors) {
    // Create a string stream to capture output
    std::stringstream stream;
    
    // Create a console sink with colors
    ConsoleSink sink(true, stream);
    
    // Check that colors are enabled
    EXPECT_TRUE(sink.use_colors());
    
    // Disable colors
    sink.set_use_colors(false);
    
    // Check that colors are disabled
    EXPECT_FALSE(sink.use_colors());
    
    // Write a message
    sink.write(BeaconSeverity::ERROR, BeaconCategory::SYSTEM, "Error message", std::source_location::current());
    
    // Check that the stream does not contain color codes
    std::string output = stream.str();
    std::string color_code = get_severity_color(BeaconSeverity::ERROR);
    std::string reset_code = get_reset_color();
    
    EXPECT_EQ(output.find(color_code), std::string::npos);
    EXPECT_EQ(output.find(reset_code), std::string::npos);
}
