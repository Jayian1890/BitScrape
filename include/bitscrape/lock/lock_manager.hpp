#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <optional>
#include <functional>

#include "bitscrape/lock/lock_exceptions.hpp"
#include "bitscrape/beacon/beacon.hpp"

namespace bitscrape::lock {

// Forward declarations
class LockGuard;

/**
 * @brief Comprehensive lock management system to prevent deadlocks
 *
 * The LockManager provides a centralized mechanism for managing locks across
 * the application. It enforces lock ordering, detects potential deadlocks,
 * and provides timeout-based lock acquisition to prevent indefinite waiting.
 */
class LockManager {
public:
    /// Lock acquisition result
    enum class AcquireResult {
        SUCCESS,        ///< Lock acquired successfully
        TIMEOUT,        ///< Timed out waiting for lock
        WOULD_DEADLOCK, ///< Acquiring the lock would cause a deadlock
        ALREADY_HELD    ///< Lock is already held by the current thread
    };

    /// Lock types
    enum class LockType {
        EXCLUSIVE,  ///< Exclusive lock (write)
        SHARED,     ///< Shared lock (read)
        UPGRADABLE  ///< Upgradable lock (read that can be upgraded to write)
    };

    /// Lock priority levels (higher numbers = higher priority)
    enum class LockPriority {
        LOWEST  = 0,
        LOW     = 25,
        NORMAL  = 50,
        HIGH    = 75,
        HIGHEST = 100
    };

    /**
     * @brief Create a new lock manager
     *
     * @param enable_deadlock_detection Whether to enable deadlock detection
     * @param enable_logging Whether to enable lock acquisition logging
     * @param beacon Optional beacon for logging
     */
    explicit LockManager(bool enable_deadlock_detection = true, bool enable_logging = false,
                        std::shared_ptr<beacon::Beacon> beacon = nullptr);

    /**
     * @brief Destructor
     */
    ~LockManager();

    /**
     * @brief Register a new lockable resource
     *
     * @param resource_name Name of the resource
     * @param priority Priority of the resource (used for lock ordering)
     * @return Resource ID
     */
    uint64_t register_resource(const std::string& resource_name, LockPriority priority = LockPriority::NORMAL);

    /**
     * @brief Acquire a lock on a resource
     *
     * @param resource_id Resource ID
     * @param lock_type Type of lock to acquire
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return AcquireResult Result of the lock acquisition
     */
    AcquireResult acquire_lock(uint64_t resource_id,
                              LockType lock_type = LockType::EXCLUSIVE,
                              uint64_t timeout_ms = 0);

    /**
     * @brief Release a lock on a resource
     *
     * @param resource_id Resource ID
     * @return true if the lock was released, false if it wasn't held
     */
    bool release_lock(uint64_t resource_id);

    /**
     * @brief Try to upgrade a shared lock to an exclusive lock
     *
     * @param resource_id Resource ID
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return AcquireResult Result of the lock upgrade
     */
    AcquireResult upgrade_lock(uint64_t resource_id, uint64_t timeout_ms = 0);

    /**
     * @brief Check if acquiring a lock would cause a deadlock
     *
     * @param resource_id Resource ID
     * @return true if acquiring the lock would cause a deadlock
     */
    bool would_deadlock(uint64_t resource_id) const;

    /**
     * @brief Get a lock guard for a resource
     *
     * @param resource_id Resource ID
     * @param lock_type Type of lock to acquire
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return Unique pointer to a lock guard
     * @throws DeadlockException if acquiring the lock would cause a deadlock
     * @throws TimeoutException if the lock acquisition times out
     */
    std::unique_ptr<LockGuard> get_lock_guard(uint64_t resource_id,
                                             LockType lock_type = LockType::EXCLUSIVE,
                                             uint64_t timeout_ms = 0);

