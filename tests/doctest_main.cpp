#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

#include <bitscrape/core/controller.hpp>
#include <bitscrape/core/configuration.hpp>
#include <filesystem>

int main(int argc, char** argv) {
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int result = context.run();
    if (context.shouldExit()) {
        return result;
    }
    return result;
}

TEST_SUITE("Controller Tests") {
    // Lightweight runtime sanity check that boots a controller against a temp config
// and asserts all module checks pass. This keeps the check close to the test entrypoint
// so it runs in both CI and local test runs.
TEST_CASE("controller sanity checks") {
    using bitscrape::core::Controller;

    auto temp_dir = std::filesystem::temp_directory_path();
    auto cfg_path = (temp_dir / "bitscrape_sanity.json").string();
    auto db_path = (temp_dir / "bitscrape_sanity.db").string();

    // Seed a config file with minimal overrides before the controller loads it.
    {
        bitscrape::core::Configuration cfg(cfg_path);
        REQUIRE(cfg.load());
        cfg.set_string("database.path", db_path);
        cfg.set_int("dht.port", 0);              // let OS choose an available UDP port
        cfg.set_string("dht.bootstrap_nodes", ""); // avoid network resolution in tests
        cfg.save();
    }

    Controller controller(cfg_path);
    REQUIRE(controller.initialize());
    REQUIRE(controller.start());

    auto checks = controller.run_sanity_checks();
    for (const auto& check : checks) {
        CHECK_MESSAGE(check.ok, check.module << " failed: " << check.message);
    }

    controller.stop();
}

} // namespace bitscrape::core