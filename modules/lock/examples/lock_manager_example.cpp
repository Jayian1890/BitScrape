#include "bitscrape/lock/lock_manager_singleton.hpp"
#include "bitscrape/beacon/beacon.hpp"
#include "bitscrape/beacon/console_sink.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

using namespace bitscrape::lock;
using namespace bitscrape::beacon;
using namespace bitscrape::types;

// Example function that uses the LockManager
void example_function(int thread_id, uint64_t resource1_id, uint64_t resource2_id) {
    // Get the LockManager instance
    auto lock_manager = LockManagerSingleton::instance();
    
    std::cout << "Thread " << thread_id << " starting" << std::endl;
    
    // Acquire locks in the correct order (higher priority first)
    auto guard1 = lock_manager->get_lock_guard(resource2_id, LockManager::LockType::EXCLUSIVE);
    std::cout << "Thread " << thread_id << " acquired lock on resource2" << std::endl;
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto guard2 = lock_manager->get_lock_guard(resource1_id, LockManager::LockType::EXCLUSIVE);
    std::cout << "Thread " << thread_id << " acquired lock on resource1" << std::endl;
    
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "Thread " << thread_id << " releasing locks" << std::endl;
    // Locks are automatically released when the guards go out of scope
}

int main() {
    // Create a beacon for logging
    auto beacon = std::make_shared<Beacon>();
    beacon->add_sink(std::make_unique<ConsoleSink>());
    
    // Initialize the LockManager singleton with the beacon
    auto lock_manager = LockManagerSingleton::instance(beacon, true, true);
    
    // Register some resources with different priorities
    uint64_t resource1_id = lock_manager->register_resource("resource1", LockManager::LockPriority::LOW);
    uint64_t resource2_id = lock_manager->register_resource("resource2", LockManager::LockPriority::HIGH);
    
    // Create some threads that use the LockManager
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(example_function, i, resource1_id, resource2_id);
    }
    
    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Dump the lock state
    std::cout << lock_manager->dump_lock_state() << std::endl;
    
    // Reset the LockManager singleton (for testing purposes)
    LockManagerSingleton::reset();
    
    return 0;
}
