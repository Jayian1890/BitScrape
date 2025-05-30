#include "bitscrape/network/buffer.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace bitscrape::network {

Buffer::Buffer(size_t capacity) {
    data_.reserve(capacity);
}

Buffer::Buffer(const uint8_t* data, size_t size) {
    if (data == nullptr && size > 0) {
        throw std::invalid_argument("Buffer: data is null but size is non-zero");
    }

    data_.resize(size);
    if (size > 0) {
        std::memcpy(data_.data(), data, size);
    }
}

Buffer::Buffer(const std::vector<uint8_t>& data)
    : data_(data) {
}

const uint8_t* Buffer::data() const {
    return data_.data();
}

uint8_t* Buffer::data() {
    return data_.data();
}

size_t Buffer::size() const {
    return data_.size();
}

size_t Buffer::capacity() const {
    return data_.capacity();
}

void Buffer::resize(size_t size) {
    data_.resize(size);
}

void Buffer::reserve(size_t capacity) {
    data_.reserve(capacity);
}

void Buffer::clear() {
    data_.clear();
}

void Buffer::append(const uint8_t* data, size_t size) {
    if (data == nullptr && size > 0) {
        throw std::invalid_argument("Buffer::append: data is null but size is non-zero");
    }

    if (size == 0) {
        return;
    }

    // Check for potential overflow
    if (data_.size() > std::numeric_limits<size_t>::max() - size) {
        throw std::overflow_error("Buffer::append: size would overflow");
    }

    size_t old_size = data_.size();
    data_.resize(old_size + size);
    std::memcpy(data_.data() + old_size, data, size);
}

void Buffer::append(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return;
    }

    append(data.data(), data.size());
}

void Buffer::append(const Buffer& buffer) {
    if (buffer.size() == 0) {
        return;
    }

    append(buffer.data(), buffer.size());
}

std::vector<uint8_t> Buffer::to_vector() const {
    return data_;
}

size_t Buffer::read_at(size_t offset, uint8_t* data, size_t size) const {
    if (data == nullptr && size > 0) {
        throw std::invalid_argument("Buffer::read_at: data is null but size is non-zero");
    }

    if (offset >= data_.size() || size == 0) {
        return 0;
    }

    // Check for potential overflow in offset + bytes calculation
    if (offset > data_.size()) {
        return 0; // Offset is beyond the end of the buffer
    }

    size_t bytes_to_read = std::min(size, data_.size() - offset);
    std::memcpy(data, data_.data() + offset, bytes_to_read);
    return bytes_to_read;
}

size_t Buffer::write_at(size_t offset, const uint8_t* data, size_t size) {
    if (data == nullptr && size > 0) {
        throw std::invalid_argument("Buffer::write_at: data is null but size is non-zero");
    }

    if (size == 0) {
        return 0;
    }

    // Check for potential overflow in offset + size calculation
    if (offset > std::numeric_limits<size_t>::max() - size) {
        throw std::overflow_error("Buffer::write_at: offset + size would overflow");
    }

    if (offset + size > data_.size()) {
        data_.resize(offset + size);
    }

    std::memcpy(data_.data() + offset, data, size);
    return size;
}

} // namespace bitscrape::network
