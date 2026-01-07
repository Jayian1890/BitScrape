#include <bitscrape/testing.hpp>
#include <bitscrape/core/configuration.hpp>
#include <bitscrape/types/endpoint.hpp>

#include <filesystem>
#include <thread>
#include <chrono>

namespace bitscrape::core::test {

class ConfigurationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary configuration file
        config_path_ = "config_test.json";

        // Remove any existing test file
        std::filesystem::remove(config_path_);
    }

    void TearDown() override {
        // Clean up test file
        std::filesystem::remove(config_path_);
    }

    std::string config_path_;
};

TEST_F(ConfigurationTest, ConstructorTest) {
    // Test default constructor
    Configuration config;
    EXPECT_EQ("bitscrape.json", config.get_config_path());

    // Test constructor with config path
    Configuration config_with_path(config_path_);
    EXPECT_EQ(config_path_, config_with_path.get_config_path());
}

TEST_F(ConfigurationTest, LoadSaveTest) {
    Configuration config(config_path_);

    // Test save
    EXPECT_TRUE(config.save());
    EXPECT_TRUE(std::filesystem::exists(config_path_));

    // Test load
    EXPECT_TRUE(config.load());
}

TEST_F(ConfigurationTest, LoadSaveAsyncTest) {
    Configuration config(config_path_);

    // Test asynchronous save
    auto save_future = config.save_async();
    EXPECT_TRUE(save_future.get());
    EXPECT_TRUE(std::filesystem::exists(config_path_));

    // Test asynchronous load
    auto load_future = config.load_async();
    EXPECT_TRUE(load_future.get());
}

TEST_F(ConfigurationTest, SetGetStringTest) {
    Configuration config(config_path_);

    // Test set_string and get_string
    config.set_string("test_key", "test_value");
    EXPECT_EQ("test_value", config.get_string("test_key"));

    // Test default value
    EXPECT_EQ("default", config.get_string("non_existent_key", "default"));

    // Test save and load
    EXPECT_TRUE(config.save());

    // Create a new configuration object to load from the same file
    Configuration config2(config_path_);
    EXPECT_TRUE(config2.load());

    // Verify that the value was loaded correctly
    EXPECT_EQ("test_value", config2.get_string("test_key"));
}

TEST_F(ConfigurationTest, SetGetIntTest) {
    Configuration config(config_path_);

    // Test set_int and get_int
    config.set_int("test_key", 42);
    EXPECT_EQ(42, config.get_int("test_key"));

    // Test default value
    EXPECT_EQ(123, config.get_int("non_existent_key", 123));

    // Test save and load
    EXPECT_TRUE(config.save());

    // Create a new configuration object to load from the same file
    Configuration config2(config_path_);
    EXPECT_TRUE(config2.load());

    // Verify that the value was loaded correctly
    EXPECT_EQ(42, config2.get_int("test_key"));
}

TEST_F(ConfigurationTest, SetGetBoolTest) {
    Configuration config(config_path_);

    // Test set_bool and get_bool
    config.set_bool("test_key", true);
    EXPECT_TRUE(config.get_bool("test_key"));

    config.set_bool("test_key2", false);
    EXPECT_FALSE(config.get_bool("test_key2"));

    // Test default value
    EXPECT_TRUE(config.get_bool("non_existent_key", true));

    // Test save and load
    EXPECT_TRUE(config.save());

    // Create a new configuration object to load from the same file
    Configuration config2(config_path_);
    EXPECT_TRUE(config2.load());

    // Verify that the values were loaded correctly
    EXPECT_TRUE(config2.get_bool("test_key"));
    EXPECT_FALSE(config2.get_bool("test_key2"));
}

TEST_F(ConfigurationTest, SetGetStringListTest) {
    Configuration config(config_path_);

    // Test set_string_list and get_string_list
    std::vector<std::string> test_list = {"item1", "item2", "item3"};
    config.set_string_list("test_key", test_list);

    auto result = config.get_string_list("test_key");
    EXPECT_EQ(test_list.size(), result.size());
    for (size_t i = 0; i < test_list.size(); ++i) {
        EXPECT_EQ(test_list[i], result[i]);
    }

    // Test empty list
    std::vector<std::string> empty_list;
    config.set_string_list("empty_key", empty_list);
    EXPECT_TRUE(config.get_string_list("empty_key").empty());

    // Test save and load
    EXPECT_TRUE(config.save());

    // Create a new configuration object to load from the same file
    Configuration config2(config_path_);
    EXPECT_TRUE(config2.load());

    // Verify that the list was loaded correctly
    auto loaded_result = config2.get_string_list("test_key");
    EXPECT_EQ(test_list.size(), loaded_result.size());
    for (size_t i = 0; i < test_list.size(); ++i) {
        EXPECT_EQ(test_list[i], loaded_result[i]);
    }
}

