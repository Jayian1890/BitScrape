#include <gtest/gtest.h>
#include <bitscrape/web/http_server.hpp>
#include <bitscrape/web/web_controller.hpp>

using namespace bitscrape::web;

class HTTPServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a web controller
        web_controller = std::make_shared<WebController>("test_config.conf");
        
        // Create an HTTP server
        http_server = std::make_unique<HTTPServer>(8080, web_controller);
    }
    
    void TearDown() override {
        // Stop the HTTP server
        if (http_server && http_server->is_running()) {
            http_server->stop();
        }
    }
    
    std::shared_ptr<WebController> web_controller;
    std::unique_ptr<HTTPServer> http_server;
};

TEST_F(HTTPServerTest, Constructor) {
    EXPECT_EQ(http_server->port(), 8080);
    EXPECT_FALSE(http_server->is_running());
}

TEST_F(HTTPServerTest, StartStop) {
    // Start the server
    EXPECT_TRUE(http_server->start());
    EXPECT_TRUE(http_server->is_running());
    
    // Stop the server
    EXPECT_TRUE(http_server->stop());
    EXPECT_FALSE(http_server->is_running());
}

TEST_F(HTTPServerTest, Router) {
    // Get the router
    HTTPRouter& router = http_server->router();
    
    // Add a route
    bool route_called = false;
    router.get("/test", [&route_called](const HTTPRequest&, std::shared_ptr<WebController>) {
        route_called = true;
        HTTPResponse response;
        response.status_code = 200;
        response.status_message = "OK";
        response.body = bitscrape::network::Buffer(
            reinterpret_cast<const uint8_t*>("Test"),
            4
        );
        return response;
    });
    
    // Create a request
    HTTPRequest request;
    request.method = "GET";
    request.path = "/test";
    
    // Find the handler
    auto handler = router.find_handler(request);
    EXPECT_NE(handler, nullptr);
    
    // Call the handler
    handler(request, web_controller);
    EXPECT_TRUE(route_called);
}
