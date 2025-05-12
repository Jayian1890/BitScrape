#include <gtest/gtest.h>
#include <bitscrape/web/web_controller.hpp>

using namespace bitscrape::web;

class WebControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a web controller
        web_controller = std::make_unique<WebController>("test_config.conf");
    }
    
    std::unique_ptr<WebController> web_controller;
};

TEST_F(WebControllerTest, Constructor) {
    EXPECT_NE(web_controller->get_controller(), nullptr);
}

TEST_F(WebControllerTest, GetStatistics) {
    // Get statistics
    auto stats = web_controller->get_statistics();
    
    // Check that we got some statistics
    EXPECT_FALSE(stats.empty());
}

TEST_F(WebControllerTest, WebSocketCallbacks) {
    // Register a callback
    bool callback_called = false;
    auto callback_id = web_controller->register_websocket_callback(
        [&callback_called](const std::string&) {
            callback_called = true;
        }
    );
    
    // Unregister the callback
    web_controller->unregister_websocket_callback(callback_id);
}
