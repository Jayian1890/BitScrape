#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>

namespace bitscrape::lock {

/**
 * @brief Exception thrown when a deadlock is detected
 */
class DeadlockException : public std::runtime_error {
public:
    /**
     * @brief Create a new deadlock exception
     * 
     * @param message Error message
     * @param resources Resources involved in the deadlock
     */
    DeadlockException(const std::string& message, const std::vector<uint64_t>& resources);
    
    /**
     * @brief Get the resources involved in the deadlock
     * 
     * @return Vector of resource IDs involved in the deadlock
     */
    const std::vector<uint64_t>& get_resources() const;
    
private:
    std::vector<uint64_t> resources_;  ///< Resources involved in the deadlock
};

/**
 * @brief Exception thrown when a lock acquisition times out
 */
class TimeoutException : public std::runtime_error {
public:
    /**
     * @brief Create a new timeout exception
     * 
     * @param message Error message
     * @param resource_id Resource ID that timed out
     */
    TimeoutException(const std::string& message, uint64_t resource_id);
    
    /**
     * @brief Get the resource that timed out
     * 
     * @return Resource ID
     */
    uint64_t get_resource() const;
    
private:
    uint64_t resource_id_;  ///< Resource ID that timed out
};

/**
 * @brief Exception thrown when a lock operation is invalid
 */
class LockOperationException : public std::runtime_error {
public:
    /**
     * @brief Create a new lock operation exception
     * 
     * @param message Error message
     */
    explicit LockOperationException(const std::string& message);
};

} // namespace bitscrape::lock
