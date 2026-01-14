#include "bitscrape/web/menubar_controller.hpp"
#include "bitscrape/web/http_server.hpp"
#include "bitscrape/web/web_controller.hpp"

namespace bitscrape::web {

MenubarController::MenubarController() = default;
MenubarController::~MenubarController() {
    stop_server();
}

bool MenubarController::start_server(uint16_t port) {
    if (server_ && server_->is_running()) {
        return true;
    }
    // Create a WebController with default config (empty string) for now
    auto controller = std::make_shared<WebController>("");
    // Initialize and start the controller if needed
    controller->initialize();
    // Create the HTTP server
    server_ = std::make_unique<HTTPServer>(port, controller);
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
