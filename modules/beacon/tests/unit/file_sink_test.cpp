#include <bitscrape/testing.hpp>

#include "bitscrape/beacon/file_sink.hpp"
#include <filesystem>
#include <fstream>
#include <string>

using namespace bitscrape::beacon;
using namespace bitscrape::types;

class FileSinkTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary file name
        temp_file_ = "test_beacon.log";
        
        // Remove the file if it exists
        std::filesystem::remove(temp_file_);
    }
    
    void TearDown() override {
        // Remove the file
        std::filesystem::remove(temp_file_);
    }
    
    std::string temp_file_;
};

TEST_F(FileSinkTest, WriteToFile) {
    // Create a file sink
    FileSink sink(temp_file_);
    
    // Write a message
    sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Test message", std::source_location::current());
    
    // Check that the file exists
    EXPECT_TRUE(std::filesystem::exists(temp_file_));
    
    // Read the file content
    std::ifstream file(temp_file_);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    // Check that the content contains the message
    EXPECT_NE(content.find("INFO"), std::string::npos);
    EXPECT_NE(content.find("SYSTEM"), std::string::npos);
    EXPECT_NE(content.find("Test message"), std::string::npos);
}

TEST_F(FileSinkTest, Append) {
    // Create a file sink with append mode
    {
        FileSink sink(temp_file_);
        sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "First message", std::source_location::current());
    }
    
    // Create another file sink with append mode
    {
        FileSink sink(temp_file_, true);
        sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Second message", std::source_location::current());
    }
    
    // Read the file content
    std::ifstream file(temp_file_);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    // Check that the content contains both messages
    EXPECT_NE(content.find("First message"), std::string::npos);
    EXPECT_NE(content.find("Second message"), std::string::npos);
}

TEST_F(FileSinkTest, Overwrite) {
    // Create a file sink
    {
        FileSink sink(temp_file_);
        sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "First message", std::source_location::current());
    }
    
    // Create another file sink with overwrite mode
    {
        FileSink sink(temp_file_, false);
        sink.write(BeaconSeverity::INFO, BeaconCategory::SYSTEM, "Second message", std::source_location::current());
    }
    
    // Read the file content
    std::ifstream file(temp_file_);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    // Check that the content contains only the second message
    EXPECT_EQ(content.find("First message"), std::string::npos);
    EXPECT_NE(content.find("Second message"), std::string::npos);
}
