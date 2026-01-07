#include "bitscrape/tracker/tracker_request.hpp"
#include <bitscrape/testing.hpp>

namespace bitscrape::tracker::test {

class TrackerRequestTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test info hash
        std::vector<uint8_t> hash_bytes(20, 0);
        for (size_t i = 0; i < 20; ++i) {
            hash_bytes[i] = static_cast<uint8_t>(i);
        }
        info_hash_ = types::InfoHash(hash_bytes);
        
        // Create a test peer ID
        peer_id_ = "-BS0001-123456789012";
    }
    
    types::InfoHash info_hash_;
    std::string peer_id_;
};

TEST_F(TrackerRequestTest, AnnounceRequestParameters) {
    // Create an announce request
    AnnounceRequest request(
        "http://tracker.example.com/announce",
        info_hash_,
        peer_id_,
        6881,
        0,
        0,
        1000,
        "started"
    );
    
    // Get the parameters
    auto params = request.parameters();
    
    // Check that the required parameters are present
    EXPECT_TRUE(params.find("info_hash") != params.end());
    EXPECT_TRUE(params.find("peer_id") != params.end());
    EXPECT_TRUE(params.find("port") != params.end());
    EXPECT_TRUE(params.find("uploaded") != params.end());
    EXPECT_TRUE(params.find("downloaded") != params.end());
    EXPECT_TRUE(params.find("left") != params.end());
    EXPECT_TRUE(params.find("event") != params.end());
    EXPECT_TRUE(params.find("compact") != params.end());
    
    // Check the parameter values
    EXPECT_EQ(params["port"], "6881");
    EXPECT_EQ(params["uploaded"], "0");
    EXPECT_EQ(params["downloaded"], "0");
    EXPECT_EQ(params["left"], "1000");
    EXPECT_EQ(params["event"], "started");
    EXPECT_EQ(params["compact"], "1");
}

TEST_F(TrackerRequestTest, ScrapeRequestParameters) {
    // Create a scrape request
    ScrapeRequest request(
        "http://tracker.example.com/scrape",
        {info_hash_}
    );
    
    // Get the parameters
    auto params = request.parameters();
    
    // Check that the info_hash parameter is present
    EXPECT_TRUE(params.find("info_hash") != params.end());
}

TEST_F(TrackerRequestTest, BuildUrl) {
    // Create an announce request
    AnnounceRequest request(
        "http://tracker.example.com/announce",
        info_hash_,
        peer_id_,
        6881,
        0,
        0,
        1000
    );
    
    // Build the URL
    std::string url = request.build_url();
    
    // Check that the URL starts with the base URL
    EXPECT_EQ(url.substr(0, 35), "http://tracker.example.com/announce?");
    
    // Check that the URL contains the required parameters
    EXPECT_TRUE(url.find("info_hash=") != std::string::npos);
    EXPECT_TRUE(url.find("peer_id=") != std::string::npos);
    EXPECT_TRUE(url.find("port=6881") != std::string::npos);
    EXPECT_TRUE(url.find("uploaded=0") != std::string::npos);
    EXPECT_TRUE(url.find("downloaded=0") != std::string::npos);
    EXPECT_TRUE(url.find("left=1000") != std::string::npos);
    EXPECT_TRUE(url.find("compact=1") != std::string::npos);
}

} // namespace bitscrape::tracker::test
