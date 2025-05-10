#include "bitscrape/types/node_id.hpp"
#include "bitscrape/types/beacon_types.hpp"
#include "bitscrape/event/event_bus.hpp"
#include "bitscrape/beacon/beacon.hpp"
#include "bitscrape/beacon/console_sink.hpp"
#include "bitscrape/beacon/file_sink.hpp"
#include "bitscrape/beacon/event_sink.hpp"
#include "bitscrape/beacon/beacon_adapter.hpp"


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

    // Log shutdown message
    beacon->info("BitScrape CLI shutting down", SYSTEM);

    return 0;
}
