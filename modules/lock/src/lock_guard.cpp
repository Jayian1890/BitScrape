#include "bitscrape/lock/lock_guard.hpp"
#include "bitscrape/lock/lock_exceptions.hpp"

namespace bitscrape::lock {

LockGuard::LockGuard(LockManager& lock_manager, uint64_t resource_id, LockManager::LockType lock_type)
    : lock_manager_(lock_manager), resource_id_(resource_id), lock_type_(lock_type), released_(false) {
    
    // Acquire the lock
    auto result = lock_manager_.acquire_lock(resource_id_, lock_type_, 0);
    
    // Check the result
    switch (result) {
        case LockManager::AcquireResult::SUCCESS:
            // Lock acquired successfully
            break;
            
        case LockManager::AcquireResult::WOULD_DEADLOCK:
            // Would cause a deadlock
            throw DeadlockException("Acquiring lock would cause a deadlock", 
                                   {resource_id_});
            
        case LockManager::AcquireResult::TIMEOUT:
            // Timed out
            throw TimeoutException("Timed out acquiring lock", resource_id_);
            
        case LockManager::AcquireResult::ALREADY_HELD:
            // Already held by this thread
            throw LockOperationException("Lock already held by this thread");
    }
}

LockGuard::~LockGuard() {
    // Release the lock if it hasn't been released yet
    if (!released_) {
        release();
    }
}

bool LockGuard::upgrade(uint64_t timeout_ms) {
    // Check if the lock is already released
    if (released_) {
        throw LockOperationException("Cannot upgrade a released lock");
    }
    
    // Check if the lock is already exclusive
    if (lock_type_ == LockManager::LockType::EXCLUSIVE) {
        return true;
    }
    
    // Check if the lock is upgradable
    if (lock_type_ != LockManager::LockType::UPGRADABLE) {
        throw LockOperationException("Cannot upgrade a non-upgradable lock");
    }
    
    // Try to upgrade the lock
    auto result = lock_manager_.upgrade_lock(resource_id_, timeout_ms);
    
    // Check the result
    switch (result) {
        case LockManager::AcquireResult::SUCCESS:
            // Lock upgraded successfully
            lock_type_ = LockManager::LockType::EXCLUSIVE;
            return true;
            
        case LockManager::AcquireResult::WOULD_DEADLOCK:
            // Would cause a deadlock
            throw DeadlockException("Upgrading lock would cause a deadlock", 
                                   {resource_id_});
            
        case LockManager::AcquireResult::TIMEOUT:
            // Timed out
            throw TimeoutException("Timed out upgrading lock", resource_id_);
            
        case LockManager::AcquireResult::ALREADY_HELD:
            // Already held exclusively by this thread
            lock_type_ = LockManager::LockType::EXCLUSIVE;
            return true;
    }
    
    return false;
}

void LockGuard::release() {
    // Check if the lock is already released
    if (released_) {
        return;
    }
    
    // Release the lock
    lock_manager_.release_lock(resource_id_);
    released_ = true;
}

uint64_t LockGuard::resource_id() const {
    return resource_id_;
}

LockManager::LockType LockGuard::lock_type() const {
    return lock_type_;
}

bool LockGuard::is_released() const {
    return released_;
}

} // namespace bitscrape::lock
