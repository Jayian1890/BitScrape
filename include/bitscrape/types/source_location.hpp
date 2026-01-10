#pragma once

#include <cstdint>
#include <string>

namespace std {

using uint32_t = unsigned int;

/**
 * @brief Simple implementation of std::source_location
 *
 * This is a simplified version of std::source_location for compilers
 * that don't support it yet.
 */
class source_location {
public:
    /**
     * @brief Create a source location
     *
     * @param file File name
     * @param function Function name
     * @param line Line number
     * @param column Column number
     */
    constexpr source_location(
        const char* file = "unknown",
        const char* function = "unknown",
        std::uint32_t line = 0,
        std::uint32_t column = 0
    ) noexcept
        : file_(file), function_(function), line_(line), column_(column) {}

    /**
     * @brief Get the current source location
     *
     * @return Current source location
     */
    static constexpr source_location current(
        const char* file = __builtin_FILE(),
        const char* function = __builtin_FUNCTION(),
        std::uint32_t line = __builtin_LINE(),
        std::uint32_t column = 0
    ) noexcept {
        return source_location(file, function, line, column);
    }

    /**
     * @brief Get the file name
     *
     * @return File name
     */
    constexpr const char* file_name() const noexcept {
        return file_;
    }

    /**
     * @brief Get the function name
     *
     * @return Function name
     */
    constexpr const char* function_name() const noexcept {
        return function_;
    }

    /**
     * @brief Get the line number
     *
     * @return Line number
     */
    constexpr std::uint32_t line() const noexcept {
        return line_;
    }

    /**
     * @brief Get the column number
     *
     * @return Column number
     */
    constexpr std::uint32_t column() const noexcept {
        return column_;
    }

private:
    const char* file_;
    const char* function_;
    std::uint32_t line_;
    std::uint32_t column_;
};

} // namespace std
