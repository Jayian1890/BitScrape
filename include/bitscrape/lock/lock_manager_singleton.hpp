#pragma once

#include "bitscrape/lock/lock_manager.hpp"
#include "bitscrape/beacon/beacon.hpp"

#include <memory>
#include <mutex>

namespace bitscrape::lock {

/**
 * @brief Singleton for accessing a shared LockManager instance
 * 
 * The LockManagerSingleton provides a global access point to a shared
 * LockManager instance. It ensures that only one instance of the
 * LockManager exists throughout the application.
 */
class LockManagerSingleton {
public:
    /**
     * @brief Get the LockManager instance
     * 
     * @param beacon Optional beacon for logging
     * @param enable_deadlock_detection Whether to enable deadlock detection
     * @param enable_logging Whether to enable lock acquisition logging
     * @return Shared pointer to the LockManager instance
     */
    static std::shared_ptr<LockManager> instance(
        std::shared_ptr<beacon::Beacon> beacon = nullptr,
        bool enable_deadlock_detection = true,
        bool enable_logging = false);
    
    /**
     * @brief Reset the LockManager instance (for testing purposes)
     */
    static void reset();
    
private:
    /**
     * @brief Private constructor to prevent instantiation
     */
    LockManagerSingleton() = default;
    
    /**
     * @brief Private destructor to prevent deletion
     */
    ~LockManagerSingleton() = default;
    
    /**
     * @brief Private copy constructor to prevent copying
     */
    LockManagerSingleton(const LockManagerSingleton&) = delete;
    
    /**
     * @brief Private assignment operator to prevent assignment
     */
    LockManagerSingleton& operator=(const LockManagerSingleton&) = delete;
    
    static std::shared_ptr<LockManager> instance_; ///< Shared instance
    static std::mutex mutex_; ///< Mutex for thread-safe access to the instance
};

} // namespace bitscrape::lock
