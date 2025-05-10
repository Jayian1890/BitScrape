# BitScrape DHT Module

This module implements the BitTorrent Mainline DHT protocol, providing functionality for node discovery, peer lookup, and participation in the DHT network.

## Overview

The DHT (Distributed Hash Table) module implements the BitTorrent Mainline DHT protocol as specified in [BEP 5](http://www.bittorrent.org/beps/bep_0005.html). It provides functionality for:

- Joining and participating in the DHT network
- Finding nodes close to a target ID
- Looking up peers for a specific infohash
- Announcing as a peer for a specific infohash
- Managing a routing table of known nodes

The module is designed to be event-driven and fully asynchronous, with no blocking operations.

## Core Components

### DHT Messages

`DHTMessage` and its derived classes represent the different types of messages in the DHT protocol:

- `DHTPingMessage`: Simple ping message to check if a node is alive
- `DHTFindNodeMessage`: Request to find nodes close to a target ID
- `DHTGetPeersMessage`: Request to find peers for a specific infohash
- `DHTAnnouncePeerMessage`: Announce as a peer for a specific infohash

`DHTMessageFactory` provides factory methods for creating and parsing DHT messages.

### Routing Table

`RoutingTable` manages a collection of known nodes in the DHT network, organized into k-buckets based on the distance from the local node ID.

`KBucket` represents a k-bucket in the routing table, containing up to k nodes with a common prefix length.

### Node and Peer Lookup

`NodeLookup` implements the iterative node lookup algorithm for finding nodes close to a target ID.

`PeerLookup` implements the peer lookup algorithm for finding peers for a specific infohash.

### Bootstrap

`Bootstrap` manages the process of joining the DHT network by contacting known bootstrap nodes and performing initial node lookups.

### DHT Session

`DHTSession` is the main entry point for the DHT module, providing a high-level interface for DHT operations.

### Token Manager

`TokenManager` manages the generation and verification of tokens used in the announce_peer process.

## Usage

```cpp
// Create a DHT session with a random node ID
DHTSession session;

// Start the DHT session with bootstrap nodes
std::vector<Endpoint> bootstrap_nodes = {
    Endpoint("router.bittorrent.com", 6881),
    Endpoint("router.utorrent.com", 6881),
    Endpoint("dht.transmissionbt.com", 6881)
};
session.start(bootstrap_nodes);

// Find peers for an infohash
InfoHash infohash("0102030405060708090a0b0c0d0e0f1011121314");
auto future = session.find_peers_async(infohash);
auto peers = future.get();

// Announce as a peer for an infohash
session.announce_peer(infohash, 6881);

// Stop the DHT session
session.stop();
```

## Implementation Details

### Asynchronous Operations

All potentially blocking operations in the DHT module are implemented with both synchronous and asynchronous versions. The asynchronous versions use `std::async` and `std::future` to provide non-blocking operation.

### Event-Driven Architecture

The DHT module integrates with the event system to provide event-driven operation. Components can subscribe to DHT-related events and react accordingly.

### Thread Safety

All components in the DHT module are designed to be thread-safe, allowing concurrent access from multiple threads.

## References

- [BEP 5: DHT Protocol](http://www.bittorrent.org/beps/bep_0005.html)
- [Kademlia: A Peer-to-peer Information System Based on the XOR Metric](https://pdos.csail.mit.edu/~petar/papers/maymounkov-kademlia-lncs.pdf)
