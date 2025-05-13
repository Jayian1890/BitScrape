#include <bitscrape/types/beacon_types.hpp>
#include <bitscrape/core/controller.hpp>
#include <bitscrape/core/configuration.hpp>
#include <bitscrape/storage/query_interface.hpp>
#include <bitscrape/storage/data_models.hpp>
#include <bitscrape/web/http_server.hpp>
#include <bitscrape/web/web_controller.hpp>
#include <bitscrape/web/api_handler.hpp>
#include <bitscrape/web/static_file_handler.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <mutex>
#include <filesystem>

// Global variables for signal handling
std::atomic running = true;
std::shared_ptr<bitscrape::core::Controller> controller;

// Global variables for CLI state
std::atomic<bool> interactive_mode = true;
std::mutex console_mutex;

// Global variables for web interface
std::shared_ptr<bitscrape::web::HTTPServer> http_server;
std::shared_ptr<bitscrape::web::WebController> web_controller;

// Signal handler
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
        running = false;

        if (controller) {
            controller->stop();
        }

        if (http_server) {
            http_server->stop();
        }
    }
}

// Utility functions for formatting and display
std::string format_time_point(const std::chrono::system_clock::time_point& tp) {
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string format_size(uint64_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size_d = static_cast<double>(size);

    while (size_d >= 1024.0 && unit_index < 4) {
        size_d /= 1024.0;
        unit_index++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size_d << " " << units[unit_index];
    return ss.str();
}

std::string format_duration(std::chrono::seconds duration) {
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    auto seconds = duration;

    std::stringstream ss;
    if (hours.count() > 0) {
        ss << hours.count() << "h ";
    }
    if (minutes.count() > 0 || hours.count() > 0) {
        ss << minutes.count() << "m ";
    }
    ss << seconds.count() << "s";
    return ss.str();
}

void print_horizontal_line(int width = 80) {
    std::cout << std::string(width, '-') << std::endl;
}

void print_centered(const std::string& text, int width = 80) {
    int padding = (width - text.length()) / 2;
    if (padding > 0) {
        std::cout << std::string(padding, ' ');
    }
    std::cout << text << std::endl;
}

void print_table_row(const std::vector<std::string>& columns, const std::vector<int>& widths) {
    for (size_t i = 0; i < columns.size(); ++i) {
        std::cout << std::left << std::setw(widths[i]) << columns[i];
    }
    std::cout << std::endl;
}

void print_table_header(const std::vector<std::string>& headers, const std::vector<int>& widths) {
    print_table_row(headers, widths);

    std::vector<std::string> separator_row;
    for (auto width : widths) {
        separator_row.push_back(std::string(width - 1, '-'));
    }
    print_table_row(separator_row, widths);
}

// Convert string to lowercase
std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return str;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help, -h              Show this help message" << std::endl;
    std::cout << "  --config, -c <file>     Specify configuration file" << std::endl;
    std::cout << "  --database, -d <file>   Specify database file" << std::endl;
    std::cout << "  --crawl, -C             Start crawling immediately" << std::endl;
    std::cout << "  --interactive, -i       Start in interactive mode" << std::endl;
    std::cout << "  --version, -v           Show version information" << std::endl;
    std::cout << "  --no-web                Disable web interface" << std::endl;
    std::cout << "  --port=PORT             Web interface port (default: 8080)" << std::endl;
    std::cout << "  --static-dir=PATH       Path to static files (default: public)" << std::endl;
}

void print_version() {
    std::cout << "BitScrape CLI version 0.1.0" << std::endl;
    std::cout << "Copyright (c) 2025" << std::endl;
    std::cout << "Licensed under the MIT License" << std::endl;
}

// Function to start the web interface
bool start_web_interface(const std::string& config_path, uint16_t web_port,
                         const std::string& static_dir,
                         const std::shared_ptr<bitscrape::beacon::Beacon>& beacon) {
    using enum bitscrape::types::BeaconCategory;

    // Create a web controller
    web_controller = std::make_shared<bitscrape::web::WebController>(config_path);

    // Initialize web controller
    if (!web_controller->initialize()) {
        beacon->error("Failed to initialize web controller", SYSTEM);
        return false;
    }

    // Start web controller
    if (!web_controller->start()) {
        beacon->error("Failed to start web controller", SYSTEM);
        return false;
    }

    // Create HTTP server
    http_server = std::make_shared<bitscrape::web::HTTPServer>(web_port, web_controller);

    // Register API routes
    bitscrape::web::APIHandler::register_routes(http_server->router());

    // Resolve static directory path
    std::filesystem::path static_path(static_dir);
    if (!static_path.is_absolute()) {
        // If relative, make it relative to the executable directory
        static_path = std::filesystem::current_path() / static_path;
    }

    // Register static file routes
    bitscrape::web::StaticFileHandler::register_routes(
        http_server->router(), static_path.string());

    // Start HTTP server
    if (!http_server->start()) {
        beacon->error("Failed to start HTTP server", SYSTEM);
        return false;
    }

    beacon->info("Web interface available at http://localhost:" + std::to_string(web_port), SYSTEM);
    return true;
}

// Functions for displaying detailed information
void display_node_details(const bitscrape::storage::NodeModel& node) {
    print_horizontal_line();
    print_centered("Node Details", 80);
    print_horizontal_line();

    std::cout << "Node ID:       " << node.node_id.to_hex() << std::endl;
    std::cout << "IP Address:    " << node.endpoint.address() << std::endl;
    std::cout << "Port:          " << node.endpoint.port() << std::endl;
    std::cout << "First Seen:    " << format_time_point(node.first_seen) << std::endl;
    std::cout << "Last Seen:     " << format_time_point(node.last_seen) << std::endl;
    std::cout << "Ping Count:    " << node.ping_count << std::endl;
    std::cout << "Query Count:   " << node.query_count << std::endl;
    std::cout << "Response Count:" << node.response_count << std::endl;
    std::cout << "Is Responsive: " << (node.is_responsive ? "Yes" : "No") << std::endl;

    print_horizontal_line();
}

void display_infohash_details(const bitscrape::storage::InfoHashModel& infohash,
                              const std::shared_ptr<bitscrape::storage::QueryInterface>& query) {
    print_horizontal_line();
    print_centered("InfoHash Details", 80);
    print_horizontal_line();

    std::cout << "InfoHash:       " << infohash.info_hash.to_hex() << std::endl;
    std::cout << "First Seen:      " << format_time_point(infohash.first_seen) << std::endl;
    std::cout << "Last Seen:       " << format_time_point(infohash.last_seen) << std::endl;
    std::cout << "Announce Count:  " << infohash.announce_count << std::endl;
    std::cout << "Peer Count:      " << infohash.peer_count << std::endl;
    std::cout << "Has Metadata:    " << (infohash.has_metadata ? "Yes" : "No") << std::endl;

    // If metadata is available, display it
    if (infohash.has_metadata) {
        auto metadata = query->get_metadata(infohash.info_hash);
        if (metadata) {
            std::cout << "\nMetadata Information:" << std::endl;
            std::cout << "Name:            " << metadata->name << std::endl;
            std::cout << "Total Size:      " << format_size(metadata->total_size) << std::endl;
            std::cout << "Piece Count:     " << metadata->piece_count << std::endl;
            std::cout << "File Count:      " << metadata->file_count << std::endl;

            if (!metadata->comment.empty()) {
                std::cout << "Comment:         " << metadata->comment << std::endl;
            }

            if (!metadata->created_by.empty()) {
                std::cout << "Created By:      " << metadata->created_by << std::endl;
            }

            if (metadata->creation_date) {
                std::cout << "Creation Date:   " << format_time_point(*metadata->creation_date) <<
                    std::endl;
            }

            // Display files if available
            auto files = query->get_files(infohash.info_hash);
            if (!files.empty()) {
                std::cout << "\nFiles:" << std::endl;
                print_horizontal_line();

                std::vector<int> widths = {60, 20};
                print_table_header({"Path", "Size"}, widths);

                for (const auto& file : files) {
                    print_table_row({file.path, format_size(file.size)}, widths);
                }
            }

            // Display trackers if available
            auto trackers = query->get_trackers(infohash.info_hash);
            if (!trackers.empty()) {
                std::cout << "\nTrackers:" << std::endl;
                print_horizontal_line();

                std::vector<int> widths = {60, 20};
                print_table_header({"URL", "Last Seen"}, widths);

                for (const auto& tracker : trackers) {
                    print_table_row({tracker.url, format_time_point(tracker.last_seen)}, widths);
                }
            }

            // Display peers if available
            auto peers = query->get_peers(infohash.info_hash);
            if (!peers.empty()) {
                std::cout << "\nPeers:" << std::endl;
                print_horizontal_line();

                std::vector<int> widths = {20, 10, 20, 30};
                print_table_header({"IP Address", "Port", "Last Seen", "Peer ID"}, widths);

                for (const auto& peer : peers) {
                    std::string peer_id_str = peer.peer_id ? peer.peer_id->to_hex() : "N/A";
                    print_table_row({peer.endpoint.address(),
                                     std::to_string(peer.endpoint.port()),
                                     format_time_point(peer.last_seen),
                                     peer_id_str}, widths);
                }
            }
        }
    }

    print_horizontal_line();
}

void display_metadata_details(const bitscrape::storage::MetadataModel& metadata,
                              const std::shared_ptr<bitscrape::storage::QueryInterface>& query) {
    print_horizontal_line();
    print_centered("Metadata Details", 80);
    print_horizontal_line();

    std::cout << "InfoHash:       " << metadata.info_hash.to_hex() << std::endl;
    std::cout << "Name:           " << metadata.name << std::endl;
    std::cout << "Total Size:     " << format_size(metadata.total_size) << std::endl;
    std::cout << "Piece Count:    " << metadata.piece_count << std::endl;
    std::cout << "File Count:     " << metadata.file_count << std::endl;
    std::cout << "Download Time:  " << format_time_point(metadata.download_time) << std::endl;

    if (!metadata.comment.empty()) {
        std::cout << "Comment:        " << metadata.comment << std::endl;
    }

    if (!metadata.created_by.empty()) {
        std::cout << "Created By:     " << metadata.created_by << std::endl;
    }

    if (metadata.creation_date) {
        std::cout << "Creation Date:  " << format_time_point(*metadata.creation_date) << std::endl;
    }

    // Display files if available
    auto files = query->get_files(metadata.info_hash);
    if (!files.empty()) {
        std::cout << "\nFiles:" << std::endl;
        print_horizontal_line();

        std::vector<int> widths = {60, 20};
        print_table_header({"Path", "Size"}, widths);

        for (const auto& file : files) {
            print_table_row({file.path, format_size(file.size)}, widths);
        }
    }

    print_horizontal_line();
}

// Functions for displaying statistics and lists
void display_statistics(const std::unordered_map<std::string, std::string>& stats) {
    print_horizontal_line();
    print_centered("BitScrape Statistics", 80);
    print_horizontal_line();

    // Controller statistics
    std::cout << "Controller Status:" << std::endl;
    std::cout << "  Running:        " << stats.at("controller.running") << std::endl;
    std::cout << "  Crawling:       " << stats.at("controller.crawling") << std::endl;

    // Storage statistics
    std::cout << "\nStorage Statistics:" << std::endl;
    if (stats.contains("storage.node_count")) {
        std::cout << "  Nodes:          " << stats.at("storage.node_count") << std::endl;
    }

    if (stats.contains("storage.infohash_count")) {
        std::cout << "  Infohashes:     " << stats.at("storage.infohash_count") << std::endl;
    }

    if (stats.contains("storage.metadata_count")) {
        std::cout << "  Metadata:       " << stats.at("storage.metadata_count") << std::endl;
    }

    // DHT statistics
    std::cout << "\nDHT Statistics:" << std::endl;
    for (const auto& [key, value] : stats) {
        if (key.find("dht.") == 0) {
            std::cout << "  " << std::left << std::setw(15) << key.substr(4) << ": " << value <<
                std::endl;
        }
    }

    // BitTorrent statistics
    std::cout << "\nBitTorrent Statistics:" << std::endl;
    for (const auto& [key, value] : stats) {
        if (key.find("bt.") == 0) {
            std::cout << "  " << std::left << std::setw(15) << key.substr(3) << ": " << value <<
                std::endl;
        }
    }

    // Tracker statistics
    std::cout << "\nTracker Statistics:" << std::endl;
    for (const auto& [key, value] : stats) {
        if (key.find("tracker.") == 0) {
            std::cout << "  " << std::left << std::setw(15) << key.substr(8) << ": " << value <<
                std::endl;
        }
    }

    print_horizontal_line();
}

void display_nodes_list(const std::vector<bitscrape::storage::NodeModel>& nodes,
                        size_t limit = 10) {
    print_horizontal_line();
    print_centered("DHT Nodes", 80);
    print_horizontal_line();

    if (nodes.empty()) {
        std::cout << "No nodes found." << std::endl;
        print_horizontal_line();
        return;
    }

    std::vector<int> widths = {45, 15, 10, 10};
    print_table_header({"Node ID", "IP:Port", "Responsive", "Last Seen"}, widths);

    size_t count = 0;
    for (const auto& node : nodes) {
        if (count >= limit)
            break;

        std::string node_id = node.node_id.to_hex().substr(0, 16) + "...";
        std::string endpoint = node.endpoint.address() + ":" + std::to_string(node.endpoint.port());
        std::string responsive = node.is_responsive ? "Yes" : "No";

        // Calculate time since last seen
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - node.last_seen);
        std::string last_seen = format_duration(duration) + " ago";

        print_table_row({node_id, endpoint, responsive, last_seen}, widths);
        count++;
    }

    if (nodes.size() > limit) {
        std::cout << "\nShowing " << limit << " of " << nodes.size() << " nodes." << std::endl;
    }

    print_horizontal_line();
}

