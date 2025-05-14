#include <gtest/gtest.h>
#include "bitscrape/lock/lock_manager.hpp"
#include "bitscrape/lock/lock_guard.hpp"
#include "bitscrape/lock/lock_exceptions.hpp"
#include "bitscrape/lock/lock_manager_singleton.hpp"
#include "bitscrape/beacon/beacon.hpp"

#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <future>

using namespace bitscrape::lock;
using namespace bitscrape::beacon;
using namespace bitscrape::types;

TEST(LockManagerTest, BasicLockAcquisition) {
    LockManager lock_manager;

    // Register a resource
    uint64_t resource_id = lock_manager.register_resource("test_resource");

    // Acquire a lock
    auto result = lock_manager.acquire_lock(resource_id);
    EXPECT_EQ(result, LockManager::AcquireResult::SUCCESS);

    // Release the lock
    bool released = lock_manager.release_lock(resource_id);
    EXPECT_TRUE(released);
}

TEST(LockManagerTest, LockGuard) {
    LockManager lock_manager;

    // Register a resource
    uint64_t resource_id = lock_manager.register_resource("test_resource");

    // Create a lock guard
    {
        auto guard = lock_manager.get_lock_guard(resource_id);
        EXPECT_FALSE(guard->is_released());

        // Check that the lock is held
        auto result = lock_manager.acquire_lock(resource_id);
        EXPECT_EQ(result, LockManager::AcquireResult::ALREADY_HELD);
    }

    // Check that the lock is released
    auto result = lock_manager.acquire_lock(resource_id);
    EXPECT_EQ(result, LockManager::AcquireResult::SUCCESS);

    // Release the lock
    bool released = lock_manager.release_lock(resource_id);
    EXPECT_TRUE(released);
}

TEST(LockManagerTest, SharedLocks) {
    LockManager lock_manager;

    // Register a resource
    uint64_t resource_id = lock_manager.register_resource("test_resource");

    // Acquire a shared lock
    auto result = lock_manager.acquire_lock(resource_id, LockManager::LockType::SHARED);
    EXPECT_EQ(result, LockManager::AcquireResult::SUCCESS);

    // Acquire another shared lock in a different thread
    auto future = std::async(std::launch::async, [&lock_manager, resource_id]() {
        return lock_manager.acquire_lock(resource_id, LockManager::LockType::SHARED);
    });

    // Wait for the future
    auto result2 = future.get();
    EXPECT_EQ(result2, LockManager::AcquireResult::SUCCESS);

    // Try to acquire an exclusive lock (should fail)
    auto result3 = lock_manager.acquire_lock(resource_id, LockManager::LockType::EXCLUSIVE, 100);
    EXPECT_EQ(result3, LockManager::AcquireResult::TIMEOUT);

    // Release the shared locks
    bool released = lock_manager.release_lock(resource_id);
    EXPECT_TRUE(released);

    // Release the other shared lock in a different thread
    auto future2 = std::async(std::launch::async, [&lock_manager, resource_id]() {
        return lock_manager.release_lock(resource_id);
    });

    // Wait for the future
    bool released2 = future2.get();
    EXPECT_TRUE(released2);

    // Now we should be able to acquire an exclusive lock
    auto result4 = lock_manager.acquire_lock(resource_id, LockManager::LockType::EXCLUSIVE);
    EXPECT_EQ(result4, LockManager::AcquireResult::SUCCESS);

    // Release the exclusive lock
    bool released3 = lock_manager.release_lock(resource_id);
    EXPECT_TRUE(released3);
}

TEST(LockManagerTest, UpgradableLocks) {
    LockManager lock_manager;

    // Register a resource
    uint64_t resource_id = lock_manager.register_resource("test_resource");

    // Acquire an upgradable lock
    auto result = lock_manager.acquire_lock(resource_id, LockManager::LockType::UPGRADABLE);
    EXPECT_EQ(result, LockManager::AcquireResult::SUCCESS);

    // Upgrade the lock
    auto result2 = lock_manager.upgrade_lock(resource_id);
    EXPECT_EQ(result2, LockManager::AcquireResult::SUCCESS);

    // Release the lock
    bool released = lock_manager.release_lock(resource_id);
    EXPECT_TRUE(released);
}

