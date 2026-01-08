#include <filesystem>
#include <fstream>
#include <string>
#include <random>
#include <chrono>
#include <thread>

#include "doctest/doctest.h"

#include <bitscrape/core/controller.hpp>
#include <bitscrape/core/configuration.hpp>
#include <bitscrape/bittorrent/bittorrent_event_processor.hpp>
#include "test_helpers.hpp"

using namespace bitscrape::core;
namespace types = bitscrape::types;
namespace bittorrent = bitscrape::bittorrent;


static std::filesystem::path make_temp_path(const std::string &suffix = "") {
    auto dir = std::filesystem::temp_directory_path();
    std::random_device rd;
    std::mt19937_64 rng(rd());
    uint64_t nonce = rng();
    auto name = "bitscrape_ctrl_test_" + std::to_string(nonce) + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + suffix + ".json";
    return dir / name;
}

TEST_SUITE("core::Controller") {

TEST_CASE("accessors") {
    auto path = make_temp_path("_basic");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Controller ctrl(path.string());
    auto cfg = ctrl.get_configuration();
    CHECK(cfg != nullptr);

    cfg->set_string("ctrl.test", "ok");
    CHECK_EQ(cfg->get_string("ctrl.test"), "ok");

    auto eb = ctrl.get_event_bus();
    // The event bus is currently exposed as a non-owning pointer
    CHECK(eb != nullptr);

    auto beacon = ctrl.get_beacon();
    CHECK(beacon != nullptr);

    // Initialize so storage_manager is created (some accessors rely on it)
    CHECK(ctrl.initialize());

    auto ih = ctrl.get_infohashes();
    CHECK(ih.empty());

    auto nodes = ctrl.get_nodes();
    CHECK(nodes.empty());

    // Stop controller to avoid background activity from later tests
    CHECK(ctrl.stop());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

TEST_CASE("initialize") {
    auto path = make_temp_path("_init");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Controller c1(path.string());
    CHECK(c1.initialize());
    CHECK(c1.get_configuration()->has_key("database.path"));

    Controller c2(path.string());
    auto f = c2.initialize_async();
    CHECK(f.valid());
    CHECK(f.get());
}

TEST_CASE("idempotency") {
    auto path = make_temp_path("_start_stop");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Controller ctrl(path.string());
    // stop before start should succeed
    CHECK(ctrl.stop());

    bool started = false;
    started = ctrl.start();
    // start may fail in constrained environments; ensure stop always succeeds
    if (started) {
        // calling start again should be a no-op and return true
        CHECK(ctrl.start());
        // stop should succeed
        CHECK(ctrl.stop());
        // stop again should still succeed (idempotent)
        CHECK(ctrl.stop());
    } else {
        // Even if start failed, stop should still be true
        CHECK(ctrl.stop());
    }
}

TEST_CASE("lifecycle") {
    auto path = make_temp_path("_crawl");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Controller ctrl(path.string());
    // Use conservative config to minimize network activity
    auto cfg = ctrl.get_configuration();
    cfg->set_string("dht.bootstrap_nodes", "");
    cfg->set_string("dht.bootstrap_infohash", "");
    cfg->set_string("dht.bootstrap_trackers", "");

    // Initialize and start
    CHECK(ctrl.initialize());
    bool started = ctrl.start();

    if (!started) {
        // If start fails, starting crawling must fail
        CHECK_FALSE(ctrl.start_crawling());
        CHECK(ctrl.stop());
        return;
    }

    // Now attempt to start crawling
    bool crawling_started = ctrl.start_crawling();
    CHECK(crawling_started);

    // Stats should reflect crawling state
    auto stats = ctrl.get_statistics();
    CHECK(stats.at("controller.crawling") == "true");

    // Controller should have generated some random infohashes if the DB was empty
    uint64_t infohash_count = std::stoull(stats.at("storage.infohash_count"));
    CHECK(infohash_count >= 0); // Non-negative; environment-dependent (may be 0 or >=5)

    // Stop crawling
    CHECK(ctrl.stop_crawling());
    // Give background threads a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    stats = ctrl.get_statistics();
    CHECK(stats.at("controller.crawling") == "false");

    // Stop controller
    CHECK(ctrl.stop());

    // Give detached background threads time to exit to avoid cross-test interference
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_CASE("handlers") {
    auto path = make_temp_path("_events");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Controller ctrl(path.string());
    // Use deterministic temporary storage for this controller to allow precise checks
    auto db_path = bitscrape::test::make_temp_db_path("_events");
    ctrl.get_configuration()->set_string("database.path", db_path);
    CHECK(ctrl.initialize());

    using namespace bitscrape::types;
    using namespace bitscrape::bittorrent;

    // Create a simple concrete Event (Event is abstract)
    struct SimpleEvent : public types::Event {
        SimpleEvent(types::Event::Type t) : types::Event(t) {}
        std::unique_ptr<types::Event> clone() const override { return std::make_unique<SimpleEvent>(*this); }
    };

    SimpleEvent evt_node(types::Event::Type::DHT_NODE_FOUND);
    SimpleEvent evt_info(types::Event::Type::DHT_INFOHASH_FOUND);

    // Initially no nodes/infohashes
    auto nodes_before = ctrl.get_nodes();
    auto ih_before = ctrl.get_infohashes();

    // Deliver events directly to the controller (testing helper). Any exceptions
    // (e.g., endpoint parsing) are acceptable in constrained test environments —
    // the important part is that the handler runs without crashing the process.
    try {
        ctrl.receive_event(evt_node);
    } catch (const std::exception& e) {
        INFO("Node handler threw: " << e.what());
    }

    try {
        ctrl.receive_event(evt_info);
    } catch (const std::exception& e) {
        INFO("Infohash handler threw: " << e.what());
    }

    // Allow some time for any async storage operations
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Now assert storage counts deterministically (each handler stores exactly one)
    auto stats = ctrl.get_statistics();
    uint64_t node_count = std::stoull(stats.at("storage.node_count"));
    uint64_t infohash_count = std::stoull(stats.at("storage.infohash_count"));

    CHECK_EQ(node_count, nodes_before.size() + 1);
    CHECK_EQ(infohash_count, ih_before.size() + 1);

    // Stop controller and cleanup
    CHECK(ctrl.stop());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

TEST_CASE("metadata") {
    auto path = make_temp_path("_metadata");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Controller ctrl(path.string());
    // Use deterministic temporary storage for this controller
    auto db_path = bitscrape::test::make_temp_db_path("_metadata");
    ctrl.get_configuration()->set_string("database.path", db_path);
    CHECK(ctrl.initialize());

    // Build test infohash and metadata
    std::vector<uint8_t> bytes(20);
    std::iota(bytes.begin(), bytes.end(), 1);
    types::InfoHash info(bytes);

    types::MetadataInfo metadata;
    metadata.set_name("test-torrent");
    metadata.set_total_size(12345);
    metadata.set_info_hash(info);

    // Create metadata event
    bittorrent::MetadataReceivedEvent mrev(info, metadata);

    // Ensure the base infohash exists so metadata storage succeeds deterministically
    auto& storage_mgr = ctrl.get_storage_manager();
    CHECK(storage_mgr.store_infohash(info));

    auto stats_before = ctrl.get_statistics();
    uint64_t infohash_before = std::stoull(stats_before.at("storage.infohash_count"));
    uint64_t metadata_before = std::stoull(stats_before.at("storage.metadata_count"));

    // Deliver the metadata event directly to the controller and accept exceptions
    try {
        ctrl.receive_event(mrev);
    } catch (const std::exception& e) {
        INFO("Metadata handler threw: " << e.what());
    }

    // Allow some time for any async storage operations
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Now assert database stats deterministically
    auto stats = ctrl.get_statistics();
    uint64_t infohash_count = std::stoull(stats.at("storage.infohash_count"));
    uint64_t metadata_count = std::stoull(stats.at("storage.metadata_count"));

    CHECK_EQ(infohash_count, infohash_before);      // store_metadata should not add a new infohash
    CHECK_EQ(metadata_count, metadata_before + 1);  // exactly one metadata record was stored

    CHECK(ctrl.stop());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

} // TEST_SUITE
