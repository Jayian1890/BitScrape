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

## DHT-Specific Types

### DHTNode

`DHTNode` encapsulates a node in the DHT network, consisting of a NodeID and an Endpoint. It also tracks the node's status, last seen time, and other properties.

```cpp
// Create a node with an ID and endpoint
NodeID id("0102030405060708090a0b0c0d0e0f1011121314");
Endpoint ep("192.168.1.1", 6881);
DHTNode node(id, ep);

// Update the node status
node.set_status(DHTNode::Status::GOOD);

// Update the last seen time
node.update_last_seen();

// Calculate the distance to another node
NodeID distance = node.distance(other_node);
```

### DHTToken

`DHTToken` encapsulates a token used in the DHT protocol for verification. Tokens are used to prevent unauthorized announce_peer messages.

```cpp
// Create a random token
DHTToken token;

// Create a token from a byte vector
std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04, 0x05};
DHTToken token(bytes);

// Check if the token is expired
bool expired = token.is_expired(std::chrono::seconds(300));
```

### DHTRoutingTableEntry

`DHTRoutingTableEntry` encapsulates an entry in the DHT routing table, which consists of a k-bucket of nodes and a range of node IDs.

```cpp
// Create a routing table entry with a prefix length
DHTRoutingTableEntry entry(0);

// Add a node to the bucket
entry.add_node(node);

// Check if the bucket contains a node
bool contains = entry.contains_node(node);

// Get a node by ID
const DHTNode* node = entry.get_node(id);
```

## Metadata Types

### MetadataInfo

`MetadataInfo` encapsulates the metadata info dictionary from a torrent file, including the name, piece length, pieces, and other properties.

```cpp
// Create a metadata info from raw bencode data
MetadataInfo info(data);

// Set the metadata properties
info.set_name("example.torrent");
info.set_piece_length(16384);
info.set_total_size(1000000);

// Check if the metadata is valid
bool valid = info.is_valid();
```

### MetadataPiece

`MetadataPiece` encapsulates a piece of metadata during the metadata exchange protocol. It includes the piece index, data, and total size.

```cpp
// Create a metadata piece with an index, data, and total size
uint32_t index = 0;
std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
uint32_t total_size = 1000;
MetadataPiece piece(index, data, total_size);

// Check if the piece is valid
bool valid = piece.is_valid();
```

### TorrentInfo

`TorrentInfo` encapsulates the complete torrent information, including the metadata info, announce URLs, creation date, and other properties.

```cpp
// Create a torrent info from raw bencode data
TorrentInfo info(data);

// Create a torrent info from an info hash and metadata
TorrentInfo info(hash, metadata);

// Set the torrent properties
info.set_announce("http://tracker.example.com:6969/announce");
info.set_comment("Example torrent");

// Check if the torrent info is valid
bool valid = info.is_valid();
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