TEST(LockManagerTest, DeadlockDetection) {
    LockManager lock_manager(true, true);

    // Register two resources
    uint64_t resource1_id = lock_manager.register_resource("resource1", LockManager::LockPriority::NORMAL);
    uint64_t resource2_id = lock_manager.register_resource("resource2", LockManager::LockPriority::HIGH);

    // Acquire a lock on resource1
    auto result = lock_manager.acquire_lock(resource1_id);
    EXPECT_EQ(result, LockManager::AcquireResult::SUCCESS);

    // Try to acquire a lock on resource2 in a different thread
    auto future = std::async(std::launch::async, [&lock_manager, resource2_id]() {
        return lock_manager.acquire_lock(resource2_id);
    });

    // Wait for the future
    auto result2 = future.get();
    EXPECT_EQ(result2, LockManager::AcquireResult::SUCCESS);

    // Try to acquire a lock on resource2 in this thread
    auto result3 = lock_manager.acquire_lock(resource2_id, LockManager::LockType::EXCLUSIVE, 100);
    EXPECT_EQ(result3, LockManager::AcquireResult::TIMEOUT);

    // Try to acquire a lock on resource1 in the other thread
    auto future2 = std::async(std::launch::async, [&lock_manager, resource1_id]() {
        return lock_manager.acquire_lock(resource1_id, LockManager::LockType::EXCLUSIVE, 100);
    });

    // Wait for the future
    auto result4 = future2.get();
    EXPECT_EQ(result4, LockManager::AcquireResult::TIMEOUT);

    // Release the locks
    bool released = lock_manager.release_lock(resource1_id);
    EXPECT_TRUE(released);

    auto future3 = std::async(std::launch::async, [&lock_manager, resource2_id]() {
        return lock_manager.release_lock(resource2_id);
    });

    bool released2 = future3.get();
    EXPECT_TRUE(released2);
}

TEST(LockManagerTest, LockOrdering) {
    LockManager lock_manager(true, true);

    // Register two resources with different priorities
    uint64_t low_priority_id = lock_manager.register_resource("low_priority", LockManager::LockPriority::LOW);
    uint64_t high_priority_id = lock_manager.register_resource("high_priority", LockManager::LockPriority::HIGH);

    // Acquire a lock on the high priority resource
    auto result = lock_manager.acquire_lock(high_priority_id);
    EXPECT_EQ(result, LockManager::AcquireResult::SUCCESS);

    // Try to acquire a lock on the low priority resource (should fail due to lock ordering)
    auto result2 = lock_manager.acquire_lock(low_priority_id);
    EXPECT_EQ(result2, LockManager::AcquireResult::WOULD_DEADLOCK);

    // Release the high priority lock
    bool released = lock_manager.release_lock(high_priority_id);
    EXPECT_TRUE(released);

    // Now acquire the locks in the correct order
    auto result3 = lock_manager.acquire_lock(low_priority_id);
    EXPECT_EQ(result3, LockManager::AcquireResult::SUCCESS);

    auto result4 = lock_manager.acquire_lock(high_priority_id);
    EXPECT_EQ(result4, LockManager::AcquireResult::SUCCESS);

    // Release the locks
    bool released2 = lock_manager.release_lock(high_priority_id);
    EXPECT_TRUE(released2);

    bool released3 = lock_manager.release_lock(low_priority_id);
    EXPECT_TRUE(released3);
}

