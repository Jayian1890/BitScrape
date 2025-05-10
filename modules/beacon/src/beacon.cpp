#include "bitscrape/beacon/beacon.hpp"

#include <vector>
#include <future>

namespace bitscrape::beacon {

Beacon::Beacon() = default;

Beacon::~Beacon() = default;

void Beacon::add_sink(std::unique_ptr<BeaconSink> sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(std::move(sink));
}

void Beacon::debug(const std::string& message, types::BeaconCategory category,
                  const std::source_location& location) {
    log(types::BeaconSeverity::DEBUG, message, category, location);
}

void Beacon::info(const std::string& message, types::BeaconCategory category,
                 const std::source_location& location) {
    log(types::BeaconSeverity::INFO, message, category, location);
}

void Beacon::warning(const std::string& message, types::BeaconCategory category,
                    const std::source_location& location) {
    log(types::BeaconSeverity::WARNING, message, category, location);
}

void Beacon::error(const std::string& message, types::BeaconCategory category,
                  const std::source_location& location) {
    log(types::BeaconSeverity::ERROR, message, category, location);
}

void Beacon::critical(const std::string& message, types::BeaconCategory category,
                     const std::source_location& location) {
    log(types::BeaconSeverity::CRITICAL, message, category, location);
}

std::future<void> Beacon::debug_async(const std::string& message, types::BeaconCategory category,
                                     const std::source_location& location) {
    return log_async(types::BeaconSeverity::DEBUG, message, category, location);
}

std::future<void> Beacon::info_async(const std::string& message, types::BeaconCategory category,
                                    const std::source_location& location) {
    return log_async(types::BeaconSeverity::INFO, message, category, location);
}

std::future<void> Beacon::warning_async(const std::string& message, types::BeaconCategory category,
                                       const std::source_location& location) {
    return log_async(types::BeaconSeverity::WARNING, message, category, location);
}

std::future<void> Beacon::error_async(const std::string& message, types::BeaconCategory category,
                                     const std::source_location& location) {
    return log_async(types::BeaconSeverity::ERROR, message, category, location);
}

std::future<void> Beacon::critical_async(const std::string& message, types::BeaconCategory category,
                                        const std::source_location& location) {
    return log_async(types::BeaconSeverity::CRITICAL, message, category, location);
}

void Beacon::log(types::BeaconSeverity severity, const std::string& message, 
                types::BeaconCategory category, const std::source_location& location) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Write to all sinks
    for (auto& sink : sinks_) {
        sink->write(severity, category, message, location);
    }
}

std::future<void> Beacon::log_async(types::BeaconSeverity severity, const std::string& message, 
                                   types::BeaconCategory category, const std::source_location& location) {
    return std::async(std::launch::async, [this, severity, message, category, location]() {
        this->log(severity, message, category, location);
    });
}

std::unique_ptr<Beacon> create_beacon() {
    return std::make_unique<Beacon>();
}

} // namespace bitscrape::beacon
