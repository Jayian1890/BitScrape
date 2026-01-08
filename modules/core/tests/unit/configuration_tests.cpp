#include <filesystem>
#include <fstream>
#include <string>
#include <random>
#include <chrono>

#include "doctest/doctest.h"

#include <bitscrape/core/configuration.hpp>
#include <bitscrape/core/controller.hpp>
#include <bitscrape/types/endpoint.hpp>

using namespace bitscrape::core;
using namespace bitscrape::types;

static std::filesystem::path make_temp_path(const std::string &suffix = "") {
    auto dir = std::filesystem::temp_directory_path();
    std::random_device rd;
    std::mt19937_64 rng(rd());
    uint64_t nonce = rng();
    auto name = "bitscrape_test_" + std::to_string(nonce) + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + suffix + ".json";
    return dir / name;
}

TEST_SUITE("core::Configuration") {

TEST_CASE("primitives") {
    auto path = make_temp_path("_primitives");
    // Ensure no pre-existing file
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Configuration cfg(path.string());

    // Initially not present, defaults should not be visible until load() creates them
    CHECK_FALSE(cfg.has_key("database.path"));

    // Set/get string
    cfg.set_string("foo", "bar");
    CHECK_EQ(cfg.get_string("foo"), "bar");

    // Set/get int
    cfg.set_int("int.val", 42);
    CHECK_EQ(cfg.get_int("int.val"), 42);
    CHECK_EQ(cfg.get_int("no.such", 7), 7);

    // Set/get bool
    cfg.set_bool("flag", true);
    CHECK(cfg.get_bool("flag"));
    CHECK_FALSE(cfg.get_bool("missing.flag", false) == true);

    // Remove key
    CHECK(cfg.has_key("foo"));
    CHECK(cfg.remove_key("foo"));
    CHECK_FALSE(cfg.has_key("foo"));

    // Clear
    cfg.set_string("a", "b");
    cfg.clear();
    CHECK_FALSE(cfg.has_key("a"));
}

TEST_CASE("lists") {
    auto path = make_temp_path("_lists");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Configuration cfg(path.string());

    std::vector<std::string> fruits = {"apple", "banana", "cantaloupe"};
    cfg.set_string_list("fruits", fruits);
    auto got = cfg.get_string_list("fruits");
    CHECK_EQ(got.size(), fruits.size());
    CHECK_EQ(got[0], "apple");

    // Endpoints
    std::vector<Endpoint> eps = { Endpoint(std::string("127.0.0.1"), 8080), Endpoint(std::string("127.0.0.1"), 6881) };
    cfg.set_endpoint_list("peers", eps);
    auto parsed = cfg.get_endpoint_list("peers");
    CHECK_EQ(parsed.size(), 2);
    CHECK(parsed[0].address() == "127.0.0.1");
    CHECK(parsed[0].port() == 8080);
}

TEST_CASE("save_load") {
    auto path = make_temp_path("_save_load");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Configuration cfg(path.string());
    cfg.set_string("s", "hello");
    cfg.set_int("n", 123);
    cfg.set_bool("b", false);
    std::vector<std::string> arr = {"one", "2", "3"};
    cfg.set_string_list("arr", arr);

    CHECK(cfg.save());
    REQUIRE(std::filesystem::exists(path));

    // Create new instance and load file
    Configuration cfg2(path.string());
    CHECK(cfg2.load());

    CHECK_EQ(cfg2.get_string("s"), "hello");
    CHECK_EQ(cfg2.get_int("n"), 123);
    CHECK_FALSE(cfg2.get_bool("b"));

    auto arr2 = cfg2.get_string_list("arr");
    CHECK_EQ(arr2.size(), 3);
    CHECK_EQ(arr2[1], "2");
}

TEST_CASE("defaults") {
    auto path = make_temp_path("_defaults");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Configuration cfg(path.string());
    // Loading non-existent file should create defaults and save them
    CHECK(cfg.load());
    CHECK(std::filesystem::exists(path));
    // Default should include database.path
    CHECK(cfg.has_key("database.path"));
    CHECK_EQ(cfg.get_string("database.path"), "bitscrape.db");
}

TEST_CASE("malformed") {
    auto path = make_temp_path("_malformed");
    // Write malformed content
    {
        std::ofstream f(path);
        f << "not a json";
    }

    Configuration cfg(path.string());
    CHECK_FALSE(cfg.load());
}

TEST_CASE("async") {
    auto path = make_temp_path("_async");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Configuration cfg(path.string());
    cfg.set_string("async", "yes");
    auto fut = cfg.save_async();
    CHECK(fut.valid());
    CHECK(fut.get());

    Configuration cfg2(path.string());
    auto lf = cfg2.load_async();
    CHECK(lf.valid());
    CHECK(lf.get());
    // "yes" is interpreted as a boolean on save/load -> check boolean view
    CHECK(cfg2.get_bool("async", false));
}

TEST_CASE("keys") {
    auto path = make_temp_path("_all");
    std::error_code ec;
    std::filesystem::remove(path, ec);

    Configuration cfg(path.string());
    cfg.set_string("k1", "v1");
    cfg.set_string("k2", "v2");

    auto keys = cfg.get_keys();
    auto all = cfg.get_all();

    CHECK_EQ(keys.size(), all.size());
    CHECK(all.at("k1") == "v1");
}


} // TEST_SUITE
