#include <gtest/gtest.h>
#include <bitscrape/core/controller.hpp>
#include <bitscrape/core/configuration.hpp>
#include <bitscrape/event/event_bus.hpp>
#include <bitscrape/beacon/beacon.hpp>

#include <filesystem>
#include <thread>
#include <chrono>

namespace bitscrape::core::test {

class ControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary configuration file
        config_path_ = "controller_test_config.conf";

        // Remove any existing test files
        std::filesystem::remove(config_path_);
        std::filesystem::remove("controller_test.db");
    }

    void TearDown() override {
        // Clean up test files
        std::filesystem::remove(config_path_);
        std::filesystem::remove("controller_test.db");
    }

    std::string config_path_;
};

TEST_F(ControllerTest, ConstructorTest) {
    // Test default constructor
    Controller controller;
    EXPECT_NE(nullptr, controller.get_configuration());
    EXPECT_NE(nullptr, controller.get_storage_manager());
    EXPECT_NE(nullptr, controller.get_event_bus());
    EXPECT_NE(nullptr, controller.get_beacon());

    // Test constructor with config path
    Controller controller_with_path(config_path_);
    EXPECT_NE(nullptr, controller_with_path.get_configuration());
    EXPECT_NE(nullptr, controller_with_path.get_storage_manager());
    EXPECT_NE(nullptr, controller_with_path.get_event_bus());
    EXPECT_NE(nullptr, controller_with_path.get_beacon());
    EXPECT_EQ(config_path_, controller_with_path.get_configuration()->get_config_path());
}

TEST_F(ControllerTest, InitializeTest) {
    Controller controller(config_path_);

    // Test initialization
    EXPECT_TRUE(controller.initialize());

    // Verify that configuration was created
    EXPECT_TRUE(std::filesystem::exists(config_path_));

    // Verify that database was created
    std::string db_path = controller.get_configuration()->get_string("database.path", "");
    EXPECT_FALSE(db_path.empty());
    EXPECT_TRUE(std::filesystem::exists(db_path));
}

TEST_F(ControllerTest, InitializeAsyncTest) {
    Controller controller(config_path_);

    // Test asynchronous initialization
    auto future = controller.initialize_async();
    EXPECT_TRUE(future.get());

    // Verify that configuration was created
    EXPECT_TRUE(std::filesystem::exists(config_path_));

    // Verify that database was created
    std::string db_path = controller.get_configuration()->get_string("database.path", "");
    EXPECT_FALSE(db_path.empty());
    EXPECT_TRUE(std::filesystem::exists(db_path));
}

TEST_F(ControllerTest, StartStopTest) {
    Controller controller(config_path_);

    // Initialize controller
    EXPECT_TRUE(controller.initialize());

    // Test start
    EXPECT_TRUE(controller.start());

    // Test stop
    EXPECT_TRUE(controller.stop());
}

TEST_F(ControllerTest, StartStopAsyncTest) {
    Controller controller(config_path_);

    // Initialize controller
    EXPECT_TRUE(controller.initialize());

    // Test asynchronous start
    auto start_future = controller.start_async();
    EXPECT_TRUE(start_future.get());

    // Test asynchronous stop
    auto stop_future = controller.stop_async();
    EXPECT_TRUE(stop_future.get());
}

TEST_F(ControllerTest, CrawlingTest) {
    Controller controller(config_path_);

    // Initialize and start controller
    EXPECT_TRUE(controller.initialize());
    EXPECT_TRUE(controller.start());

    // Test start crawling
    EXPECT_TRUE(controller.start_crawling());

    // Test stop crawling
    EXPECT_TRUE(controller.stop_crawling());

    // Clean up
    EXPECT_TRUE(controller.stop());
}

TEST_F(ControllerTest, CrawlingAsyncTest) {
    Controller controller(config_path_);

    // Initialize and start controller
    EXPECT_TRUE(controller.initialize());
    EXPECT_TRUE(controller.start());

    // Test asynchronous start crawling
    auto start_future = controller.start_crawling_async();
    EXPECT_TRUE(start_future.get());

    // Test asynchronous stop crawling
    auto stop_future = controller.stop_crawling_async();
    EXPECT_TRUE(stop_future.get());

    // Clean up
    EXPECT_TRUE(controller.stop());
}

TEST_F(ControllerTest, GetStatisticsTest) {
    Controller controller(config_path_);

    // Initialize and start controller
    EXPECT_TRUE(controller.initialize());
    EXPECT_TRUE(controller.start());

    // Get statistics
    auto stats = controller.get_statistics();

    // Verify statistics
    EXPECT_FALSE(stats.empty());
    EXPECT_EQ("false", stats["controller.crawling"]);
    EXPECT_EQ("true", stats["controller.running"]);

    // Clean up
    EXPECT_TRUE(controller.stop());
}

TEST_F(ControllerTest, GetInfohashesTest) {
    Controller controller(config_path_);

    // Initialize and start controller
    EXPECT_TRUE(controller.initialize());
    EXPECT_TRUE(controller.start());

    // Get infohashes (should be empty initially)
    auto infohashes = controller.get_infohashes();
    EXPECT_TRUE(infohashes.empty());

    // Clean up
    EXPECT_TRUE(controller.stop());
}

TEST_F(ControllerTest, GetNodesTest) {
    Controller controller(config_path_);

    // Initialize and start controller
    EXPECT_TRUE(controller.initialize());
    EXPECT_TRUE(controller.start());

    // Get nodes (should be empty initially)
    auto nodes = controller.get_nodes();
    EXPECT_TRUE(nodes.empty());

    // Clean up
    EXPECT_TRUE(controller.stop());
}

} // namespace bitscrape::core::test