TEST(LockManagerTest, Timeout) {
    LockManager lock_manager;

    // Register a resource
    uint64_t resource_id = lock_manager.register_resource("test_resource");

    // Acquire a lock
    auto result = lock_manager.acquire_lock(resource_id);
    EXPECT_EQ(result, LockManager::AcquireResult::SUCCESS);

    // Try to acquire the lock again with a timeout
    auto start = std::chrono::steady_clock::now();
    auto result2 = lock_manager.acquire_lock(resource_id, LockManager::LockType::EXCLUSIVE, 100);
    auto end = std::chrono::steady_clock::now();

    EXPECT_EQ(result2, LockManager::AcquireResult::TIMEOUT);

    // Check that the timeout was respected
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_GE(duration, 100);
    EXPECT_LT(duration, 200);

    // Release the lock
    bool released = lock_manager.release_lock(resource_id);
    EXPECT_TRUE(released);
}

TEST(LockManagerTest, DumpLockState) {
    LockManager lock_manager(true, true);

    // Register some resources
    uint64_t resource1_id = lock_manager.register_resource("resource1", LockManager::LockPriority::LOW);
    uint64_t resource2_id = lock_manager.register_resource("resource2", LockManager::LockPriority::NORMAL);
    uint64_t resource3_id = lock_manager.register_resource("resource3", LockManager::LockPriority::HIGH);

    // Acquire some locks
    auto result1 = lock_manager.acquire_lock(resource1_id, LockManager::LockType::EXCLUSIVE);
    EXPECT_EQ(result1, LockManager::AcquireResult::SUCCESS);

    auto result2 = lock_manager.acquire_lock(resource2_id, LockManager::LockType::SHARED);
    EXPECT_EQ(result2, LockManager::AcquireResult::SUCCESS);

    // Dump the lock state
    std::string state = lock_manager.dump_lock_state();

    // Check that the state contains the resource names
    EXPECT_NE(state.find("resource1"), std::string::npos);
    EXPECT_NE(state.find("resource2"), std::string::npos);
    EXPECT_NE(state.find("resource3"), std::string::npos);

    // Check that the state contains the lock types
    EXPECT_NE(state.find("Exclusive"), std::string::npos);
    EXPECT_NE(state.find("Shared"), std::string::npos);

    // Release the locks
    bool released1 = lock_manager.release_lock(resource1_id);
    EXPECT_TRUE(released1);

    bool released2 = lock_manager.release_lock(resource2_id);
    EXPECT_TRUE(released2);
}

TEST(LockManagerSingletonTest, SingletonPattern) {
    // Reset the singleton to ensure a clean state
    LockManagerSingleton::reset();

    // Get the instance
    auto instance1 = LockManagerSingleton::instance();
    EXPECT_NE(instance1, nullptr);

    // Get the instance again, should be the same
    auto instance2 = LockManagerSingleton::instance();
    EXPECT_EQ(instance1, instance2);

    // Register a resource
    uint64_t resource_id = instance1->register_resource("test_resource");

    // Check that the resource is available in the second instance
    EXPECT_NO_THROW(instance2->get_resource_name(resource_id));
    EXPECT_EQ(instance2->get_resource_name(resource_id), "test_resource");

    // Reset the singleton
    LockManagerSingleton::reset();

    // Get a new instance
    auto instance3 = LockManagerSingleton::instance();
    EXPECT_NE(instance3, nullptr);

    // Should be a different instance
    EXPECT_NE(instance1, instance3);

    // The resource should no longer be available
    EXPECT_THROW(instance3->get_resource_name(resource_id), LockOperationException);
}

TEST(LockManagerSingletonTest, WithBeacon) {
    // Reset the singleton to ensure a clean state
    LockManagerSingleton::reset();

    // Create a beacon
    auto beacon = std::make_shared<Beacon>();

    // Get the instance with the beacon
    auto instance = LockManagerSingleton::instance(beacon, true, true);
    EXPECT_NE(instance, nullptr);

    // Register a resource
    uint64_t resource_id = instance->register_resource("test_resource");

    // Acquire a lock (should log through the beacon)
    auto result = instance->acquire_lock(resource_id);
    EXPECT_EQ(result, LockManager::AcquireResult::SUCCESS);

    // Release the lock (should log through the beacon)
    bool released = instance->release_lock(resource_id);
    EXPECT_TRUE(released);

    // Reset the singleton
    LockManagerSingleton::reset();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
