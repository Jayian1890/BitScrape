#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace bitscrape::network {

/**
 * @brief Buffer class for network data handling
 * 
 * This class provides a convenient way to handle network data.
 * It supports reading and writing data with various data types.
 */
class Buffer {
public:
    /**
     * @brief Construct a new Buffer object with the specified capacity
     * 
     * @param capacity The initial capacity of the buffer
     */
    explicit Buffer(size_t capacity = 1024);

    /**
     * @brief Construct a new Buffer object from existing data
     * 
     * @param data The data to initialize the buffer with
     * @param size The size of the data
     */
    Buffer(const uint8_t* data, size_t size);

    /**
     * @brief Construct a new Buffer object from a vector
     * 
     * @param data The vector to initialize the buffer with
     */
    explicit Buffer(const std::vector<uint8_t>& data);

    /**
     * @brief Destroy the Buffer object
     */
    ~Buffer() = default;

    /**
     * @brief Buffer is copyable
     */
    Buffer(const Buffer&) = default;
    Buffer& operator=(const Buffer&) = default;

    /**
     * @brief Buffer is movable
     */
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;

    /**
     * @brief Get the data pointer
     * 
     * @return The data pointer
     */
    const uint8_t* data() const;

    /**
     * @brief Get the data pointer
     * 
     * @return The data pointer
     */
    uint8_t* data();

    /**
     * @brief Get the size of the buffer
     * 
     * @return The size of the buffer
     */
    size_t size() const;

    /**
     * @brief Get the capacity of the buffer
     * 
     * @return The capacity of the buffer
     */
    size_t capacity() const;

    /**
     * @brief Resize the buffer
     * 
     * @param size The new size
     */
    void resize(size_t size);

    /**
     * @brief Reserve capacity for the buffer
     * 
     * @param capacity The new capacity
     */
    void reserve(size_t capacity);

    /**
     * @brief Clear the buffer
     */
    void clear();

    /**
     * @brief Append data to the buffer
     * 
     * @param data The data to append
     * @param size The size of the data
     */
    void append(const uint8_t* data, size_t size);

    /**
     * @brief Append a vector to the buffer
     * 
     * @param data The vector to append
     */
    void append(const std::vector<uint8_t>& data);

    /**
     * @brief Append another buffer to this buffer
     * 
     * @param buffer The buffer to append
     */
    void append(const Buffer& buffer);

    /**
     * @brief Get the buffer as a vector
     * 
     * @return The buffer as a vector
     */
    std::vector<uint8_t> to_vector() const;

    /**
     * @brief Read data from the buffer at the specified offset
     * 
     * @param offset The offset to read from
     * @param data The buffer to read into
     * @param size The number of bytes to read
     * @return The number of bytes read
     */
    size_t read_at(size_t offset, uint8_t* data, size_t size) const;

    /**
     * @brief Write data to the buffer at the specified offset
     * 
     * @param offset The offset to write to
     * @param data The data to write
     * @param size The number of bytes to write
     * @return The number of bytes written
     */
    size_t write_at(size_t offset, const uint8_t* data, size_t size);

private:
    std::vector<uint8_t> data_;
};

} // namespace bitscrape::network
