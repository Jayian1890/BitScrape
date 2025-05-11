#include <bitscrape/types/beacon_types.hpp>
#include <bitscrape/core/controller.hpp>
#include <bitscrape/core/configuration.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

// Global variables for signal handling
std::atomic running = true;
std::shared_ptr<bitscrape::core::Controller> controller;

// Signal handler
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
        running = false;

        if (controller) {
            controller->stop();
        }
    }
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help, -h              Show this help message" << std::endl;
    std::cout << "  --config, -c <file>     Specify configuration file" << std::endl;
    std::cout << "  --database, -d <file>   Specify database file" << std::endl;
    std::cout << "  --crawl, -C             Start crawling immediately" << std::endl;
    std::cout << "  --version, -v           Show version information" << std::endl;
}

void print_version() {
    std::cout << "BitScrape CLI version 0.1.0" << std::endl;
    std::cout << "Copyright (c) 2025" << std::endl;
    std::cout << "Licensed under the MIT License" << std::endl;
}

int main(int argc, char *argv[])
{
    // Use enum for cleaner code
    using enum bitscrape::types::BeaconCategory;
    using enum bitscrape::types::BeaconSeverity;

    // Parse command line arguments
    std::string config_path = "bitscrape.conf";
    std::string db_path;
    bool start_crawling = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            print_version();
            return 0;
        } else if (arg == "--crawl" || arg == "-C") {
            start_crawling = true;
        } else if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            config_path = argv[++i];
        } else if ((arg == "--database" || arg == "-d") && i + 1 < argc) {
            db_path = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Create controller
    controller = std::make_shared<bitscrape::core::Controller>(config_path);

    // Get beacon for logging
    auto beacon = controller->get_beacon();

    // Initialize controller
    beacon->info("Initializing BitScrape...", SYSTEM);
    if (!controller->initialize()) {
        beacon->critical("Failed to initialize BitScrape", SYSTEM);
        return 1;
    }

    // Set database path if specified
    if (!db_path.empty()) {
        controller->get_configuration()->set_string("database.path", db_path);
        controller->get_configuration()->save();
    }

    // Start controller
    beacon->info("Starting BitScrape...", SYSTEM);
    if (!controller->start()) {
        beacon->critical("Failed to start BitScrape", SYSTEM);
        return 1;
    }

    // Start crawling if requested
    if (start_crawling) {
        beacon->info("Starting DHT crawling...", SYSTEM);
        if (!controller->start_crawling()) {
            beacon->error("Failed to start crawling", SYSTEM);
        }
    }

    // Main loop
    beacon->info("BitScrape CLI started", SYSTEM);
    beacon->info("Version: 0.1.0", SYSTEM);
    beacon->info("Press Ctrl+C to exit", SYSTEM);

    while (running) {
        // Print statistics periodically
        auto stats = controller->get_statistics();

        std::cout << "\nCurrent Statistics:" << std::endl;
        std::cout << "-------------------" << std::endl;

        // Controller statistics
        std::cout << "Controller running: " << stats["controller.running"] << std::endl;
        std::cout << "Crawling active: " << stats["controller.crawling"] << std::endl;

        // Storage statistics
        if (stats.contains("storage.node_count")) {
            std::cout << "Nodes discovered: " << stats["storage.node_count"] << std::endl;
        }

        if (stats.contains("storage.infohash_count")) {
            std::cout << "Infohashes discovered: " << stats["storage.infohash_count"] << std::endl;
        }

        if (stats.contains("storage.metadata_count")) {
            std::cout << "Metadata downloaded: " << stats["storage.metadata_count"] << std::endl;
        }

        // Sleep for a while
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    // Stop controller if not already stopped by signal handler
    if (running) {
        beacon->info("BitScrape CLI shutting down", SYSTEM);
        controller->stop();
    }

    return 0;
}
