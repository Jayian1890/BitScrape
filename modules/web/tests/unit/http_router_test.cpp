#include <bitscrape/testing.hpp>
#include <bitscrape/web/http_router.hpp>
#include <bitscrape/web/http_server.hpp>
#include <bitscrape/web/web_controller.hpp>

using namespace bitscrape::web;

class HTTPRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a router
        router = std::make_unique<HTTPRouter>();
        
        // Create a web controller
        web_controller = std::make_shared<WebController>("test_config.conf");
    }
    
    std::unique_ptr<HTTPRouter> router;
    std::shared_ptr<WebController> web_controller;
};

TEST_F(HTTPRouterTest, AddRoute) {
    // Add a route
    router->add_route("GET", "/test", [](const HTTPRequest&, std::shared_ptr<WebController>) {
        HTTPResponse response;
        response.status_code = 200;
        response.status_message = "OK";
        return response;
    });
    
    // Create a request
    HTTPRequest request;
    request.method = "GET";
    request.path = "/test";
    
    // Find the handler
    auto handler = router->find_handler(request);
    EXPECT_NE(handler, nullptr);
    
    // Call the handler
    auto response = handler(request, web_controller);
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.status_message, "OK");
}

TEST_F(HTTPRouterTest, AddRouteWithParameters) {
    // Add a route with parameters
    router->add_route("GET", "/users/:id", [](const HTTPRequest& request, std::shared_ptr<WebController>) {
        HTTPResponse response;
        response.status_code = 200;
        response.status_message = "OK";
        response.body = bitscrape::network::Buffer(
            reinterpret_cast<const uint8_t*>(request.path_params.at("id").c_str()),
            request.path_params.at("id").length()
        );
        return response;
    });
    
    // Create a request
    HTTPRequest request;
    request.method = "GET";
    request.path = "/users/123";
    
    // Find the handler
    auto handler = router->find_handler(request);
    EXPECT_NE(handler, nullptr);
    
    // Call the handler
    auto response = handler(request, web_controller);
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.status_message, "OK");
    
    // Check that the parameter was extracted
    EXPECT_EQ(request.path_params.at("id"), "123");
}

TEST_F(HTTPRouterTest, RouteNotFound) {
    // Add a route
    router->add_route("GET", "/test", [](const HTTPRequest&, std::shared_ptr<WebController>) {
        HTTPResponse response;
        response.status_code = 200;
        response.status_message = "OK";
        return response;
    });
    
    // Create a request for a different path
    HTTPRequest request;
    request.method = "GET";
    request.path = "/not-found";
    
    // Find the handler
    auto handler = router->find_handler(request);
    EXPECT_EQ(handler, nullptr);
}

TEST_F(HTTPRouterTest, MethodNotAllowed) {
    // Add a route for GET
    router->add_route("GET", "/test", [](const HTTPRequest&, std::shared_ptr<WebController>) {
        HTTPResponse response;
        response.status_code = 200;
        response.status_message = "OK";
        return response;
    });
    
    // Create a request for POST
    HTTPRequest request;
    request.method = "POST";
    request.path = "/test";
    
    // Find the handler
    auto handler = router->find_handler(request);
    EXPECT_EQ(handler, nullptr);
}
