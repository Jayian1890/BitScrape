#include "bitscrape/network/buffer.hpp"

#include <gtest/gtest.h>

namespace bitscrape::network::test {

TEST(BufferTest, DefaultConstruction) {
    Buffer buffer;
    EXPECT_EQ(buffer.size(), 0UL);
    EXPECT_GE(buffer.capacity(), 0UL);
}

TEST(BufferTest, ConstructWithCapacity) {
    Buffer buffer(1024);
    EXPECT_EQ(buffer.size(), 0UL);
    EXPECT_GE(buffer.capacity(), 1024UL);
}

TEST(BufferTest, ConstructWithData) {
    const uint8_t data[] = {1, 2, 3, 4, 5};
    Buffer buffer(data, sizeof(data));

    EXPECT_EQ(buffer.size(), sizeof(data));
    EXPECT_EQ(std::memcmp(buffer.data(), data, sizeof(data)), 0);
}

TEST(BufferTest, ConstructWithVector) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    Buffer buffer(data);

    EXPECT_EQ(buffer.size(), data.size());
    EXPECT_EQ(std::memcmp(buffer.data(), data.data(), data.size()), 0);
}

TEST(BufferTest, Resize) {
    Buffer buffer;
    EXPECT_EQ(buffer.size(), 0UL);

    buffer.resize(10);
    EXPECT_EQ(buffer.size(), 10UL);

    buffer.resize(5);
    EXPECT_EQ(buffer.size(), 5UL);
}

TEST(BufferTest, Reserve) {
    Buffer buffer;
    size_t initial_capacity = buffer.capacity();

    buffer.reserve(initial_capacity * 2);
    EXPECT_GE(buffer.capacity(), initial_capacity * 2);
    EXPECT_EQ(buffer.size(), 0UL);
}

TEST(BufferTest, Clear) {
    Buffer buffer;
    buffer.resize(10);
    EXPECT_EQ(buffer.size(), 10UL);

    buffer.clear();
    EXPECT_EQ(buffer.size(), 0UL);
}

TEST(BufferTest, AppendData) {
    Buffer buffer;
    const uint8_t data1[] = {1, 2, 3};
    const uint8_t data2[] = {4, 5};

    buffer.append(data1, sizeof(data1));
    EXPECT_EQ(buffer.size(), static_cast<size_t>(sizeof(data1)));

    buffer.append(data2, sizeof(data2));
    EXPECT_EQ(buffer.size(), static_cast<size_t>(sizeof(data1) + sizeof(data2)));

    const uint8_t expected[] = {1, 2, 3, 4, 5};
    EXPECT_EQ(std::memcmp(buffer.data(), expected, sizeof(expected)), 0);
}

TEST(BufferTest, AppendVector) {
    Buffer buffer;
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};

    buffer.append(data);
    EXPECT_EQ(buffer.size(), data.size());
    EXPECT_EQ(std::memcmp(buffer.data(), data.data(), data.size()), 0);
}

TEST(BufferTest, AppendBuffer) {
    Buffer buffer1;
    Buffer buffer2;

    const uint8_t data1[] = {1, 2, 3};
    const uint8_t data2[] = {4, 5};

    buffer1.append(data1, sizeof(data1));
    buffer2.append(data2, sizeof(data2));

    buffer1.append(buffer2);
    EXPECT_EQ(buffer1.size(), static_cast<size_t>(sizeof(data1) + sizeof(data2)));

    const uint8_t expected[] = {1, 2, 3, 4, 5};
    EXPECT_EQ(std::memcmp(buffer1.data(), expected, sizeof(expected)), 0);
}

TEST(BufferTest, ToVector) {
    const uint8_t data[] = {1, 2, 3, 4, 5};
    Buffer buffer(data, sizeof(data));

    std::vector<uint8_t> vec = buffer.to_vector();
    EXPECT_EQ(vec.size(), static_cast<size_t>(sizeof(data)));
    EXPECT_EQ(std::memcmp(vec.data(), data, sizeof(data)), 0);
}

