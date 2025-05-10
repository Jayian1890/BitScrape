#include "bitscrape/tracker/http_tracker.hpp"
#include "bitscrape/bencode/bencode_decoder.hpp"
#include "bitscrape/bencode/bencode_value.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace bitscrape::tracker {

HTTPTracker::HTTPTracker(std::string url)
    : url_(std::move(url)),
      decoder_(bencode::create_bencode_decoder()) {
    // Ensure URL ends with a slash if it doesn't have query parameters
    if (url_.find('?') == std::string::npos && url_.back() != '/') {
        url_ += '/';
    }

    // Set reasonable defaults for HTTP client
    http_client_.set_connection_timeout(30000);  // 30 seconds
    http_client_.set_request_timeout(30000);     // 30 seconds
    http_client_.set_follow_redirects(true);
    http_client_.set_max_redirects(5);
}

const std::string& HTTPTracker::url() const {
    return url_;
}

void HTTPTracker::set_url(const std::string& url) {
    url_ = url;

    // Ensure URL ends with a slash if it doesn't have query parameters
    if (url_.find('?') == std::string::npos && url_.back() != '/') {
        url_ += '/';
    }
}

AnnounceResponse HTTPTracker::announce(const AnnounceRequest& request) {
    try {
        // Build the full URL with parameters
        std::string full_url = request.build_url();

        // Send the HTTP request
        network::HTTPResponse http_response = http_client_.get(full_url);

        // Check for HTTP errors
        if (http_response.status_code != 200) {
            AnnounceResponse response;
            response.set_error_message("HTTP error: " + std::to_string(http_response.status_code));
            return response;
        }

        // Parse the response
        return parse_announce_response(http_response.body);
    } catch (const std::exception& e) {
        AnnounceResponse response;
        response.set_error_message("Exception: " + std::string(e.what()));
        return response;
    }
}

std::future<AnnounceResponse> HTTPTracker::announce_async(const AnnounceRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return announce(request);
    });
}

ScrapeResponse HTTPTracker::scrape(const ScrapeRequest& request) {
    try {
        // Build the full URL with parameters
        std::string full_url = request.build_url();

        // Send the HTTP request
        network::HTTPResponse http_response = http_client_.get(full_url);

        // Check for HTTP errors
        if (http_response.status_code != 200) {
            ScrapeResponse response;
            response.set_error_message("HTTP error: " + std::to_string(http_response.status_code));
            return response;
        }

        // Parse the response
        return parse_scrape_response(http_response.body);
    } catch (const std::exception& e) {
        ScrapeResponse response;
        response.set_error_message("Exception: " + std::string(e.what()));
        return response;
    }
}

std::future<ScrapeResponse> HTTPTracker::scrape_async(const ScrapeRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return scrape(request);
    });
}

void HTTPTracker::set_connection_timeout(int timeout_ms) {
    http_client_.set_connection_timeout(timeout_ms);
}

void HTTPTracker::set_request_timeout(int timeout_ms) {
    http_client_.set_request_timeout(timeout_ms);
}

AnnounceResponse HTTPTracker::parse_announce_response(const network::Buffer& data) {
    AnnounceResponse response;

    try {
        // Decode the bencode data
        bencode::BencodeValue value = decoder_->decode(data.to_vector());

        // Check if the response is a dictionary
        if (!value.is_dict()) {
            response.set_error_message("Invalid response: not a dictionary");
            return response;
        }

        const auto& dict = value.as_dict();

        // Check for failure
        auto failure_it = dict.find("failure reason");
        if (failure_it != dict.end() && failure_it->second.is_string()) {
            response.set_error_message(failure_it->second.as_string());
            return response;
        }

        // Check for warning
        auto warning_it = dict.find("warning message");
        if (warning_it != dict.end() && warning_it->second.is_string()) {
            response.set_warning_message(warning_it->second.as_string());
        }

        // Parse interval
        auto interval_it = dict.find("interval");
        if (interval_it != dict.end() && interval_it->second.is_integer()) {
            response.set_interval(static_cast<int>(interval_it->second.as_integer()));
        }

        // Parse min interval
        auto min_interval_it = dict.find("min interval");
        if (min_interval_it != dict.end() && min_interval_it->second.is_integer()) {
            response.set_min_interval(static_cast<int>(min_interval_it->second.as_integer()));
        }

        // Parse tracker ID
        auto tracker_id_it = dict.find("tracker id");
        if (tracker_id_it != dict.end() && tracker_id_it->second.is_string()) {
            response.set_tracker_id(tracker_id_it->second.as_string());
        }

        // Parse complete
        auto complete_it = dict.find("complete");
        if (complete_it != dict.end() && complete_it->second.is_integer()) {
            response.set_complete(static_cast<int>(complete_it->second.as_integer()));
        }

        // Parse incomplete
        auto incomplete_it = dict.find("incomplete");
        if (incomplete_it != dict.end() && incomplete_it->second.is_integer()) {
            response.set_incomplete(static_cast<int>(incomplete_it->second.as_integer()));
        }

        // Parse peers
        auto peers_it = dict.find("peers");
        if (peers_it != dict.end()) {
            if (peers_it->second.is_string()) {
                // Binary model: string of 6 bytes per peer (4 for IP, 2 for port)
                const std::string& peers_data = peers_it->second.as_string();

                // Each peer is 6 bytes
                if (peers_data.size() % 6 == 0) {
                    for (size_t i = 0; i < peers_data.size(); i += 6) {
                        // Extract IP (4 bytes)
                        std::ostringstream ip_stream;
                        ip_stream << static_cast<int>(static_cast<uint8_t>(peers_data[i])) << "."
                                 << static_cast<int>(static_cast<uint8_t>(peers_data[i + 1])) << "."
                                 << static_cast<int>(static_cast<uint8_t>(peers_data[i + 2])) << "."
                                 << static_cast<int>(static_cast<uint8_t>(peers_data[i + 3]));

                        // Extract port (2 bytes, big-endian)
                        uint16_t port = (static_cast<uint16_t>(static_cast<uint8_t>(peers_data[i + 4])) << 8) |
                                       static_cast<uint16_t>(static_cast<uint8_t>(peers_data[i + 5]));

                        // Add peer
                        response.add_peer(network::Address(ip_stream.str(), port));
                    }
                }
            } else if (peers_it->second.is_list()) {
                // Dictionary model: list of dictionaries
                const auto& peers_list = peers_it->second.as_list();

                for (const auto& peer : peers_list) {
                    if (peer.is_dict()) {
                        const auto& peer_dict = peer.as_dict();

                        auto ip_it = peer_dict.find("ip");
                        auto port_it = peer_dict.find("port");

                        if (ip_it != peer_dict.end() && ip_it->second.is_string() &&
                            port_it != peer_dict.end() && port_it->second.is_integer()) {
                            response.add_peer(network::Address(
                                ip_it->second.as_string(),
                                static_cast<uint16_t>(port_it->second.as_integer())
                            ));
                        }
                    }
                }
            }
        }

        return response;
    } catch (const std::exception& e) {
        response.set_error_message("Exception: " + std::string(e.what()));
        return response;
    }
}

