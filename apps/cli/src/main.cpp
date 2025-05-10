#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/beacon_types.hpp"
#include "bitscrape/event/event_bus.hpp"
#include "bitscrape/beacon/beacon.hpp"
#include "bitscrape/beacon/console_sink.hpp"
#include "bitscrape/beacon/file_sink.hpp"
#include "bitscrape/beacon/event_sink.hpp"
#include "bitscrape/beacon/beacon_adapter.hpp"

#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{
    // Use enum for cleaner code
    using enum bitscrape::types::BeaconCategory;
    using enum bitscrape::types::BeaconSeverity;

    // Create an event bus
    auto event_bus = bitscrape::event::create_event_bus();

    // Create a beacon
    auto beacon = bitscrape::beacon::create_beacon();

    // Add a console sink with colors
    beacon->add_sink(bitscrape::beacon::create_console_sink(true));

    // Add a file sink
    beacon->add_sink(bitscrape::beacon::create_file_sink("bitscrape.log"));

    // Add an event sink
    beacon->add_sink(bitscrape::beacon::create_event_sink(*event_bus));

    // Create a beacon adapter
    auto beacon_adapter = bitscrape::beacon::create_beacon_adapter(*beacon);

    // Connect the adapter to the event bus
    beacon_adapter->connect(*event_bus);

    // Log some messages
    beacon->info("BitScrape CLI started", SYSTEM);
    beacon->info("Version: 0.1.0", SYSTEM);

    // Create a random node ID
    bitscrape::types::NodeID node_id = bitscrape::types::NodeID::secure_random();
    beacon->info("Random Node ID: " + node_id.to_hex(), SYSTEM);

    // Log messages at different severity levels
    beacon->debug("This is a debug message", GENERAL);
    beacon->info("This is an info message", GENERAL);
    beacon->warning("This is a warning message", GENERAL);
    beacon->error("This is an error message", GENERAL);
    beacon->critical("This is a critical message", GENERAL);

    // Log messages in different categories
    beacon->info("Network message", NETWORK);
    beacon->info("DHT message", DHT);
    beacon->info("BitTorrent message", BITTORRENT);

    // Log a message asynchronously
    auto future = beacon->info_async("This message is logged asynchronously", SYSTEM);

    // Wait for the async log to complete
    future.wait();

    // Log shutdown message
    beacon->info("BitScrape CLI shutting down", SYSTEM);

    return 0;
}
