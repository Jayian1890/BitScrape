#include <bitscrape/storage/storage_manager.hpp>
#include <bitscrape/types/info_hash.hpp>
#include <bitscrape/types/node_id.hpp>
#include <bitscrape/types/endpoint.hpp>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>

// Global variables for signal handling
std::atomic<bool> running = true;
std::shared_ptr<bitscrape::storage::StorageManager> storage_manager;

// Signal handler
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
        running = false;

        if (storage_manager) {
            storage_manager->close();
        }
    }
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help, -h              Show this help message" << std::endl;
    std::cout << "  --database, -d <file>   Specify database file" << std::endl;
    std::cout << "  --version, -v           Show version information" << std::endl;
}

void print_version() {
    std::cout << "BitScrape Storage CLI version 0.1.0" << std::endl;
    std::cout << "Copyright (c) 2025" << std::endl;
    std::cout << "Licensed under the MIT License" << std::endl;
}

int main(int argc, char *argv[])
{
    // Parse command line arguments
    std::string db_path = "bitscrape.db";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            print_version();
            return 0;
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

    // Create storage manager
    std::cout << "Initializing storage manager with database: " << db_path << std::endl;
    storage_manager = std::make_shared<bitscrape::storage::StorageManager>(db_path);

    // Initialize storage manager
    if (!storage_manager->initialize()) {
        std::cerr << "Failed to initialize storage manager" << std::endl;
        return 1;
    }

    std::cout << "Storage manager initialized successfully" << std::endl;

    // Add some test data
    auto node_id = bitscrape::types::NodeID::random();
    auto endpoint = bitscrape::types::Endpoint(std::string("192.168.1.1"), 6881);

    std::cout << "Storing test node: " << node_id.to_hex() << " at " << endpoint.to_string() << std::endl;
    storage_manager->store_node(node_id, endpoint, true);

    auto info_hash = bitscrape::types::InfoHash::random();
    std::cout << "Storing test infohash: " << info_hash.to_hex() << std::endl;
    storage_manager->store_infohash(info_hash);

    // Main loop
    std::cout << "\nStorage CLI started" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;

    while (running) {
        // Print statistics periodically
        auto stats = storage_manager->get_statistics();

        std::cout << "\nCurrent Statistics:" << std::endl;
        std::cout << "-------------------" << std::endl;
        std::cout << "Nodes: " << stats["node_count"] << std::endl;
        std::cout << "Infohashes: " << stats["infohash_count"] << std::endl;
        std::cout << "Metadata: " << stats["metadata_count"] << std::endl;

        // Sleep for a while
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    // Close storage manager
    std::cout << "Closing storage manager..." << std::endl;
    storage_manager->close();

    return 0;
}
