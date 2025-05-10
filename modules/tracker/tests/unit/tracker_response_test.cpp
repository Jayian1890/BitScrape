#include "bitscrape/tracker/tracker_response.hpp"
#include "gtest/gtest.h"

namespace bitscrape::tracker::test {

class TrackerResponseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test info hash
        std::vector<uint8_t> hash_bytes(20, 0);
        for (size_t i = 0; i < 20; ++i) {
            hash_bytes[i] = static_cast<uint8_t>(i);
        }
        info_hash_ = types::InfoHash(hash_bytes);
    }

    types::InfoHash info_hash_;
};

TEST_F(TrackerResponseTest, TrackerResponseError) {
    // Create a tracker response
    TrackerResponse response;

    // Check that it doesn't have an error initially
    EXPECT_FALSE(response.has_error());
    EXPECT_TRUE(response.error_message().empty());

    // Set an error message
    response.set_error_message("Test error");

    // Check that it has an error now
    EXPECT_TRUE(response.has_error());
    EXPECT_EQ(response.error_message(), "Test error");
}

TEST_F(TrackerResponseTest, TrackerResponseWarning) {
    // Create a tracker response
    TrackerResponse response;

    // Check that it doesn't have a warning initially
    EXPECT_TRUE(response.warning_message().empty());

    // Set a warning message
    response.set_warning_message("Test warning");

    // Check that it has a warning now
    EXPECT_EQ(response.warning_message(), "Test warning");
}

TEST_F(TrackerResponseTest, AnnounceResponse) {
    // Create an announce response
    AnnounceResponse response;

    // Check the initial values
    EXPECT_EQ(response.interval(), 0);
    EXPECT_EQ(response.min_interval(), 0);
    EXPECT_TRUE(response.tracker_id().empty());
    EXPECT_EQ(response.complete(), 0);
    EXPECT_EQ(response.incomplete(), 0);
    EXPECT_TRUE(response.peers().empty());

    // Set the values
    response.set_interval(1800);
    response.set_min_interval(900);
    response.set_tracker_id("test_tracker");
    response.set_complete(10);
    response.set_incomplete(20);

    // Add some peers
    response.add_peer(network::Address("192.168.1.1", 6881));
    response.add_peer(network::Address("192.168.1.2", 6882));

    // Check the values
    EXPECT_EQ(response.interval(), 1800);
    EXPECT_EQ(response.min_interval(), 900);
    EXPECT_EQ(response.tracker_id(), "test_tracker");
    EXPECT_EQ(response.complete(), 10);
    EXPECT_EQ(response.incomplete(), 20);
    EXPECT_EQ(response.peers().size(), 2);
    EXPECT_EQ(response.peers()[0].to_string(), "192.168.1.1");
    EXPECT_EQ(response.peers()[0].port(), 6881);
    EXPECT_EQ(response.peers()[1].to_string(), "192.168.1.2");
    EXPECT_EQ(response.peers()[1].port(), 6882);

    // Set the peers
    std::vector<network::Address> peers = {
        network::Address("192.168.1.3", 6883),
        network::Address("192.168.1.4", 6884),
        network::Address("192.168.1.5", 6885)
    };
    response.set_peers(peers);

    // Check the peers
    EXPECT_EQ(response.peers().size(), 3);
    EXPECT_EQ(response.peers()[0].to_string(), "192.168.1.3");
    EXPECT_EQ(response.peers()[0].port(), 6883);
    EXPECT_EQ(response.peers()[1].to_string(), "192.168.1.4");
    EXPECT_EQ(response.peers()[1].port(), 6884);
    EXPECT_EQ(response.peers()[2].to_string(), "192.168.1.5");
    EXPECT_EQ(response.peers()[2].port(), 6885);
}

TEST_F(TrackerResponseTest, ScrapeResponse) {
    // Create a scrape response
    ScrapeResponse response;

    // Check the initial values
    EXPECT_TRUE(response.files().empty());

    // Add a file
    ScrapeResponse::ScrapeData data;
    data.complete = 10;
    data.downloaded = 100;
    data.incomplete = 20;
    data.name = "test_torrent";
    response.add_file(info_hash_, data);

    // Check the values
    EXPECT_EQ(response.files().size(), 1);
    EXPECT_EQ(response.files().at(info_hash_).complete, 10);
    EXPECT_EQ(response.files().at(info_hash_).downloaded, 100);
    EXPECT_EQ(response.files().at(info_hash_).incomplete, 20);
    EXPECT_EQ(response.files().at(info_hash_).name, "test_torrent");

    // Set the files
    std::map<types::InfoHash, ScrapeResponse::ScrapeData> files;

    // Create a second info hash
    std::vector<uint8_t> hash_bytes2(20, 0);
    for (size_t i = 0; i < 20; ++i) {
        hash_bytes2[i] = static_cast<uint8_t>(20 - i);
    }
    types::InfoHash info_hash2(hash_bytes2);

    // Add two files
    files[info_hash_] = data;

    ScrapeResponse::ScrapeData data2;
    data2.complete = 5;
    data2.downloaded = 50;
    data2.incomplete = 10;
    data2.name = "test_torrent2";
    files[info_hash2] = data2;

    response.set_files(files);

    // Check the files
    EXPECT_EQ(response.files().size(), 2);
    EXPECT_EQ(response.files().at(info_hash_).complete, 10);
    EXPECT_EQ(response.files().at(info_hash_).downloaded, 100);
    EXPECT_EQ(response.files().at(info_hash_).incomplete, 20);
    EXPECT_EQ(response.files().at(info_hash_).name, "test_torrent");
    EXPECT_EQ(response.files().at(info_hash2).complete, 5);
    EXPECT_EQ(response.files().at(info_hash2).downloaded, 50);
    EXPECT_EQ(response.files().at(info_hash2).incomplete, 10);
    EXPECT_EQ(response.files().at(info_hash2).name, "test_torrent2");
}

} // namespace bitscrape::tracker::test
