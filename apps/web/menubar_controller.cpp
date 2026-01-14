#include "bitscrape/web/menubar_controller.hpp"
#include "bitscrape/web/http_server.hpp"
#include "bitscrape/web/web_controller.hpp"
#include "bitscrape/web/static_file_handler.hpp"
#include "bitscrape/web/api_handler.hpp"

namespace bitscrape::web {

MenubarController::MenubarController() = default;
MenubarController::~MenubarController() {
    stop_server();
}

bool MenubarController::start_server(uint16_t port, const std::string& resource_path) {
    if (server_ && server_->is_running()) {
        return true;
    }
    // Create a WebController with default config (empty string) for now
    auto controller = std::make_shared<WebController>("");
    
    // Get the underlying core controller to initialize it properly
    auto core_controller = controller->get_controller();
    
    // Initialize and start the core controller
    if (!core_controller->initialize()) {
        return false;
    }
    
    if (!core_controller->start()) {
        return false;
    }

    // Start crawling automatically
    core_controller->start_crawling();

    // Create the HTTP server
    server_ = std::make_unique<HTTPServer>(port, controller);
    
    // Register API routes
    APIHandler::register_routes(server_->router());

    // Register static file routes
    std::string public_dir = resource_path + "/public";
    StaticFileHandler::register_routes(server_->router(), public_dir);
    
    return server_->start();
}

void MenubarController::stop_server() {
    if (server_) {
        server_->stop();
        server_.reset();
    }
}

bool MenubarController::is_running() const {
    return server_ && server_->is_running();
}

} // namespace bitscrape::web