void display_infohashes_list(const std::vector<bitscrape::storage::InfoHashModel>& infohashes,
                             size_t limit = 10) {
    print_horizontal_line();
    print_centered("InfoHashes", 80);
    print_horizontal_line();

    if (infohashes.empty()) {
        std::cout << "No infohashes found." << std::endl;
        print_horizontal_line();
        return;
    }

    std::vector<int> widths = {45, 15, 10, 10};
    print_table_header({"InfoHash", "Peers", "Metadata", "Last Seen"}, widths);

    size_t count = 0;
    for (const auto& infohash : infohashes) {
        if (count >= limit)
            break;

        std::string hash = infohash.info_hash.to_hex().substr(0, 16) + "...";
        std::string peers = std::to_string(infohash.peer_count);
        std::string has_metadata = infohash.has_metadata ? "Yes" : "No";

        // Calculate time since last seen
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - infohash.last_seen);
        std::string last_seen = format_duration(duration) + " ago";

        print_table_row({hash, peers, has_metadata, last_seen}, widths);
        count++;
    }

    if (infohashes.size() > limit) {
        std::cout << "\nShowing " << limit << " of " << infohashes.size() << " infohashes." <<
            std::endl;
    }

    print_horizontal_line();
}

void display_metadata_list(const std::vector<bitscrape::storage::MetadataModel>& metadata_list,
                           size_t limit = 10) {
    print_horizontal_line();
    print_centered("Metadata", 80);
    print_horizontal_line();

    if (metadata_list.empty()) {
        std::cout << "No metadata found." << std::endl;
        print_horizontal_line();
        return;
    }

    std::vector<int> widths = {40, 20, 10, 10};
    print_table_header({"Name", "Size", "Files", "Downloaded"}, widths);

    size_t count = 0;
    for (const auto& metadata : metadata_list) {
        if (count >= limit)
            break;

        std::string name = metadata.name;
        if (name.length() > 37) {
            name = name.substr(0, 34) + "...";
        }

        std::string size = format_size(metadata.total_size);
        std::string files = std::to_string(metadata.file_count);

        // Calculate time since download
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now - metadata.download_time);
        std::string download_time = format_duration(duration) + " ago";

        print_table_row({name, size, files, download_time}, widths);
        count++;
    }

    if (metadata_list.size() > limit) {
        std::cout << "\nShowing " << limit << " of " << metadata_list.size() << " metadata entries."
            << std::endl;
    }

    print_horizontal_line();
}

