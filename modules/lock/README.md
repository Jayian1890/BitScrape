# Lock Manager Module

The Lock Manager module provides a comprehensive solution for managing locks in a multi-threaded environment, with a
focus on preventing deadlocks and improving thread safety.

## Features

- **Deadlock Prevention**: Enforces lock ordering and detects potential deadlocks
- **Timeout Support**: Prevents indefinite waiting on locks
- **Lock Types**: Supports exclusive, shared, and upgradable locks
- **RAII Support**: Provides RAII-based lock guards for automatic lock release
- **Diagnostic Tools**: Includes tools for debugging lock-related issues
- **Thread Safety**: Ensures thread-safe access to shared resources

## Components

### LockManager

The `LockManager` class is the core component of the module. It provides methods for registering resources, acquiring
and releasing locks, and detecting potential deadlocks.

```cpp
// Create a lock manager
auto lock_manager = std::make_shared<LockManager>();

// Register resources
uint64_t resource1_id = lock_manager->register_resource("resource1", LockPriority::NORMAL);
uint64_t resource2_id = lock_manager->register_resource("resource2", LockPriority::HIGH);

// Acquire a lock
auto result = lock_manager->acquire_lock(resource1_id, LockType::EXCLUSIVE);
if (result == LockManager::AcquireResult::SUCCESS) {
    // Lock acquired successfully
    // ...

    // Release the lock
    lock_manager->release_lock(resource1_id);
}
```

### LockGuard

The `LockGuard` class provides RAII-based lock management, automatically releasing locks when they go out of scope.

```cpp
// Create a lock guard
{
    auto guard = lock_manager->get_lock_guard(resource_id, LockType::EXCLUSIVE);

    // Lock is held here
    // ...

    // Optionally release the lock early
    guard->release();
}
// Lock is automatically released if not released earlier
```

### Lock Types

The module supports three types of locks:

- **Exclusive**: Only one thread can hold an exclusive lock on a resource
- **Shared**: Multiple threads can hold shared locks on a resource
- **Upgradable**: A shared lock that can be upgraded to an exclusive lock

```cpp
// Acquire an exclusive lock
auto result1 = lock_manager->acquire_lock(resource_id, LockType::EXCLUSIVE);

// Acquire a shared lock
auto result2 = lock_manager->acquire_lock(resource_id, LockType::SHARED);

// Acquire an upgradable lock
auto result3 = lock_manager->acquire_lock(resource_id, LockType::UPGRADABLE);

// Upgrade a lock
auto result4 = lock_manager->upgrade_lock(resource_id);
```

### Deadlock Detection

The module includes deadlock detection capabilities to prevent deadlocks before they occur.

```cpp
// Check if acquiring a lock would cause a deadlock
if (lock_manager->would_deadlock(resource_id)) {
    // Handle potential deadlock
    // ...
}
```

### Diagnostic Tools

The module provides diagnostic tools for debugging lock-related issues.

```cpp
// Dump the current lock state
std::string state = lock_manager->dump_lock_state();
std::cout << state << std::endl;

// Get the current thread's lock stack
auto lock_stack = lock_manager->get_lock_stack();
```

## Integration

To integrate the Lock Manager module with your code:

1. Use the provided `LockManagerSingleton` to access the shared `LockManager` instance
2. Register all resources that need thread-safe access
3. Use the `LockManager` to acquire and release locks
4. Use `LockGuard` for RAII-based lock management

```cpp
// Get the LockManager instance with a beacon for logging
auto beacon = std::make_shared<beacon::Beacon>();
beacon->add_sink(std::make_unique<beacon::ConsoleSink>());

auto lock_manager = LockManagerSingleton::instance(beacon, true, true);

// Or simply get the instance without a beacon
auto lock_manager = LockManagerSingleton::instance();

// Register resources
uint64_t resource_id = lock_manager->register_resource("my_resource");

// Use the lock manager
auto guard = lock_manager->get_lock_guard(resource_id);
```

### LockManagerSingleton

The `LockManagerSingleton` class provides a global access point to a shared `LockManager` instance. It ensures that only
one instance of the `LockManager` exists throughout the application.

```cpp
// Get the LockManager instance
auto lock_manager = LockManagerSingleton::instance();

// Get the LockManager instance with a beacon for logging
auto lock_manager = LockManagerSingleton::instance(beacon, true, true);

// Reset the LockManager instance (for testing purposes)
LockManagerSingleton::reset();
```

## Best Practices

- Always acquire locks in order of priority (higher priority first)
- Use `LockGuard` instead of manually acquiring and releasing locks
- Set appropriate timeouts to prevent indefinite waiting
- Use shared locks when possible to improve concurrency
- Enable deadlock detection in development and testing environments
