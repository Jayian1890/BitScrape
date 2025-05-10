#include "bitscrape/tracker/tracker_request.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace bitscrape::tracker {

namespace {

// URL encode a string
std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }

    return escaped.str();
}

// URL encode binary data
std::string url_encode_binary(const std::vector<uint8_t>& data) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (uint8_t byte : data) {
        escaped << '%' << std::setw(2) << static_cast<int>(byte);
    }

    return escaped.str();
}

} // namespace

// TrackerRequest implementation
TrackerRequest::TrackerRequest(std::string url)
    : url_(std::move(url)) {
}

const std::string& TrackerRequest::url() const {
    return url_;
}

void TrackerRequest::set_url(const std::string& url) {
    url_ = url;
}

std::string TrackerRequest::build_url() const {
    std::ostringstream url_stream;
    url_stream << url_;

    // Add query separator if needed
    if (url_.find('?') == std::string::npos) {
        url_stream << '?';
    } else if (url_.back() != '?' && url_.back() != '&') {
        url_stream << '&';
    }

    // Add parameters
    auto params = parameters();
    bool first = true;
    for (const auto& [key, value] : params) {
        if (!first) {
            url_stream << '&';
        }
        url_stream << key << '=' << value;
        first = false;
    }

    return url_stream.str();
}

// AnnounceRequest implementation
AnnounceRequest::AnnounceRequest(
    const std::string& url,
    const types::InfoHash& info_hash,
    const std::string& peer_id,
    uint16_t port,
    uint64_t uploaded,
    uint64_t downloaded,
    uint64_t left,
    const std::string& event,
    const std::string& ip,
    int numwant,
    const std::string& key,
    const std::string& tracker_id
)
    : TrackerRequest(url),
      info_hash_(info_hash),
      peer_id_(peer_id),
      port_(port),
      uploaded_(uploaded),
      downloaded_(downloaded),
      left_(left),
      event_(event),
      ip_(ip),
      numwant_(numwant),
      key_(key),
      tracker_id_(tracker_id) {
}

const types::InfoHash& AnnounceRequest::info_hash() const {
    return info_hash_;
}

const std::string& AnnounceRequest::peer_id() const {
    return peer_id_;
}

uint16_t AnnounceRequest::port() const {
    return port_;
}

uint64_t AnnounceRequest::uploaded() const {
    return uploaded_;
}

uint64_t AnnounceRequest::downloaded() const {
    return downloaded_;
}

uint64_t AnnounceRequest::left() const {
    return left_;
}

const std::string& AnnounceRequest::event() const {
    return event_;
}

const std::string& AnnounceRequest::ip() const {
    return ip_;
}

int AnnounceRequest::numwant() const {
    return numwant_;
}

const std::string& AnnounceRequest::key() const {
    return key_;
}

const std::string& AnnounceRequest::tracker_id() const {
    return tracker_id_;
}

std::map<std::string, std::string> AnnounceRequest::parameters() const {
    std::map<std::string, std::string> params;

    // Required parameters
    params["info_hash"] = url_encode_binary(std::vector<uint8_t>(info_hash_.bytes().begin(), info_hash_.bytes().end()));
    params["peer_id"] = url_encode(peer_id_);
    params["port"] = std::to_string(port_);
    params["uploaded"] = std::to_string(uploaded_);
    params["downloaded"] = std::to_string(downloaded_);
    params["left"] = std::to_string(left_);

    // Optional parameters
    if (!event_.empty()) {
        params["event"] = event_;
    }
    if (!ip_.empty()) {
        params["ip"] = ip_;
    }
    if (numwant_ >= 0) {
        params["numwant"] = std::to_string(numwant_);
    }
    if (!key_.empty()) {
        params["key"] = key_;
    }
    if (!tracker_id_.empty()) {
        params["trackerid"] = tracker_id_;
    }

    // Add compact=1 parameter for efficiency
    params["compact"] = "1";

    return params;
}

// ScrapeRequest implementation
ScrapeRequest::ScrapeRequest(
    const std::string& url,
    const std::vector<types::InfoHash>& info_hashes
)
    : TrackerRequest(url),
      info_hashes_(info_hashes) {
}

const std::vector<types::InfoHash>& ScrapeRequest::info_hashes() const {
    return info_hashes_;
}

std::map<std::string, std::string> ScrapeRequest::parameters() const {
    std::map<std::string, std::string> params;

    // Add info_hash parameters
    for (size_t i = 0; i < info_hashes_.size(); ++i) {
        std::string param_name = "info_hash";
        if (i > 0) {
            param_name += std::to_string(i);
        }
        params[param_name] = url_encode_binary(std::vector<uint8_t>(info_hashes_[i].bytes().begin(), info_hashes_[i].bytes().end()));
    }

    return params;
}

} // namespace bitscrape::tracker
