#pragma once

#include "bitscrape/tracker/tracker_request.hpp"
#include "bitscrape/tracker/tracker_response.hpp"
#include "bitscrape/network/udp_socket.hpp"
#include "bitscrape/network/address.hpp"
#include "bitscrape/network/buffer.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <future>
#include <mutex>
#include <atomic>
#include <chrono>

namespace bitscrape::tracker {

/**
 * @brief UDP tracker implementation (BEP 15)
 * 
 * This class provides functionality for communicating with UDP trackers.
 */
class UDPTracker {
public:
    /**
     * @brief Constructor
     * 
     * @param url Tracker URL
     */
    explicit UDPTracker(std::string url);
    
    /**
     * @brief Destructor
     */
    ~UDPTracker();
    
    /**
     * @brief UDPTracker is non-copyable
     */
    UDPTracker(const UDPTracker&) = delete;
    UDPTracker& operator=(const UDPTracker&) = delete;
    
    /**
     * @brief UDPTracker is movable
     */
    UDPTracker(UDPTracker&&) noexcept = delete;
    UDPTracker& operator=(UDPTracker&&) noexcept = delete;
    
    /**
     * @brief Get the tracker URL
     * 
     * @return Tracker URL
     */
    [[nodiscard]] const std::string& url() const;
    
    /**
     * @brief Set the tracker URL
     * 
     * @param url Tracker URL
     */
    void set_url(const std::string& url);
    
    /**
     * @brief Send an announce request to the tracker
     * 
     * @param request Announce request
     * @return Announce response
     */
    AnnounceResponse announce(const AnnounceRequest& request);
    
    /**
     * @brief Send an announce request to the tracker asynchronously
     * 
     * @param request Announce request
     * @return Future containing the announce response
     */
    std::future<AnnounceResponse> announce_async(const AnnounceRequest& request);
    
    /**
     * @brief Send a scrape request to the tracker
     * 
     * @param request Scrape request
     * @return Scrape response
     */
    ScrapeResponse scrape(const ScrapeRequest& request);
    
    /**
     * @brief Send a scrape request to the tracker asynchronously
     * 
     * @param request Scrape request
     * @return Future containing the scrape response
     */
    std::future<ScrapeResponse> scrape_async(const ScrapeRequest& request);
    
    /**
     * @brief Set the connection timeout
     * 
     * @param timeout_ms Timeout in milliseconds
     */
    void set_connection_timeout(int timeout_ms);
    
    /**
     * @brief Set the request timeout
     * 
     * @param timeout_ms Timeout in milliseconds
     */
    void set_request_timeout(int timeout_ms);
    
private:
    /**
     * @brief Action types for UDP tracker protocol
     */
    enum class Action : uint32_t {
        CONNECT = 0,
        ANNOUNCE = 1,
        SCRAPE = 2,
        ERROR = 3
    };
    
    /**
     * @brief Connect to the tracker
     * 
     * @return Connection ID
     */
    uint64_t connect();
    
    /**
     * @brief Connect to the tracker asynchronously
     * 
     * @return Future containing the connection ID
     */
    std::future<uint64_t> connect_async();
    
    /**
     * @brief Send a UDP packet to the tracker
     * 
     * @param buffer Data to send
     * @return Number of bytes sent
     */
    int send_packet(const network::Buffer& buffer);
    
    /**
     * @brief Receive a UDP packet from the tracker
     * 
     * @param buffer Buffer to receive into
     * @param timeout_ms Timeout in milliseconds
     * @return Number of bytes received
     */
    int receive_packet(network::Buffer& buffer, int timeout_ms);
    
    /**
     * @brief Parse a URL into host and port
     * 
     * @param url URL to parse
     * @return Pair of host and port
     */
    static std::pair<std::string, uint16_t> parse_url(const std::string& url);
    
    std::string url_;                          ///< Tracker URL
    network::Address address_;                 ///< Tracker address
    std::unique_ptr<network::UDPSocket> socket_; ///< UDP socket
    
    uint64_t connection_id_;                   ///< Connection ID
    std::chrono::steady_clock::time_point connection_time_; ///< Time of last connection
    std::mutex connection_mutex_;              ///< Mutex for connection ID
    
    int connection_timeout_ms_;                ///< Connection timeout in milliseconds
    int request_timeout_ms_;                   ///< Request timeout in milliseconds
    
    std::atomic<uint32_t> transaction_id_;     ///< Transaction ID counter
};

} // namespace bitscrape::tracker