ScrapeResponse HTTPTracker::parse_scrape_response(const network::Buffer& data) {
    ScrapeResponse response;

    try {
        // Decode the bencode data
        bencode::BencodeValue value = decoder_->decode(data.to_vector());

        // Check if the response is a dictionary
        if (!value.is_dict()) {
            response.set_error_message("Invalid response: not a dictionary");
            return response;
        }

        const auto& dict = value.as_dict();

        // Check for failure
        auto failure_it = dict.find("failure reason");
        if (failure_it != dict.end() && failure_it->second.is_string()) {
            response.set_error_message(failure_it->second.as_string());
            return response;
        }

        // Parse files
        auto files_it = dict.find("files");
        if (files_it != dict.end() && files_it->second.is_dict()) {
            const auto& files_dict = files_it->second.as_dict();

            for (const auto& [hash_str, file_value] : files_dict) {
                if (file_value.is_dict()) {
                    const auto& file_dict = file_value.as_dict();

                    ScrapeResponse::ScrapeData data{};

                    // Parse complete
                    auto complete_it = file_dict.find("complete");
                    if (complete_it != file_dict.end() && complete_it->second.is_integer()) {
                        data.complete = static_cast<int>(complete_it->second.as_integer());
                    }

                    // Parse downloaded
                    auto downloaded_it = file_dict.find("downloaded");
                    if (downloaded_it != file_dict.end() && downloaded_it->second.is_integer()) {
                        data.downloaded = static_cast<int>(downloaded_it->second.as_integer());
                    }

                    // Parse incomplete
                    auto incomplete_it = file_dict.find("incomplete");
                    if (incomplete_it != file_dict.end() && incomplete_it->second.is_integer()) {
                        data.incomplete = static_cast<int>(incomplete_it->second.as_integer());
                    }

                    // Parse name
                    auto name_it = file_dict.find("name");
                    if (name_it != file_dict.end() && name_it->second.is_string()) {
                        data.name = name_it->second.as_string();
                    }

                    // Convert hash string to InfoHash
                    if (hash_str.size() == 20) {
                        std::vector<uint8_t> hash_bytes(hash_str.begin(), hash_str.end());
                        types::InfoHash info_hash(hash_bytes);
                        response.add_file(info_hash, data);
                    }
                }
            }
        }

        return response;
    } catch (const std::exception& e) {
        response.set_error_message("Exception: " + std::string(e.what()));
        return response;
    }
}

std::string HTTPTracker::announce_to_scrape_url(const std::string& announce_url) {
    // Find the last component of the URL path
    size_t last_slash = announce_url.find_last_of('/');
    if (last_slash == std::string::npos) {
        return announce_url;
    }

    // Find the query string
    size_t query_start = announce_url.find('?', last_slash);
    std::string query;
    if (query_start != std::string::npos) {
        query = announce_url.substr(query_start);
    }

    // Get the base URL (everything before the last component)
    std::string base_url = announce_url.substr(0, last_slash + 1);

    // Get the last component
    std::string last_component;
    if (query_start != std::string::npos) {
        last_component = announce_url.substr(last_slash + 1, query_start - last_slash - 1);
    } else {
        last_component = announce_url.substr(last_slash + 1);
    }

    // If the last component is "announce", replace it with "scrape"
    if (last_component == "announce") {
        return base_url + "scrape" + query;
    }

    // Otherwise, just return the original URL
    return announce_url;
}

} // namespace bitscrape::tracker
