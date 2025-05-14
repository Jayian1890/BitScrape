#pragma once

#include "bitscrape/lock/lock_manager.hpp"

namespace bitscrape::lock {

/**
 * @brief RAII-based lock guard
 * 
 * LockGuard provides RAII-based lock management, automatically releasing
 * locks when they go out of scope.
 */
class LockGuard {
public:
    /**
     * @brief Create a new lock guard
     * 
     * @param lock_manager Lock manager
     * @param resource_id Resource ID
     * @param lock_type Type of lock
     */
    LockGuard(LockManager& lock_manager, uint64_t resource_id, LockManager::LockType lock_type);
    
    /**
     * @brief Destructor (releases the lock)
     */
    ~LockGuard();
    
    /**
     * @brief Upgrade the lock (if it's upgradable)
     * 
     * @param timeout_ms Timeout in milliseconds
     * @return true if the lock was upgraded
     * @throws DeadlockException if upgrading would cause a deadlock
     * @throws TimeoutException if the upgrade times out
     */
    bool upgrade(uint64_t timeout_ms = 0);
    
    /**
     * @brief Release the lock early
     */
    void release();

    /**
     * @brief Get the resource ID
     * 
     * @return Resource ID
     */
    uint64_t resource_id() const;

    /**
     * @brief Get the lock type
     * 
     * @return Lock type
     */
    LockManager::LockType lock_type() const;

    /**
     * @brief Check if the lock is released
     * 
     * @return true if the lock is released
     */
    bool is_released() const;
    
private:
    LockManager& lock_manager_;            ///< Lock manager
    uint64_t resource_id_;                 ///< Resource ID
    LockManager::LockType lock_type_;      ///< Lock type
    bool released_;                        ///< Whether the lock is released
};

} // namespace bitscrape::lock
