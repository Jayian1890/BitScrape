#include "bitscrape/lock/lock_manager.hpp"
#include "bitscrape/lock/lock_guard.hpp"
#include "bitscrape/lock/lock_exceptions.hpp"

#include <sstream>
#include <algorithm>
#include <chrono>
#include <set>
#include <queue>
#include <iomanip>
#include <iostream>

namespace bitscrape::lock {

LockManager::LockManager(bool enable_deadlock_detection, bool enable_logging, std::shared_ptr<beacon::Beacon> beacon)
    : next_resource_id_(1), enable_deadlock_detection_(enable_deadlock_detection), enable_logging_(false), beacon_(beacon) {
}

LockManager::~LockManager() {
    // Check if there are any locks still held
    std::lock_guard<std::mutex> lock(thread_locks_mutex_);

    if (!thread_locks_.empty()) {
        // Log a warning about leaked locks
        if (enable_logging_) {
            std::stringstream ss;
            ss << "LockManager destroyed with " << thread_locks_.size() << " threads still holding locks:";

            for (const auto& [thread_id, info] : thread_locks_) {
                ss << "\n  Thread " << thread_id << " holds " << info.lock_stack.size() << " locks:";

                for (const auto& resource_id : info.lock_stack) {
                    try {
                        const Resource* resource = get_resource(resource_id);
                        ss << "\n    " << resource->name << " (" << resource_id << ")";
                    } catch (const std::out_of_range&) {
                        ss << "\n    Unknown resource (" << resource_id << ")";
                    }
                }
            }

            // Log the warning using the beacon system
            if (beacon_) {
                beacon_->warning(ss.str(), types::BeaconCategory::SYSTEM);
            } else {
                std::cout << ss.str() << std::endl;
            }
        }
    }
}

uint64_t LockManager::register_resource(const std::string& resource_name, LockPriority priority) {
    std::lock_guard<std::mutex> lock(resources_mutex_);

    // Generate a new resource ID
    uint64_t resource_id = next_resource_id_++;

    // Create a new resource
    Resource resource;
    resource.name = resource_name;
    resource.priority = priority;
    resource.locked_exclusive = false;
    resource.shared_count = 0;

    // Add the resource to the map
    // We can't use operator= because std::shared_mutex is not copyable or movable
    // Create a new resource in-place
    auto result = resources_.try_emplace(resource_id);
    if (result.second) {
        // If insertion was successful, initialize the resource
        auto& new_resource = result.first->second;
        new_resource.name = resource.name;
        new_resource.priority = resource.priority;
        new_resource.locked_exclusive.store(resource.locked_exclusive.load());
        new_resource.shared_count.store(resource.shared_count.load());
    }

    return resource_id;
}

LockManager::AcquireResult LockManager::acquire_lock(uint64_t resource_id, LockType lock_type, uint64_t timeout_ms) {
    // Check if the thread already holds the lock
    if (thread_holds_lock(resource_id, lock_type)) {
        return AcquireResult::ALREADY_HELD;
    }

    // Check if acquiring the lock would cause a deadlock
    if (enable_deadlock_detection_ && would_deadlock(resource_id)) {
        // Log the deadlock
        if (enable_logging_) {
            log_lock_acquisition(resource_id, lock_type, AcquireResult::WOULD_DEADLOCK);
        }

        return AcquireResult::WOULD_DEADLOCK;
    }

    // Check if acquiring the lock would violate the lock ordering
    if (enable_deadlock_detection_ && would_violate_lock_ordering(resource_id)) {
        // Log the lock ordering violation
        if (enable_logging_) {
            log_lock_acquisition(resource_id, lock_type, AcquireResult::WOULD_DEADLOCK);
        }

        return AcquireResult::WOULD_DEADLOCK;
    }

    // Get the resource
    Resource* resource;
    try {
        resource = get_resource(resource_id);
    } catch (const std::out_of_range&) {
        throw LockOperationException("Resource does not exist");
    }

    // Try to acquire the lock
    bool acquired = false;

    if (timeout_ms == 0) {
        // No timeout, try to acquire the lock immediately
        switch (lock_type) {
            case LockType::EXCLUSIVE:
                acquired = resource->mutex.try_lock();
                break;

            case LockType::SHARED:
                acquired = resource->mutex.try_lock_shared();
                break;

            case LockType::UPGRADABLE:
                // Upgradable locks are implemented as shared locks for now
                acquired = resource->mutex.try_lock_shared();
                break;
        }
    } else {
        // With timeout, try to acquire the lock with a timeout
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

        switch (lock_type) {
            case LockType::EXCLUSIVE:
                // Try to acquire the lock with a timeout
                {

                    while (std::chrono::steady_clock::now() < deadline) {
                        if (resource->mutex.try_lock()) {
                            acquired = true;
                            break;
                        }
                        // Sleep for a short time to avoid busy waiting
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
                break;

            case LockType::SHARED:
                // Try to acquire the shared lock with a timeout
                {

                    while (std::chrono::steady_clock::now() < deadline) {
                        if (resource->mutex.try_lock_shared()) {
                            acquired = true;
                            break;
                        }
                        // Sleep for a short time to avoid busy waiting
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
                break;

            case LockType::UPGRADABLE:
                // Upgradable locks are implemented as shared locks for now
                // Try to acquire the shared lock with a timeout
                {

                    while (std::chrono::steady_clock::now() < deadline) {
                        if (resource->mutex.try_lock_shared()) {
                            acquired = true;
                            break;
                        }
                        // Sleep for a short time to avoid busy waiting
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
                break;
        }
    }

    // Check if the lock was acquired
    if (!acquired) {
        // Log the timeout
        if (enable_logging_) {
            log_lock_acquisition(resource_id, lock_type, AcquireResult::TIMEOUT);
        }

        return AcquireResult::TIMEOUT;
    }

    // Update the resource state
    if (lock_type == LockType::EXCLUSIVE) {
        resource->locked_exclusive = true;
        resource->exclusive_owner = std::this_thread::get_id();
    } else {
        resource->shared_count++;
        resource->shared_owners.push_back(std::this_thread::get_id());
    }

    // Update the thread lock info
    ThreadLockInfo& info = get_thread_lock_info();
    info.lock_stack.push_back(resource_id);
    info.lock_types[resource_id] = lock_type;

    // Log the acquisition
    if (enable_logging_) {
        log_lock_acquisition(resource_id, lock_type, AcquireResult::SUCCESS);
    }

    return AcquireResult::SUCCESS;
}

bool LockManager::release_lock(uint64_t resource_id) {
    // Check if the thread holds the lock
    if (!thread_holds_lock(resource_id)) {
        return false;
    }

    // Get the resource
    Resource* resource;
    try {
        resource = get_resource(resource_id);
    } catch (const std::out_of_range&) {
        throw LockOperationException("Resource does not exist");
    }

    // Get the thread lock info
    ThreadLockInfo& info = get_thread_lock_info();

    // Get the lock type
    auto it = info.lock_types.find(resource_id);
    if (it == info.lock_types.end()) {
        return false;
    }

    LockType lock_type = it->second;

    // Release the lock
    switch (lock_type) {
        case LockType::EXCLUSIVE:
            resource->locked_exclusive = false;
            resource->exclusive_owner = std::thread::id();
            resource->mutex.unlock();
            break;

        case LockType::SHARED:
        case LockType::UPGRADABLE:
            resource->shared_count--;

            // Remove this thread from the shared owners
            auto owner_it = std::find(resource->shared_owners.begin(), resource->shared_owners.end(), std::this_thread::get_id());
            if (owner_it != resource->shared_owners.end()) {
                resource->shared_owners.erase(owner_it);
            }

            resource->mutex.unlock_shared();
            break;
    }

    // Update the thread lock info
    auto stack_it = std::find(info.lock_stack.begin(), info.lock_stack.end(), resource_id);
    if (stack_it != info.lock_stack.end()) {
        info.lock_stack.erase(stack_it);
    }

    info.lock_types.erase(resource_id);

    // Log the release
    if (enable_logging_) {
        log_lock_release(resource_id);
    }

    return true;
}

LockManager::AcquireResult LockManager::upgrade_lock(uint64_t resource_id, uint64_t timeout_ms) {
    // Check if the thread holds a shared or upgradable lock
    if (!thread_holds_lock(resource_id, LockType::SHARED) && !thread_holds_lock(resource_id, LockType::UPGRADABLE)) {
        return AcquireResult::ALREADY_HELD;
    }

    // Get the resource
    Resource* resource;
    try {
        resource = get_resource(resource_id);
    } catch (const std::out_of_range&) {
        throw LockOperationException("Resource does not exist");
    }

    // Get the thread lock info
    ThreadLockInfo& info = get_thread_lock_info();

    // Get the lock type
    auto it = info.lock_types.find(resource_id);
    if (it == info.lock_types.end()) {
        return AcquireResult::ALREADY_HELD;
    }

    LockType lock_type = it->second;

    // Check if the lock is already exclusive
    if (lock_type == LockType::EXCLUSIVE) {
        return AcquireResult::ALREADY_HELD;
    }

    // Release the shared lock
    resource->shared_count--;

    // Remove this thread from the shared owners
    auto owner_it = std::find(resource->shared_owners.begin(), resource->shared_owners.end(), std::this_thread::get_id());
    if (owner_it != resource->shared_owners.end()) {
        resource->shared_owners.erase(owner_it);
    }

    resource->mutex.unlock_shared();

    // Try to acquire an exclusive lock
    bool acquired = false;

    if (timeout_ms == 0) {
        // No timeout, try to acquire the lock immediately
        acquired = resource->mutex.try_lock();
    } else {
        // With timeout, try to acquire the lock with a timeout
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        // Try to acquire the lock with a timeout

        while (std::chrono::steady_clock::now() < deadline) {
            if (resource->mutex.try_lock()) {
                acquired = true;
                break;
            }
            // Sleep for a short time to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // Check if the lock was acquired
    if (!acquired) {
        // Re-acquire the shared lock
        resource->mutex.lock_shared();
        resource->shared_count++;
        resource->shared_owners.push_back(std::this_thread::get_id());

        // Log the timeout
        if (enable_logging_) {
            log_lock_acquisition(resource_id, LockType::EXCLUSIVE, AcquireResult::TIMEOUT);
        }

        return AcquireResult::TIMEOUT;
    }

    // Update the resource state
    resource->locked_exclusive = true;
    resource->exclusive_owner = std::this_thread::get_id();

    // Update the thread lock info
    info.lock_types[resource_id] = LockType::EXCLUSIVE;

    // Log the upgrade
    if (enable_logging_) {
        log_lock_acquisition(resource_id, LockType::EXCLUSIVE, AcquireResult::SUCCESS);
    }

    return AcquireResult::SUCCESS;
}

bool LockManager::would_deadlock(uint64_t resource_id) const {
    // If deadlock detection is disabled, return false
    if (!enable_deadlock_detection_) {
        return false;
    }

    // Get the resource
    const Resource* resource;
    try {
        resource = get_resource(resource_id);
    } catch (const std::out_of_range&) {
        throw LockOperationException("Resource does not exist");
    }

    // Check if the resource is already locked exclusively by another thread
    if (resource->locked_exclusive && resource->exclusive_owner != std::this_thread::get_id()) {
        // The resource is locked exclusively by another thread
        // Check if that thread is waiting for a resource that this thread holds

        std::lock_guard<std::mutex> lock(thread_locks_mutex_);

        // Get the thread ID of the exclusive owner
        std::thread::id owner_id = resource->exclusive_owner;

        // Get the thread lock info for the owner
        auto it = thread_locks_.find(owner_id);
        if (it == thread_locks_.end()) {
            // The owner thread doesn't have any locks registered
            return false;
        }

        // Check if the owner thread is waiting for a resource that this thread holds
        // const ThreadLockInfo& owner_info = it->second;

        // Get the thread lock info for this thread
        auto this_it = thread_locks_.find(std::this_thread::get_id());
        if (this_it == thread_locks_.end()) {
            // This thread doesn't have any locks registered
            return false;
        }

        const ThreadLockInfo& this_info = this_it->second;

        // Check if there's a cycle in the wait-for graph
        std::set<std::thread::id> visited;
        std::queue<std::thread::id> queue;

        // Start with the owner thread
        queue.push(owner_id);
        visited.insert(owner_id);

        while (!queue.empty()) {
            std::thread::id current_id = queue.front();
            queue.pop();

            // Get the thread lock info for the current thread
            auto current_it = thread_locks_.find(current_id);
            if (current_it == thread_locks_.end()) {
                continue;
            }

            const ThreadLockInfo& current_info = current_it->second;

            // Check if the current thread holds any locks that this thread is waiting for
            for (const auto& current_resource_id : current_info.lock_stack) {
                // Check if this thread holds this resource
                for (const auto& this_resource_id : this_info.lock_stack) {
                    if (current_resource_id == this_resource_id) {
                        // Found a cycle
                        return true;
                    }
                }

                // Check if any other thread is waiting for this resource
                for (const auto& [thread_id, info] : thread_locks_) {
                    if (thread_id == current_id || visited.count(thread_id) > 0) {
                        continue;
                    }

                    // Check if this thread is waiting for the current resource
                    for (const auto& waiting_resource_id : info.lock_stack) {
                        if (waiting_resource_id == current_resource_id) {
                            // Add this thread to the queue
                            queue.push(thread_id);
                            visited.insert(thread_id);
                            break;
                        }
                    }
                }
            }
        }
    }

    return false;
}

std::unique_ptr<LockGuard> LockManager::get_lock_guard(uint64_t resource_id, LockType lock_type, uint64_t timeout_ms) {
    return std::make_unique<LockGuard>(*this, resource_id, lock_type, timeout_ms);
}

std::vector<uint64_t> LockManager::get_lock_stack() const {
    // Get the thread lock info for this thread
    std::lock_guard<std::mutex> lock(thread_locks_mutex_);

    auto it = thread_locks_.find(std::this_thread::get_id());
    if (it == thread_locks_.end()) {
        return {};
    }

    return it->second.lock_stack;
}

std::string LockManager::get_resource_name(uint64_t resource_id) const {
    // Get the resource
    const Resource* resource;
    try {
        resource = get_resource(resource_id);
    } catch (const std::out_of_range&) {
        throw LockOperationException("Resource does not exist");
    }

    return resource->name;
}

LockManager::LockPriority LockManager::get_resource_priority(uint64_t resource_id) const {
    // Get the resource
    const Resource* resource;
    try {
        resource = get_resource(resource_id);
    } catch (const std::out_of_range&) {
        throw LockOperationException("Resource does not exist");
    }

    return resource->priority;
}

std::string LockManager::dump_lock_state() const {
    std::stringstream ss;

    ss << "Lock Manager State:";

    // Dump resources
    {
        std::lock_guard<std::mutex> lock(resources_mutex_);

        ss << "\nResources (" << resources_.size() << "):";

        for (const auto& [id, resource] : resources_) {
            ss << "\n  " << id << ": " << resource.name;
            ss << " (Priority: " << static_cast<int>(resource.priority) << ")";

            if (resource.locked_exclusive) {
                ss << " [Exclusive: " << resource.exclusive_owner << "]";
            }

            if (resource.shared_count > 0) {
                ss << " [Shared: " << resource.shared_count << " threads]";
            }
        }
    }

    // Dump thread locks
    {
        std::lock_guard<std::mutex> lock(thread_locks_mutex_);

        ss << "\nThread Locks (" << thread_locks_.size() << "):";

        for (const auto& [thread_id, info] : thread_locks_) {
            ss << "\n  Thread " << thread_id << " holds " << info.lock_stack.size() << " locks:";

            for (const auto& resource_id : info.lock_stack) {
                try {
                    const Resource* resource = get_resource(resource_id);
                    ss << "\n    " << resource->name << " (" << resource_id << ")";

                    auto type_it = info.lock_types.find(resource_id);
                    if (type_it != info.lock_types.end()) {
                        switch (type_it->second) {
                            case LockType::EXCLUSIVE:
                                ss << " [Exclusive]";
                                break;

                            case LockType::SHARED:
                                ss << " [Shared]";
                                break;

                            case LockType::UPGRADABLE:
                                ss << " [Upgradable]";
                                break;
                        }
                    }
                } catch (const std::out_of_range&) {
                    ss << "\n    Unknown resource (" << resource_id << ")";
                }
            }
        }
    }

    return ss.str();
}

LockManager::ThreadLockInfo& LockManager::get_thread_lock_info() {
    std::lock_guard<std::mutex> lock(thread_locks_mutex_);

    // Get or create the thread lock info for this thread
    return thread_locks_[std::this_thread::get_id()];
}

bool LockManager::thread_holds_lock(uint64_t resource_id, std::optional<LockType> lock_type) const {
    std::lock_guard<std::mutex> lock(thread_locks_mutex_);

    // Get the thread lock info for this thread
    auto it = thread_locks_.find(std::this_thread::get_id());
    if (it == thread_locks_.end()) {
        return false;
    }

    const ThreadLockInfo& info = it->second;

    // Check if the thread holds the lock
    auto type_it = info.lock_types.find(resource_id);
    if (type_it == info.lock_types.end()) {
        return false;
    }

    // Check if the lock type matches
    if (lock_type.has_value() && type_it->second != lock_type.value()) {
        return false;
    }

    return true;
}

bool LockManager::would_violate_lock_ordering(uint64_t resource_id) const {
    // If deadlock detection is disabled, return false
    if (!enable_deadlock_detection_) {
        return false;
    }

    // Get the resource
    const Resource* resource;
    try {
        resource = get_resource(resource_id);
    } catch (const std::out_of_range&) {
        throw LockOperationException("Resource does not exist");
    }

    // Get the thread lock info for this thread
    std::lock_guard<std::mutex> lock(thread_locks_mutex_);

    auto it = thread_locks_.find(std::this_thread::get_id());
    if (it == thread_locks_.end()) {
        return false;
    }

    const ThreadLockInfo& info = it->second;

    // Check if the thread holds any locks with a lower priority
    for (const auto& held_resource_id : info.lock_stack) {
        try {
            const Resource* held_resource = get_resource(held_resource_id);

            if (static_cast<int>(held_resource->priority) < static_cast<int>(resource->priority)) {
                // The thread holds a lock with a lower priority
                return true;
            }
        } catch (const std::out_of_range&) {
            // Ignore unknown resources
        }
    }

    return false;
}

void LockManager::log_lock_acquisition(uint64_t resource_id, LockType lock_type, AcquireResult result) {
    if (!enable_logging_) {
        return;
    }

    // Get the resource name
    std::string resource_name;
    try {
        resource_name = get_resource_name(resource_id);
    } catch (const std::out_of_range&) {
        resource_name = "Unknown";
    }

    // Format the lock type
    std::string lock_type_str;
    switch (lock_type) {
        case LockType::EXCLUSIVE:
            lock_type_str = "Exclusive";
            break;

        case LockType::SHARED:
            lock_type_str = "Shared";
            break;

        case LockType::UPGRADABLE:
            lock_type_str = "Upgradable";
            break;
    }

    // Format the result
    std::string result_str;
    switch (result) {
        case AcquireResult::SUCCESS:
            result_str = "Success";
            break;

        case AcquireResult::TIMEOUT:
            result_str = "Timeout";
            break;

        case AcquireResult::WOULD_DEADLOCK:
            result_str = "Would Deadlock";
            break;

        case AcquireResult::ALREADY_HELD:
            result_str = "Already Held";
            break;
    }

    // Format the message
    std::stringstream ss;
    ss << "Thread " << std::this_thread::get_id() << " acquiring " << lock_type_str
       << " lock on " << resource_name << " (" << resource_id << "): " << result_str;

    // Log the message using the beacon system
    if (beacon_) {
        // Determine the appropriate severity based on the result
        if (result == AcquireResult::SUCCESS) {
            beacon_->debug(ss.str(), types::BeaconCategory::SYSTEM);
        } else if (result == AcquireResult::ALREADY_HELD) {
            beacon_->debug(ss.str(), types::BeaconCategory::SYSTEM);
        } else if (result == AcquireResult::TIMEOUT) {
            beacon_->warning(ss.str(), types::BeaconCategory::SYSTEM);
        } else if (result == AcquireResult::WOULD_DEADLOCK) {
            beacon_->error(ss.str(), types::BeaconCategory::SYSTEM);
        }
    } else {
        if (beacon_) {
            beacon_->info(ss.str(), types::BeaconCategory::SYSTEM);
        } else {
            std::cout << ss.str() << std::endl;
        }
    }
}

void LockManager::log_lock_release(uint64_t resource_id) {
    if (!enable_logging_) {
        return;
    }

    // Get the resource name
    std::string resource_name;
    try {
        resource_name = get_resource_name(resource_id);
    } catch (const std::out_of_range&) {
        resource_name = "Unknown";
    }

    // Format the message
    std::stringstream ss;
    ss << "Thread " << std::this_thread::get_id() << " releasing lock on "
       << resource_name << " (" << resource_id << ")";

    // Log the message using the beacon system
    if (beacon_) {
        beacon_->debug(ss.str(), types::BeaconCategory::SYSTEM);
    } else {
        std::cout << ss.str() << std::endl;
    }
}

LockManager::Resource* LockManager::get_resource(uint64_t resource_id) {
    std::lock_guard<std::mutex> lock(resources_mutex_);

    auto it = resources_.find(resource_id);
    if (it == resources_.end()) {
        throw std::out_of_range("Resource not found");
    }

    return &it->second;
}

const LockManager::Resource* LockManager::get_resource(uint64_t resource_id) const {
    std::lock_guard<std::mutex> lock(resources_mutex_);

    auto it = resources_.find(resource_id);
    if (it == resources_.end()) {
        throw std::out_of_range("Resource not found");
    }

    return &it->second;
}

} // namespace bitscrape::lock
