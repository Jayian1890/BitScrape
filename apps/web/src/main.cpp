#include <bitscrape/web/http_server.hpp>
#include <bitscrape/web/web_controller.hpp>
#include <bitscrape/web/api_handler.hpp>
#include <bitscrape/web/static_file_handler.hpp>
#include <bitscrape/types/beacon_types.hpp>

#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include <filesystem>

// Global variables for signal handling
std::atomic<bool> running = true;
std::shared_ptr<bitscrape::web::HTTPServer> http_server;

// Signal handler
void signal_handler(int signal) {
    std::cout << "Received signal " << signal << std::endl;
    running = false;
    if (http_server) {
        http_server->stop();
    }
}

void print_usage() {
    std::cout << "Usage: bitscrape-web [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --port=PORT                 Port to listen on (default: 8080)" << std::endl;
    std::cout << "  --config-path=PATH          Path to configuration file (default: bitscrape.conf)" << std::endl;
    std::cout << "  --static-dir=PATH           Path to static files (default: public)" << std::endl;
    std::cout << "  --help                      Show this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    // Default values
    uint16_t port = 8080;
    std::string config_path = "bitscrape.conf";
    std::string static_dir = "public";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            print_usage();
            return 0;
        } else if (arg.find("--port=") == 0) {
            port = std::stoi(arg.substr(7));
        } else if (arg.find("--config-path=") == 0) {
            config_path = arg.substr(14);
        } else if (arg.find("--static-dir=") == 0) {
            static_dir = arg.substr(13);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage();
            return 1;
        }
    }

    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Create web controller
    auto web_controller = std::make_shared<bitscrape::web::WebController>(config_path);

    // Initialize web controller
    std::cout << "Initializing BitScrape..." << std::endl;
    if (!web_controller->initialize()) {
        std::cerr << "Failed to initialize BitScrape" << std::endl;
        return 1;
    }

    // Start web controller
    std::cout << "Starting BitScrape..." << std::endl;
    if (!web_controller->start()) {
        std::cerr << "Failed to start BitScrape" << std::endl;
        return 1;
    }

    // Create HTTP server
    http_server = std::make_shared<bitscrape::web::HTTPServer>(port, web_controller);

    // Register API routes
    bitscrape::web::APIHandler::register_routes(http_server->router());

    // Resolve static directory path
    std::filesystem::path static_path(static_dir);
    if (!static_path.is_absolute()) {
        // If relative, make it relative to the executable directory
        static_path = std::filesystem::current_path() / static_path;
    }

    // Register static file routes
    bitscrape::web::StaticFileHandler::register_routes(http_server->router(), static_path.string());

    // Start HTTP server
    std::cout << "Starting HTTP server on port " << port << "..." << std::endl;
    if (!http_server->start()) {
        std::cerr << "Failed to start HTTP server" << std::endl;
        web_controller->stop();
        return 1;
    }

    std::cout << "BitScrape web interface started" << std::endl;
    std::cout << "Web interface available at http://localhost:" << port << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;

    // Main loop
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Stop HTTP server
    std::cout << "Stopping HTTP server..." << std::endl;
    http_server->stop();

    // Stop web controller
    std::cout << "Stopping BitScrape..." << std::endl;
    web_controller->stop();

    std::cout << "BitScrape web interface stopped" << std::endl;
    return 0;
}
