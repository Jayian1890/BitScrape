#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <random>
#include <chrono>

#include <bitscrape/storage/storage_manager.hpp>

namespace bitscrape::test {

// Create a unique temporary file path for a SQLite DB in the system temp dir.
static inline std::string make_temp_db_path(const std::string &suffix = "") {
    auto dir = std::filesystem::temp_directory_path();
    std::random_device rd;
    std::mt19937_64 rng(rd());
    uint64_t nonce = rng();
    auto name = "bitscrape_test_db_" + std::to_string(nonce) + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + suffix + ".db";
    return (dir / name).string();
}

// Initialize a storage manager backed by the given path and return it. The
// caller is responsible for cleaning up the file if desired.
static inline std::shared_ptr<bitscrape::storage::StorageManager> make_temp_storage(const std::string &path) {
    auto mgr = bitscrape::storage::create_storage_manager(path, true);
    if (!mgr->initialize()) {
        throw std::runtime_error("Failed to initialize temporary storage manager");
    }
    return mgr;
}

} // namespace bitscrape::test
