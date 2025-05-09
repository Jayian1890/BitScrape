# BitScrape Types Module

This module provides the core types used throughout the BitScrape project.

## Overview

The types module defines the fundamental data types and interfaces used by other modules in the system. It provides a consistent type system that ensures type safety and clear semantics across the codebase.

## Core Types

### NodeID

`NodeID` represents a 160-bit identifier used in the DHT network to identify nodes. It provides methods for creating, comparing, and manipulating node IDs.

```cpp
// Create a random NodeID
NodeID id1;

// Create a NodeID from a hex string
NodeID id2("0102030405060708090a0b0c0d0e0f1011121314");

// Calculate the distance between two NodeIDs
NodeID distance = id1.distance(id2);

// Calculate the distance asynchronously
std::future<NodeID> future = id1.distance_async(id2);
NodeID distance = future.get();
```

### InfoHash

`InfoHash` represents a 160-bit identifier used in the BitTorrent network to identify torrents. It provides methods for creating, comparing, and manipulating info hashes.

```cpp
// Create an InfoHash from a hex string
InfoHash hash("0102030405060708090a0b0c0d0e0f1011121314");

// Generate a random InfoHash
InfoHash random = InfoHash::random();

// Calculate an InfoHash from bencode data
InfoHash hash = InfoHash::from_bencode(data);
```

### Endpoint

`Endpoint` encapsulates an IP address (IPv4 or IPv6) and a port number. It provides methods for creating, comparing, and manipulating endpoints.

```cpp
// Create an endpoint from an IP address and port
Endpoint ep1("192.168.1.1", 6881);

// Create an endpoint from a hostname and port
Endpoint ep2 = Endpoint::resolve("example.com", 6881);

// Resolve a hostname asynchronously
std::future<Endpoint> future = Endpoint::resolve_async("example.com", 6881);
Endpoint ep3 = future.get();
```

### Event Types

The `Event` class hierarchy provides the foundation for the event-driven architecture used throughout the system. It defines the base class for all events and provides mechanisms for event handling and subscription.

```cpp
// Create an event handler
EventHandler<MyEvent> handler = [](const MyEvent& event) {
    // Handle the event
};

// Subscribe to events
SubscriptionToken token = event_bus.subscribe<MyEvent>(handler);

// Unsubscribe from events
event_bus.unsubscribe(token);
```

### Message Types

The `Message` class hierarchy provides the foundation for all protocol messages used in the system. It defines the base class for all messages and provides mechanisms for serialization and deserialization.

```cpp
// Create a message
auto message = std::make_unique<MyMessage>();

// Serialize the message
std::vector<uint8_t> data = message->serialize();

// Deserialize a message
auto deserialized = MessageFactory::create(data);
```

## Dependencies

The types module has minimal dependencies:

- C++23 Standard Library
- No external libraries

## Building

The types module can be built using Meson:

```bash
meson setup builddir
meson compile -C builddir
```

## Testing

The types module includes comprehensive unit tests:

```bash
meson test -C builddir
```
