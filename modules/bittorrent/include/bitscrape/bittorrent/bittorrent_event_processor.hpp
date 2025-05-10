#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <future>
#include <mutex>
#include <atomic>
#include <unordered_map>

#include "bitscrape/event/event_processor.hpp"
#include "bitscrape/types/event_types.hpp"
#include "bitscrape/types/info_hash.hpp"
#include "bitscrape/bittorrent/peer_manager.hpp"
#include "bitscrape/bittorrent/metadata_exchange.hpp"

namespace bitscrape::bittorrent {

/**
 * @brief Event types for BitTorrent events
 */
enum class BitTorrentEventType : uint16_t {
    PEER_DISCOVERED = 1000,
    PEER_CONNECTED = 1001,
    PEER_DISCONNECTED = 1002,
    METADATA_RECEIVED = 1003,
    METADATA_REQUESTED = 1004,
    METADATA_PIECE_RECEIVED = 1005,
    METADATA_PIECE_REQUESTED = 1006,
    METADATA_PIECE_SENT = 1007,
    METADATA_REJECTED = 1008
};

/**
 * @brief Base class for BitTorrent events
 */
class BitTorrentEvent : public types::Event {
public:
    /**
     * @brief Constructor
     *
     * @param type BitTorrent event type
     */
    explicit BitTorrentEvent(BitTorrentEventType type);

    /**
     * @brief Get the BitTorrent event type
     *
     * @return BitTorrent event type
     */
    [[nodiscard]] BitTorrentEventType bittorrent_event_type() const;

    /**
     * @brief Clone the event
     *
     * @return A unique pointer to a clone of this event
     */
    [[nodiscard]] std::unique_ptr<types::Event> clone() const override = 0;
};

/**
 * @brief Event for peer discovery
 */
class PeerDiscoveredEvent : public BitTorrentEvent {
public:
    /**
     * @brief Constructor
     *
     * @param info_hash Torrent info hash
     * @param address Peer address
     */
    PeerDiscoveredEvent(types::InfoHash info_hash, network::Address address);

    /**
     * @brief Get the info hash
     *
     * @return Info hash
     */
    [[nodiscard]] const types::InfoHash& info_hash() const;

    /**
     * @brief Get the peer address
     *
     * @return Peer address
     */
    [[nodiscard]] const network::Address& address() const;

    /**
     * @brief Get a string representation of the event
     *
     * @return String representation
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief Clone the event
     *
     * @return A unique pointer to a clone of this event
     */
    [[nodiscard]] std::unique_ptr<types::Event> clone() const override;

private:
    types::InfoHash info_hash_; ///< Torrent info hash
    network::Address address_;  ///< Peer address
};

/**
 * @brief Event for metadata reception
 */
class MetadataReceivedEvent : public BitTorrentEvent {
public:
    /**
     * @brief Constructor
     *
     * @param info_hash Torrent info hash
     * @param metadata Metadata info
     */
    MetadataReceivedEvent(types::InfoHash info_hash, types::MetadataInfo metadata);

    /**
     * @brief Get the info hash
     *
     * @return Info hash
     */
    [[nodiscard]] const types::InfoHash& info_hash() const;

    /**
     * @brief Get the metadata info
     *
     * @return Metadata info
     */
    [[nodiscard]] const types::MetadataInfo& metadata() const;

    /**
     * @brief Get a string representation of the event
     *
     * @return String representation
     */
    [[nodiscard]] std::string to_string() const override;

    /**
     * @brief Clone the event
     *
     * @return A unique pointer to a clone of this event
     */
    [[nodiscard]] std::unique_ptr<types::Event> clone() const override;

private:
    types::InfoHash info_hash_;       ///< Torrent info hash
    types::MetadataInfo metadata_;    ///< Metadata info
};

/**
 * @brief Processes BitTorrent events
 */
class BitTorrentEventProcessor : public event::EventProcessor {
public:
    // Delete copy and move operations
    BitTorrentEventProcessor(const BitTorrentEventProcessor&) = delete;
    BitTorrentEventProcessor& operator=(const BitTorrentEventProcessor&) = delete;
    BitTorrentEventProcessor(BitTorrentEventProcessor&&) = delete;
    BitTorrentEventProcessor& operator=(BitTorrentEventProcessor&&) = delete;
    /**
     * @brief Constructor
     */
    BitTorrentEventProcessor();

    /**
     * @brief Destructor
     */
    ~BitTorrentEventProcessor() override;

    /**
     * @brief Start the event processor
     *
     * @param event_bus Event bus to use
     */
    void start(event::EventBus& event_bus) override;

    /**
     * @brief Stop the event processor
     */
    void stop() override;

    /**
     * @brief Process an event
     *
     * @param event Event to process
     */
    void process(const types::Event& event) override;

    /**
     * @brief Process an event asynchronously
     *
     * @param event Event to process
     * @return Future that will be completed when the event has been processed
     */
    std::future<void> process_async(const types::Event& event) override;

    /**
     * @brief Check if the processor is running
     *
     * @return true if the processor is running, false otherwise
     */
    bool is_running() const override;

    /**
     * @brief Process a BitTorrent event
     *
     * @param event Event to process
     * @return True if the event was processed, false otherwise
     */
    bool process_event(const types::Event& event);

    /**
     * @brief Add a peer manager
     *
     * @param info_hash Torrent info hash
     * @param peer_manager Peer manager
     */
    void add_peer_manager(const types::InfoHash& info_hash, std::shared_ptr<PeerManager> peer_manager);

    /**
     * @brief Remove a peer manager
     *
     * @param info_hash Torrent info hash
     */
    void remove_peer_manager(const types::InfoHash& info_hash);

    /**
     * @brief Add a metadata exchange
     *
     * @param info_hash Torrent info hash
     * @param metadata_exchange Metadata exchange
     */
    void add_metadata_exchange(const types::InfoHash& info_hash, std::shared_ptr<MetadataExchange> metadata_exchange);

    /**
     * @brief Remove a metadata exchange
     *
     * @param info_hash Torrent info hash
     */
    void remove_metadata_exchange(const types::InfoHash& info_hash);

private:
    /**
     * @brief Handle a peer discovered event
     *
     * @param event Peer discovered event
     */
    void handle_peer_discovered(const PeerDiscoveredEvent& event);

    /**
     * @brief Handle a metadata received event
     *
     * @param event Metadata received event
     */
    void handle_metadata_received(const MetadataReceivedEvent& event);

    std::unordered_map<std::string, std::shared_ptr<PeerManager>> peer_managers_; ///< Peer managers by info hash
    mutable std::mutex peer_managers_mutex_; ///< Mutex for peer managers map

    std::unordered_map<std::string, std::shared_ptr<MetadataExchange>> metadata_exchanges_; ///< Metadata exchanges by info hash
    mutable std::mutex metadata_exchanges_mutex_; ///< Mutex for metadata exchanges map

    event::EventBus* event_bus_; ///< Event bus
    types::SubscriptionToken token_;    ///< Event subscription token
    std::atomic<bool> running_;  ///< Whether the processor is running
};

} // namespace bitscrape::bittorrent
