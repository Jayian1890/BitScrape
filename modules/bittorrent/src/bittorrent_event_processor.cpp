#include "bitscrape/bittorrent/bittorrent_event_processor.hpp"

#include <future>

namespace bitscrape::bittorrent {

// BitTorrentEvent implementation

BitTorrentEvent::BitTorrentEvent(BitTorrentEventType type)
    : types::Event(types::Event::Type::USER_DEFINED, static_cast<uint32_t>(type)) {
}

BitTorrentEventType BitTorrentEvent::bittorrent_event_type() const {
    return static_cast<BitTorrentEventType>(custom_type_id());
}

// PeerDiscoveredEvent implementation

PeerDiscoveredEvent::PeerDiscoveredEvent(types::InfoHash info_hash, network::Address address)
    : BitTorrentEvent(BitTorrentEventType::PEER_DISCOVERED),
      info_hash_(info_hash),
      address_(std::move(address)) {
}

const types::InfoHash& PeerDiscoveredEvent::info_hash() const {
    return info_hash_;
}

const network::Address& PeerDiscoveredEvent::address() const {
    return address_;
}

std::string PeerDiscoveredEvent::to_string() const {
    return "PeerDiscoveredEvent[info_hash=" + info_hash_.to_hex() + ", address=" + address_.to_string() + "]";
}

std::unique_ptr<types::Event> PeerDiscoveredEvent::clone() const {
    return std::make_unique<PeerDiscoveredEvent>(info_hash_, address_);
}

// MetadataReceivedEvent implementation

MetadataReceivedEvent::MetadataReceivedEvent(types::InfoHash info_hash, types::MetadataInfo metadata)
    : BitTorrentEvent(BitTorrentEventType::METADATA_RECEIVED),
      info_hash_(info_hash),
      metadata_(std::move(metadata)) {
}

const types::InfoHash& MetadataReceivedEvent::info_hash() const {
    return info_hash_;
}

const types::MetadataInfo& MetadataReceivedEvent::metadata() const {
    return metadata_;
}

std::string MetadataReceivedEvent::to_string() const {
    return "MetadataReceivedEvent[info_hash=" + info_hash_.to_hex() + "]";
}

std::unique_ptr<types::Event> MetadataReceivedEvent::clone() const {
    return std::make_unique<MetadataReceivedEvent>(info_hash_, metadata_);
}

// BitTorrentEventProcessor implementation

BitTorrentEventProcessor::BitTorrentEventProcessor()
    : event_bus_(nullptr),
      token_(0),
      running_(false) {
}

BitTorrentEventProcessor::~BitTorrentEventProcessor() {
    stop();
}

void BitTorrentEventProcessor::start(event::EventBus& event_bus) {
    // Check if already running
    if (running_) {
        return;
    }

    // Store event bus
    event_bus_ = &event_bus;

    // Subscribe to events
    token_ = event_bus_->subscribe<types::Event>([this](const types::Event& event) {
        // Process event
        this->process(event);
    });

    // Set running flag
    running_ = true;
}

void BitTorrentEventProcessor::stop() {
    // Check if already stopped
    if (!running_) {
        return;
    }

    // Set running flag
    running_ = false;

    // Unsubscribe from events
    if (event_bus_ != nullptr) {
        event_bus_->unsubscribe(token_);
        event_bus_ = nullptr;
    }
}

void BitTorrentEventProcessor::process(const types::Event& event) {
    if (!running_) {
        return;
    }

    // Process the event
    process_event(event);
}

std::future<void> BitTorrentEventProcessor::process_async(const types::Event& event) {
    return std::async(std::launch::async, [this, event = event.clone()]() {
        this->process(*event);
    });
}

bool BitTorrentEventProcessor::is_running() const {
    return running_;
}

bool BitTorrentEventProcessor::process_event(const types::Event& event) {
    // Check if event is a BitTorrent event
    if (event.custom_type_id() >= static_cast<uint32_t>(BitTorrentEventType::PEER_DISCOVERED) &&
        event.custom_type_id() <= static_cast<uint32_t>(BitTorrentEventType::METADATA_REJECTED)) {
        // Process based on event type
        auto event_type = static_cast<BitTorrentEventType>(event.custom_type_id());
        switch (event_type) {
            case BitTorrentEventType::PEER_DISCOVERED:
                if (const auto* peer_event = dynamic_cast<const PeerDiscoveredEvent*>(&event)) {
                    handle_peer_discovered(*peer_event);
                    return true;
                }
                break;

            case BitTorrentEventType::METADATA_RECEIVED:
                if (const auto* metadata_event = dynamic_cast<const MetadataReceivedEvent*>(&event)) {
                    handle_metadata_received(*metadata_event);
                    return true;
                }
                break;

            default:
                // Unhandled event type
                return false;
        }
    }

    return false;
}

void BitTorrentEventProcessor::add_peer_manager(const types::InfoHash& info_hash, std::shared_ptr<PeerManager> peer_manager) {
    std::lock_guard<std::mutex> lock(peer_managers_mutex_);

    // Add peer manager
    peer_managers_[info_hash.to_hex()] = std::move(peer_manager);
}

void BitTorrentEventProcessor::remove_peer_manager(const types::InfoHash& info_hash) {
    std::lock_guard<std::mutex> lock(peer_managers_mutex_);

    // Remove peer manager
    peer_managers_.erase(info_hash.to_hex());
}

void BitTorrentEventProcessor::add_metadata_exchange(const types::InfoHash& info_hash, std::shared_ptr<MetadataExchange> metadata_exchange) {
    std::lock_guard<std::mutex> lock(metadata_exchanges_mutex_);

    // Add metadata exchange
    metadata_exchanges_[info_hash.to_hex()] = std::move(metadata_exchange);
}

void BitTorrentEventProcessor::remove_metadata_exchange(const types::InfoHash& info_hash) {
    std::lock_guard<std::mutex> lock(metadata_exchanges_mutex_);

    // Remove metadata exchange
    metadata_exchanges_.erase(info_hash.to_hex());
}

void BitTorrentEventProcessor::handle_peer_discovered(const PeerDiscoveredEvent& event) {
    std::lock_guard<std::mutex> lock(peer_managers_mutex_);

    // Find peer manager
    auto it = peer_managers_.find(event.info_hash().to_hex());
    if (it != peer_managers_.end()) {
        // Add peer to manager
        it->second->add_peer(event.address());
    }
}

void BitTorrentEventProcessor::handle_metadata_received(const MetadataReceivedEvent& /* event */) {
    // TODO: Implement metadata received handling
}

} // namespace bitscrape::bittorrent