TEST_F(ConfigurationTest, SetGetEndpointListTest) {
    Configuration config(config_path_);

    // Test set_endpoint_list and get_endpoint_list
    std::vector<types::Endpoint> test_list = {
        types::Endpoint(std::string("192.168.1.1"), 6881),
        types::Endpoint(std::string("example.com"), 6882),
        types::Endpoint(std::string("10.0.0.1"), 6883)
    };
    config.set_endpoint_list("test_key", test_list);

    auto result = config.get_endpoint_list("test_key");
    EXPECT_EQ(test_list.size(), result.size());
    for (size_t i = 0; i < test_list.size(); ++i) {
        EXPECT_EQ(test_list[i].address(), result[i].address());
        EXPECT_EQ(test_list[i].port(), result[i].port());
    }

    // Test empty list
    std::vector<types::Endpoint> empty_list;
    config.set_endpoint_list("empty_key", empty_list);
    EXPECT_TRUE(config.get_endpoint_list("empty_key").empty());

    // Test save and load
    EXPECT_TRUE(config.save());

    // Create a new configuration object to load from the same file
    Configuration config2(config_path_);
    EXPECT_TRUE(config2.load());

    // Verify that the list was loaded correctly
    auto loaded_result = config2.get_endpoint_list("test_key");
    EXPECT_EQ(test_list.size(), loaded_result.size());
    for (size_t i = 0; i < test_list.size(); ++i) {
        EXPECT_EQ(test_list[i].address(), loaded_result[i].address());
        EXPECT_EQ(test_list[i].port(), loaded_result[i].port());
    }
}

TEST_F(ConfigurationTest, HasKeyTest) {
    Configuration config(config_path_);

    // Test has_key
    EXPECT_FALSE(config.has_key("test_key"));

    config.set_string("test_key", "test_value");
    EXPECT_TRUE(config.has_key("test_key"));
}

TEST_F(ConfigurationTest, RemoveKeyTest) {
    Configuration config(config_path_);

    // Test remove_key
    config.set_string("test_key", "test_value");
    EXPECT_TRUE(config.has_key("test_key"));

    EXPECT_TRUE(config.remove_key("test_key"));
    EXPECT_FALSE(config.has_key("test_key"));

    // Test removing non-existent key
    EXPECT_FALSE(config.remove_key("non_existent_key"));
}

TEST_F(ConfigurationTest, ClearTest) {
    Configuration config(config_path_);

    // Add some values
    config.set_string("key1", "value1");
    config.set_int("key2", 42);
    config.set_bool("key3", true);

    EXPECT_TRUE(config.has_key("key1"));
    EXPECT_TRUE(config.has_key("key2"));
    EXPECT_TRUE(config.has_key("key3"));

    // Test clear
    config.clear();

    EXPECT_FALSE(config.has_key("key1"));
    EXPECT_FALSE(config.has_key("key2"));
    EXPECT_FALSE(config.has_key("key3"));
}

TEST_F(ConfigurationTest, GetKeysTest) {
    Configuration config(config_path_);

    // Add some values
    config.set_string("key1", "value1");
    config.set_int("key2", 42);
    config.set_bool("key3", true);

    // Test get_keys
    auto keys = config.get_keys();

    EXPECT_EQ(3UL, keys.size());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key1") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key2") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key3") != keys.end());
}

TEST_F(ConfigurationTest, GetAllTest) {
    Configuration config(config_path_);

    // Add some values
    config.set_string("key1", "value1");
    config.set_int("key2", 42);
    config.set_bool("key3", true);

    // Test get_all
    auto all = config.get_all();

    EXPECT_EQ(3UL, all.size());
    EXPECT_EQ("value1", all["key1"]);
    EXPECT_EQ("42", all["key2"]);
    EXPECT_EQ("1", all["key3"]);
}

} // namespace bitscrape::core::test
