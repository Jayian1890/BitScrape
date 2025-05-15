#include "bitscrape/lock/lock_manager_singleton.hpp"

namespace bitscrape::lock {

// Initialize static members
std::shared_ptr<LockManager> LockManagerSingleton::instance_ = nullptr;
std::mutex LockManagerSingleton::mutex_;

std::shared_ptr<LockManager> LockManagerSingleton::instance(
    std::shared_ptr<beacon::Beacon> beacon,
    bool enable_deadlock_detection,
    bool enable_logging) {
    
    // Double-checked locking pattern
    if (!instance_) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!instance_) {
            instance_ = std::make_shared<LockManager>(
                enable_deadlock_detection,
                enable_logging,
                beacon);
        }
    }
    
    return instance_;
}

void LockManagerSingleton::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    instance_ = nullptr;
}

} // namespace bitscrape::lock
