#include "bitscrape/beacon/console_sink.hpp"

#include <sstream>
#include <iomanip>
#include <chrono>

namespace bitscrape::beacon {

ConsoleSink::ConsoleSink(bool use_colors, std::ostream& output_stream)
    : use_colors_(use_colors), output_stream_(output_stream) {
}

void ConsoleSink::write(types::BeaconSeverity severity, types::BeaconCategory category, 
                       const std::string& message, const std::source_location& location) {
    if (!should_log(severity, category)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Format timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch() % std::chrono::seconds(1)).count();
    
    std::ostringstream timestamp;
    timestamp << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
             << '.' << std::setfill('0') << std::setw(3) << ms;
    
    // Format source location
    std::string file_name = location.file_name();
    // Extract just the filename without the path
    auto pos = file_name.find_last_of("/\\");
    if (pos != std::string::npos) {
        file_name = file_name.substr(pos + 1);
    }
    
    std::ostringstream source;
    source << file_name << ":" << location.line();
    
    // Apply color if enabled
    std::string color_start, color_end;
    if (use_colors_) {
        color_start = types::get_severity_color(severity);
        color_end = types::get_reset_color();
    }
    
    // Format and output the beacon message
    output_stream_ << timestamp.str() << " "
                  << color_start << "[" << types::severity_to_string(severity) << "]" << color_end << " "
                  << "[" << types::category_to_string(category) << "] "
                  << message << " "
                  << "(" << source.str() << ")"
                  << std::endl;
}

void ConsoleSink::set_use_colors(bool use_colors) {
    std::lock_guard<std::mutex> lock(mutex_);
    use_colors_ = use_colors;
}

std::unique_ptr<ConsoleSink> create_console_sink(bool use_colors, std::ostream& output_stream) {
    return std::make_unique<ConsoleSink>(use_colors, output_stream);
}

} // namespace bitscrape::beacon
