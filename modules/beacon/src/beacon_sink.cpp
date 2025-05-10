#include "bitscrape/beacon/beacon_sink.hpp"

#include <algorithm>

namespace bitscrape::beacon {

std::future<void> BeaconSink::write_async(types::BeaconSeverity severity, types::BeaconCategory category, 
                                        const std::string& message, const std::source_location& location) {
    return std::async(std::launch::async, [this, severity, category, message, location]() {
        this->write(severity, category, message, location);
    });
}

void BeaconSink::set_min_severity(types::BeaconSeverity min_severity) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_severity_ = min_severity;
}

void BeaconSink::set_categories(const std::vector<types::BeaconCategory>& categories) {
    std::lock_guard<std::mutex> lock(mutex_);
    categories_ = categories;
    filter_categories_ = true;
}

void BeaconSink::clear_category_filter() {
    std::lock_guard<std::mutex> lock(mutex_);
    categories_.clear();
    filter_categories_ = false;
}

bool BeaconSink::should_log(types::BeaconSeverity severity, types::BeaconCategory category) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check severity level
    if (severity < min_severity_) {
        return false;
    }
    
    // Check category filter
    if (filter_categories_) {
        return std::find(categories_.begin(), categories_.end(), category) != categories_.end();
    }
    
    return true;
}

} // namespace bitscrape::beacon
