#include <gtest/gtest.h>
#include <bitscrape/web/api_handler.hpp>
#include <bitscrape/web/http_router.hpp>
#include <bitscrape/web/web_controller.hpp>

using namespace bitscrape::web;

class APIHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a router
        router = std::make_unique<HTTPRouter>();
        
        // Create a web controller
        web_controller = std::make_shared<WebController>("test_config.conf");
        
        // Register API routes
        APIHandler::register_routes(*router);
    }
    
    std::unique_ptr<HTTPRouter> router;
    std::shared_ptr<WebController> web_controller;
};

TEST_F(APIHandlerTest, StatusEndpoint) {
    // Create a request
    HTTPRequest request;
    request.method = "GET";
    request.path = "/api/status";
    
    // Find the handler
    auto handler = router->find_handler(request);
    EXPECT_NE(handler, nullptr);
    
    // Call the handler
    auto response = handler(request, web_controller);
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.status_message, "OK");
    EXPECT_EQ(response.headers.at("Content-Type"), "application/json");
}

TEST_F(APIHandlerTest, StatisticsEndpoint) {
    // Create a request
    HTTPRequest request;
    request.method = "GET";
    request.path = "/api/statistics";
    
    // Find the handler
    auto handler = router->find_handler(request);
    EXPECT_NE(handler, nullptr);
    
    // Call the handler
    auto response = handler(request, web_controller);
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.status_message, "OK");
    EXPECT_EQ(response.headers.at("Content-Type"), "application/json");
}

TEST_F(APIHandlerTest, StartCrawlingEndpoint) {
    // Create a request
    HTTPRequest request;
    request.method = "POST";
    request.path = "/api/crawling/start";
    
    // Find the handler
    auto handler = router->find_handler(request);
    EXPECT_NE(handler, nullptr);
    
    // Call the handler
    auto response = handler(request, web_controller);
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.status_message, "OK");
    EXPECT_EQ(response.headers.at("Content-Type"), "application/json");
}

TEST_F(APIHandlerTest, StopCrawlingEndpoint) {
    // Create a request
    HTTPRequest request;
    request.method = "POST";
    request.path = "/api/crawling/stop";
    
    // Find the handler
    auto handler = router->find_handler(request);
    EXPECT_NE(handler, nullptr);
    
    // Call the handler
    auto response = handler(request, web_controller);
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.status_message, "OK");
    EXPECT_EQ(response.headers.at("Content-Type"), "application/json");
}
