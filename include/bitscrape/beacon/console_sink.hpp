#pragma once

#include <mutex>
#include <iostream>

#include "bitscrape/beacon/beacon_sink.hpp"

namespace bitscrape::beacon {

/**
 * @brief Console output destination for beacon messages
 *
 * ConsoleSink outputs beacon messages to the console with optional color-coding.
 */
class ConsoleSink : public BeaconSink {
public:
    /**
     * @brief Create a console sink
     *
     * @param use_colors Whether to use colors for output
     * @param output_stream Output stream to write to (defaults to std::cout)
     */
    explicit ConsoleSink(bool use_colors = true, std::ostream& output_stream = std::cout);

    /**
     * @brief Destructor
     */
    ~ConsoleSink() override = default;

    /**
     * @brief Write a beacon message to the console
     *
     * @param severity Beacon severity level
     * @param category Beacon category
     * @param message Beacon message
     * @param location Source location where the beacon was emitted
     */
    void write(types::BeaconSeverity severity, types::BeaconCategory category,
              const std::string& message, const std::source_location& location) override;

    /**
     * @brief Set whether to use colors for output
     *
     * @param use_colors Whether to use colors for output
     */
    void set_use_colors(bool use_colors);

    /**
     * @brief Check if colors are used for output
     *
     * @return true if colors are used, false otherwise
     */
    bool use_colors() const { return use_colors_; }

private:
    bool use_colors_; ///< Whether to use colors for output
    std::ostream& output_stream_; ///< Output stream to write to
    std::mutex console_mutex_; ///< Mutex for thread-safe console output
};

/**
 * @brief Create a new console sink
 *
 * @param use_colors Whether to use colors for output
 * @param output_stream Output stream to write to (defaults to std::cout)
 * @return Unique pointer to a new console sink
 */
std::unique_ptr<ConsoleSink> create_console_sink(bool use_colors = true, std::ostream& output_stream = std::cout);

} // namespace bitscrape::beacon