    /**
     * @brief Get the current thread's lock stack
     *
     * @return Vector of resource IDs currently locked by the thread
     */
    std::vector<uint64_t> get_lock_stack() const;

    /**
     * @brief Get the name of a resource
     *
     * @param resource_id Resource ID
     * @return Resource name
     */
    std::string get_resource_name(uint64_t resource_id) const;

    /**
     * @brief Get the priority of a resource
     *
     * @param resource_id Resource ID
     * @return Resource priority
     */
    LockPriority get_resource_priority(uint64_t resource_id) const;

    /**
     * @brief Dump the current lock state for debugging
     *
     * @return String representation of the lock state
     */
    std::string dump_lock_state() const;

private:
    /**
     * @brief Resource information
     */
    struct Resource {
        std::string name;                  ///< Resource name
        LockPriority priority;             ///< Resource priority
        std::shared_mutex mutex;           ///< Underlying mutex
        std::atomic<bool> locked_exclusive; ///< Whether the resource is locked exclusively
        std::atomic<int> shared_count;     ///< Number of shared locks
        std::thread::id exclusive_owner;   ///< Thread ID of the exclusive owner (if any)
        std::vector<std::thread::id> shared_owners; ///< Thread IDs of shared owners
    };

    /**
     * @brief Thread lock information
     */
    struct ThreadLockInfo {
        std::vector<uint64_t> lock_stack;  ///< Stack of locks held by the thread
        std::unordered_map<uint64_t, LockType> lock_types; ///< Types of locks held
    };

    /**
     * @brief Get or create thread lock info for the current thread
     *
     * @return Reference to the thread lock info
     */
    ThreadLockInfo& get_thread_lock_info();

    /**
     * @brief Check if the current thread holds a lock on a resource
     *
     * @param resource_id Resource ID
     * @param lock_type Type of lock to check for (or std::nullopt to check for any lock)
     * @return true if the thread holds the specified lock
     */
    bool thread_holds_lock(uint64_t resource_id, std::optional<LockType> lock_type = std::nullopt) const;

    /**
     * @brief Check if acquiring a lock would violate the lock ordering
     *
     * @param resource_id Resource ID
     * @return true if acquiring the lock would violate the lock ordering
     */
    bool would_violate_lock_ordering(uint64_t resource_id) const;

    /**
     * @brief Log a lock acquisition
     *
     * @param resource_id Resource ID
     * @param lock_type Type of lock
     * @param result Result of the acquisition
     */
    void log_lock_acquisition(uint64_t resource_id, LockType lock_type, AcquireResult result);

    /**
     * @brief Log a lock release
     *
     * @param resource_id Resource ID
     */
    void log_lock_release(uint64_t resource_id);

    /**
     * @brief Get a resource by ID
     *
     * @param resource_id Resource ID
     * @return Pointer to the resource
     * @throws std::out_of_range if the resource doesn't exist
     */
    Resource* get_resource(uint64_t resource_id);

    /**
     * @brief Get a resource by ID (const version)
     *
     * @param resource_id Resource ID
     * @return Const pointer to the resource
     * @throws std::out_of_range if the resource doesn't exist
     */
    const Resource* get_resource(uint64_t resource_id) const;

    std::unordered_map<uint64_t, Resource> resources_;  ///< Resources by ID
    mutable std::mutex resources_mutex_;                ///< Mutex for thread-safe access to resources

    std::unordered_map<std::thread::id, ThreadLockInfo> thread_locks_; ///< Locks held by each thread
    mutable std::mutex thread_locks_mutex_;             ///< Mutex for thread-safe access to thread_locks_

    std::atomic<uint64_t> next_resource_id_;            ///< Next resource ID

    bool enable_deadlock_detection_;                    ///< Whether deadlock detection is enabled
    bool enable_logging_;                               ///< Whether logging is enabled
    std::shared_ptr<beacon::Beacon> beacon_;            ///< Beacon for logging
};

} // namespace bitscrape::lock