TEST(BufferTest, ReadAt) {
    const uint8_t data[] = {1, 2, 3, 4, 5};
    Buffer buffer(data, sizeof(data));

    uint8_t read_data[3];
    size_t bytes_read = buffer.read_at(1, read_data, sizeof(read_data));

    EXPECT_EQ(bytes_read, static_cast<size_t>(sizeof(read_data)));

    const uint8_t expected[] = {2, 3, 4};
    EXPECT_EQ(std::memcmp(read_data, expected, sizeof(expected)), 0);
}

TEST(BufferTest, WriteAt) {
    Buffer buffer(5);
    buffer.resize(5);

    const uint8_t data[] = {9, 8, 7};
    size_t bytes_written = buffer.write_at(1, data, sizeof(data));

    EXPECT_EQ(bytes_written, static_cast<size_t>(sizeof(data)));

    const uint8_t expected[] = {0, 9, 8, 7, 0};
    EXPECT_EQ(std::memcmp(buffer.data(), expected, sizeof(expected)), 0);
}

TEST(BufferTest, ConstructWithNullData) {
    // Test with null data but zero size (should be valid)
    Buffer buffer(nullptr, 0);
    EXPECT_EQ(buffer.size(), 0UL);
}

TEST(BufferTest, ConstructWithNullDataNonZeroSize) {
    // Test with null data and non-zero size (should throw)
    EXPECT_THROW(Buffer(nullptr, 5), std::invalid_argument);
}

TEST(BufferTest, AppendNullData) {
    Buffer buffer;

    // Test with null data but zero size (should be valid)
    buffer.append(nullptr, 0);
    EXPECT_EQ(buffer.size(), 0UL);

    // Test with null data and non-zero size (should throw)
    EXPECT_THROW(buffer.append(nullptr, 5), std::invalid_argument);
}

TEST(BufferTest, ReadAtNullData) {
    const uint8_t data[] = {1, 2, 3, 4, 5};
    Buffer buffer(data, sizeof(data));

    // Test with null data but zero size (should be valid)
    size_t bytes_read = buffer.read_at(0, nullptr, 0);
    EXPECT_EQ(bytes_read, 0UL);

    // Test with null data and non-zero size (should throw)
    EXPECT_THROW(buffer.read_at(0, nullptr, 3), std::invalid_argument);
}

TEST(BufferTest, ReadAtOutOfBounds) {
    const uint8_t data[] = {1, 2, 3, 4, 5};
    Buffer buffer(data, sizeof(data));

    uint8_t read_data[3];

    // Test reading from an offset beyond the end of the buffer
    size_t bytes_read = buffer.read_at(10, read_data, sizeof(read_data));
    EXPECT_EQ(bytes_read, 0UL);
}

TEST(BufferTest, WriteAtNullData) {
    Buffer buffer(5);

    // Test with null data but zero size (should be valid)
    size_t bytes_written = buffer.write_at(0, nullptr, 0);
    EXPECT_EQ(bytes_written, 0UL);

    // Test with null data and non-zero size (should throw)
    EXPECT_THROW(buffer.write_at(0, nullptr, 3), std::invalid_argument);
}

TEST(BufferTest, WriteAtResize) {
    Buffer buffer;

    // Test writing beyond the current size (should resize)
    const uint8_t data[] = {1, 2, 3};
    size_t bytes_written = buffer.write_at(2, data, sizeof(data));

    EXPECT_EQ(bytes_written, sizeof(data));
    EXPECT_EQ(buffer.size(), 5UL); // 2 (offset) + 3 (data size)

    // Verify the data
    const uint8_t expected[] = {0, 0, 1, 2, 3};
    EXPECT_EQ(std::memcmp(buffer.data(), expected, buffer.size()), 0);
}

} // namespace bitscrape::network::test