// Function to display help for interactive mode
void display_interactive_help() {
    print_horizontal_line();
    print_centered("BitScrape CLI - Interactive Mode Help", 80);
    print_horizontal_line();

    std::cout << "Available commands:" << std::endl;
    std::cout << "  help                   - Show this help message" << std::endl;
    std::cout << "  stats                  - Show statistics" << std::endl;
    std::cout << "  nodes [limit]          - List discovered DHT nodes" << std::endl;
    std::cout << "  node <node_id>         - Show details for a specific node" << std::endl;
    std::cout << "  infohashes [limit]     - List discovered infohashes" << std::endl;
    std::cout << "  infohash <infohash>    - Show details for a specific infohash" << std::endl;
    std::cout << "  metadata [limit]       - List downloaded metadata" << std::endl;
    std::cout << "  search <query>         - Search for metadata by name" << std::endl;
    std::cout << "  start                  - Start crawling" << std::endl;
    std::cout << "  stop                   - Stop crawling" << std::endl;
    std::cout << "  web status              - Show web interface status" << std::endl;
    std::cout << "  web start [port]        - Start web interface" << std::endl;
    std::cout << "  web stop                - Stop web interface" << std::endl;
    std::cout << "  web auto-start <on|off> - Enable/disable web interface auto-start" << std::endl;
    std::cout << "  clear                  - Clear the screen" << std::endl;
    std::cout << "  exit                   - Exit the application" << std::endl;

    print_horizontal_line();
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
    bool disable_web = false;
    uint16_t web_port = 8080;
    std::string static_dir = "public";

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
        } else if (arg == "--interactive" || arg == "-i") {
            interactive_mode = true;
        } else if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            config_path = argv[++i];
        } else if ((arg == "--database" || arg == "-d") && i + 1 < argc) {
            db_path = argv[++i];
        } else if (arg == "--no-web") {
            disable_web = true;
        } else if (arg.find("--port=") == 0) {
            web_port = std::stoi(arg.substr(7));
        } else if (arg.find("--static-dir=") == 0) {
            static_dir = arg.substr(13);
        }
        else {
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
    if (!controller->start()) {
        beacon->critical("Failed to start BitScrape", SYSTEM);
        return 1;
    }

    // Start crawling if requested
    if (start_crawling) {
        //beacon->info("Starting DHT crawling...", SYSTEM);
        if (!controller->start_crawling()) {
            beacon->error("Failed to start crawling", SYSTEM);
        }
    }

    beacon->info("BitScrape CLI started", SYSTEM);
    beacon->info("Version: 0.1.0", SYSTEM);

    // Check if web interface should be started automatically
    bool auto_start_web = !disable_web && controller->get_configuration()->get_bool(
                              "web.auto_start", true);

    // Get web port from configuration if not specified on command line
    if (auto_start_web && web_port == 8080) {
        web_port = static_cast<uint16_t>(controller->get_configuration()->
                                                     get_int("web.port", 8080));
    }

    // Get the static directory from configuration if not specified on the command line
    if (auto_start_web && static_dir == "public") {
        static_dir = controller->get_configuration()->get_string("web.static_dir", "public");
    }

    if (interactive_mode) {
        beacon->info("Starting in interactive mode", SYSTEM);
        beacon->info("Type 'help' for a list of commands", SYSTEM);

        // Start the web interface if auto-start is enabled and not explicitly disabled
        if (auto_start_web) {
            start_web_interface(config_path, web_port, static_dir, beacon);
        }

        // Get a query interface for data access
        auto& storage_manager = controller->get_storage_manager();
        auto query = storage_manager.query_interface();

        // Interactive command loop
        std::string command;
        std::cout << "\nBitScrape> ";

        while (running && std::getline(std::cin, command)) {
            // Skip empty commands
            if (command.empty()) {
                std::cout << "BitScrape> ";
                continue;
            }

            // Parse command and arguments
            std::istringstream iss(command);
            std::string cmd;
            iss >> cmd;
            cmd = to_lower(cmd);

            // Process command
            if (cmd == "help") {
                display_interactive_help();
            } else if (cmd == "stats") {
                auto stats = controller->get_statistics();
                display_statistics(stats);
            } else if (cmd == "nodes") {
                // Parse limit argument
                size_t limit = 10;
                if (iss >> limit) {
                    // Limit is provided
                }

                // Get nodes
                bitscrape::storage::QueryInterface::NodeQueryOptions options;
                options.limit = limit;
                options.order_by = "last_seen";
                options.order_desc = true;

                auto nodes = query->get_nodes(options);
                display_nodes_list(nodes, limit);
            } else if (cmd == "node") {
                // Parse node_id argument
                std::string node_id_str;
                if (iss >> node_id_str) {
                    try {
                        bitscrape::types::NodeID node_id(node_id_str);
                        auto node = query->get_node(node_id);

                        if (node) {
                            display_node_details(*node);
                        } else {
                            std::cout << "Node not found: " << node_id_str << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cout << "Invalid node ID: " << node_id_str << std::endl;
                    }
                } else {
                    std::cout << "Usage: node <node_id>" << std::endl;
                }
            } else if (cmd == "infohashes") {
                // Parse limit argument
                size_t limit = 10;
                if (iss >> limit) {
                    // Limit is provided
                }

                // Get infohashes
                bitscrape::storage::QueryInterface::InfoHashQueryOptions options;
                options.limit = limit;
                options.order_by = "last_seen";
                options.order_desc = true;

                auto infohashes = query->get_infohashes(options);
                display_infohashes_list(infohashes, limit);
            } else if (cmd == "infohash") {
                // Parse infohash argument
                std::string infohash_str;
                if (iss >> infohash_str) {
                    try {
                        bitscrape::types::InfoHash infohash(infohash_str);
                        auto infohash_model = query->get_infohash(infohash);

                        if (infohash_model) {
                            display_infohash_details(*infohash_model, query);
                        } else {
                            std::cout << "InfoHash not found: " << infohash_str << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cout << "Invalid InfoHash: " << infohash_str << std::endl;
                    }
                } else {
                    std::cout << "Usage: infohash <infohash>" << std::endl;
                }
            } else if (cmd == "metadata") {
                // Parse limit argument
                size_t limit = 10;
                if (iss >> limit) {
                    // Limit is provided
                }

                // Get metadata
                bitscrape::storage::QueryInterface::MetadataQueryOptions options;
                options.limit = limit;
                options.order_by = "download_time";
                options.order_desc = true;

                auto metadata_list = query->get_metadata_list(options);
                display_metadata_list(metadata_list, limit);
            } else if (cmd == "search") {
                // Parse search query
                std::string search_query;
                std::getline(iss >> std::ws, search_query);

                if (!search_query.empty()) {
                    // Search metadata by name
                    bitscrape::storage::QueryInterface::MetadataQueryOptions options;
                    options.name_contains = search_query;
                    options.limit = 20;
                    options.order_by = "download_time";
                    options.order_desc = true;

                    auto metadata_list = query->get_metadata_list(options);

                    std::cout << "Search results for '" << search_query << "':" << std::endl;
                    display_metadata_list(metadata_list);
                } else {
                    std::cout << "Usage: search <query>" << std::endl;
                }
            } else if (cmd == "start") {
                if (controller->start_crawling()) {
                    std::cout << "Crawling started." << std::endl;
                } else {
                    std::cout << "Failed to start crawling." << std::endl;
                }
            } else if (cmd == "stop") {
                if (controller->stop_crawling()) {
                    std::cout << "Crawling stopped." << std::endl;
                } else {
                    std::cout << "Failed to stop crawling." << std::endl;
                }
            } else if (cmd == "web") {
                std::string web_cmd;
                if (iss >> web_cmd) {
                    if (web_cmd == "status") {
                        // Show web interface status
                        bool auto_start = controller->get_configuration()->get_bool(
                            "web.auto_start", true);
                        std::cout << "Web interface auto-start: " << (auto_start
                                ? "enabled"
                                : "disabled") << std::endl;

                        if (http_server) {
                            std::cout << "Web interface is " << (http_server->is_running()
                                                                     ? "running"
                                                                     : "stopped") << std::endl;
                            if (http_server->is_running()) {
                                std::cout << "Port: " << http_server->port() << std::endl;
                                std::cout << "URL: http://localhost:" << http_server->port() <<
                                    std::endl;
                            }
                        } else {
                            std::cout << "Web interface is not initialized" << std::endl;
                        }
                    } else if (web_cmd == "start") {
                        // Start web interface
                        uint16_t port = 8080;
                        if (iss >> port) {
                            // Port is provided
                        }

                        if (http_server && http_server->is_running()) {
                            std::cout << "Web interface is already running on port " << http_server
                                ->port() << std::endl;
                        } else {
                            if (start_web_interface(config_path, port, "public",
                                                    controller->get_beacon())) {
                                std::cout << "Web interface started on port " << port << std::endl;
                                std::cout << "URL: http://localhost:" << port << std::endl;
                            }
                        }
                    } else if (web_cmd == "stop") {
                        // Stop web interface
                        if (http_server && http_server->is_running()) {
                            if (http_server->stop()) {
                                std::cout << "Web interface stopped" << std::endl;
                            } else {
                                std::cout << "Failed to stop web interface" << std::endl;
                            }
                        } else {
                            std::cout << "Web interface is not running" << std::endl;
                        }
                    } else if (web_cmd == "auto-start") {
                        // Enable or disable auto-start
                        std::string enable_str;
                        if (iss >> enable_str) {
                            bool enable = (
                                enable_str == "on" || enable_str == "true" || enable_str == "1");
                            controller->get_configuration()->set_bool("web.auto_start", enable);
                            controller->get_configuration()->save();
                            std::cout << "Web interface auto-start " << (enable
                                    ? "enabled"
                                    : "disabled") << std::endl;
                        } else {
                            std::cout << "Usage: web auto-start <on|off>" << std::endl;
                        }
                    } else {
                        std::cout << "Unknown web command: " << web_cmd << std::endl;
                        std::cout << "Available web commands: status, start, stop, auto-start" <<
                            std::endl;
                    }
                } else {
                    std::cout << "Usage: web <command>" << std::endl;
                    std::cout << "Available commands:" << std::endl;
                    std::cout << "  status                - Show web interface status" << std::endl;
                    std::cout << "  start [port]          - Start web interface" << std::endl;
                    std::cout << "  stop                  - Stop web interface" << std::endl;
                    std::cout << "  auto-start <on|off>   - Enable/disable auto-start" << std::endl;
                }
            } else if (cmd == "clear") {
                // Clear the screen (platform-dependent)
#ifdef _WIN32
                system("cls");
#else
                system("clear");
#endif
            } else if (cmd == "exit" || cmd == "quit") {
                std::cout << "Exiting..." << std::endl;
                running = false;
                break;
            } else {
                std::cout << "Unknown command: " << cmd << std::endl;
                std::cout << "Type 'help' for a list of commands." << std::endl;
            }

            if (running) {
                std::cout << "BitScrape> ";
            }
        }
    } else {
        // Non-interactive mode
        beacon->info("Press Ctrl+C to exit", SYSTEM);

        while (running) {
            // Print statistics periodically
            auto stats = controller->get_statistics();
            display_statistics(stats);

            // Sleep for a while
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }

    // Stop controller if not already stopped by signal handler
    if (running) {
        beacon->info("BitScrape CLI shutting down", SYSTEM);
        controller->stop();

        // Stop web interface if it was started
        if (http_server && http_server->is_running()) {
            beacon->info("Stopping web interface", SYSTEM);
            http_server->stop();
        }
    }

    return 0;
}
