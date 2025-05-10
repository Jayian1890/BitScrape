#include <gtest/gtest.h>

#include "bitscrape/bencode/bencode_stream.hpp"
#include "bitscrape/bencode/bencode_value.hpp"

#include <sstream>

using namespace bitscrape::bencode;

TEST(BencodeStreamTest, ReadString) {
    auto stream = create_bencode_stream();

    std::istringstream input("4:test");
    auto value = stream->read_string(input);

    EXPECT_EQ(value, "test");
}

TEST(BencodeStreamTest, ReadStringAsync) {
    auto stream = create_bencode_stream();

    std::istringstream input("4:test");
    auto future = stream->read_string_async(input);
    auto value = future.get();

    EXPECT_EQ(value, "test");
}

TEST(BencodeStreamTest, ReadInteger) {
    auto stream = create_bencode_stream();

    std::istringstream input("i42e");
    auto value = stream->read_integer(input);

    EXPECT_EQ(value, 42);
}

TEST(BencodeStreamTest, ReadIntegerAsync) {
    auto stream = create_bencode_stream();

    std::istringstream input("i42e");
    auto future = stream->read_integer_async(input);
    auto value = future.get();

    EXPECT_EQ(value, 42);
}

TEST(BencodeStreamTest, ReadList) {
    auto stream = create_bencode_stream();

    std::istringstream input("l4:testi42ee");
    auto value = stream->read_list(input);

    EXPECT_EQ(value.size(), 2UL);
    EXPECT_TRUE(value[0].is_string());
    EXPECT_EQ(value[0].as_string(), "test");
    EXPECT_TRUE(value[1].is_integer());
    EXPECT_EQ(value[1].as_integer(), 42);
}

TEST(BencodeStreamTest, ReadListAsync) {
    auto stream = create_bencode_stream();

    std::istringstream input("l4:testi42ee");
    auto future = stream->read_list_async(input);
    auto value = future.get();

    EXPECT_EQ(value.size(), 2UL);
    EXPECT_TRUE(value[0].is_string());
    EXPECT_EQ(value[0].as_string(), "test");
    EXPECT_TRUE(value[1].is_integer());
    EXPECT_EQ(value[1].as_integer(), 42);
}

TEST(BencodeStreamTest, ReadDict) {
    auto stream = create_bencode_stream();

    std::istringstream input("d6:string4:test7:integeri42ee");
    auto value = stream->read_dict(input);

    EXPECT_EQ(value.size(), 2UL);
    EXPECT_TRUE(value["string"].is_string());
    EXPECT_EQ(value["string"].as_string(), "test");
    EXPECT_TRUE(value["integer"].is_integer());
    EXPECT_EQ(value["integer"].as_integer(), 42);
}

TEST(BencodeStreamTest, ReadDictAsync) {
    auto stream = create_bencode_stream();

    std::istringstream input("d6:string4:test7:integeri42ee");
    auto future = stream->read_dict_async(input);
    auto value = future.get();

    EXPECT_EQ(value.size(), 2UL);
    EXPECT_TRUE(value["string"].is_string());
    EXPECT_EQ(value["string"].as_string(), "test");
    EXPECT_TRUE(value["integer"].is_integer());
    EXPECT_EQ(value["integer"].as_integer(), 42);
}

TEST(BencodeStreamTest, ReadValue) {
    auto stream = create_bencode_stream();

    std::istringstream input("d6:string4:test7:integeri42ee");
    auto value = stream->read(input);

    EXPECT_TRUE(value.is_dict());
    EXPECT_EQ(value.as_dict().size(), 2UL);
    EXPECT_TRUE(value.as_dict().at("string").is_string());
    EXPECT_EQ(value.as_dict().at("string").as_string(), "test");
    EXPECT_TRUE(value.as_dict().at("integer").is_integer());
    EXPECT_EQ(value.as_dict().at("integer").as_integer(), 42);
}

TEST(BencodeStreamTest, ReadValueAsync) {
    auto stream = create_bencode_stream();

    std::istringstream input("d6:string4:test7:integeri42ee");
    auto future = stream->read_async(input);
    auto value = future.get();

    EXPECT_TRUE(value.is_dict());
    EXPECT_EQ(value.as_dict().size(), 2UL);
    EXPECT_TRUE(value.as_dict().at("string").is_string());
    EXPECT_EQ(value.as_dict().at("string").as_string(), "test");
    EXPECT_TRUE(value.as_dict().at("integer").is_integer());
    EXPECT_EQ(value.as_dict().at("integer").as_integer(), 42);
}

TEST(BencodeStreamTest, WriteString) {
    auto stream = create_bencode_stream();

    BencodeValue value(std::string("test"));
    std::ostringstream output;
    auto bytes_written = stream->write(value, output);

    EXPECT_EQ(output.str(), "4:test");
    EXPECT_EQ(bytes_written, 6UL);
}

TEST(BencodeStreamTest, WriteStringAsync) {
    auto stream = create_bencode_stream();

    BencodeValue value(std::string("test"));
    std::ostringstream output;
    auto future = stream->write_async(value, output);
    auto bytes_written = future.get();

    EXPECT_EQ(output.str(), "4:test");
    EXPECT_EQ(bytes_written, 6UL);
}

TEST(BencodeStreamTest, WriteInteger) {
    auto stream = create_bencode_stream();

    BencodeValue value(42);
    std::ostringstream output;
    auto bytes_written = stream->write(value, output);

    EXPECT_EQ(output.str(), "i42e");
    EXPECT_EQ(bytes_written, 4UL);
}

TEST(BencodeStreamTest, WriteIntegerAsync) {
    auto stream = create_bencode_stream();

    BencodeValue value(42);
    std::ostringstream output;
    auto future = stream->write_async(value, output);
    auto bytes_written = future.get();

    EXPECT_EQ(output.str(), "i42e");
    EXPECT_EQ(bytes_written, 4UL);
}

TEST(BencodeStreamTest, WriteList) {
    auto stream = create_bencode_stream();

    BencodeValue value(std::vector<BencodeValue>{
        BencodeValue(std::string("test")),
        BencodeValue(42)
    });

    std::ostringstream output;
    auto bytes_written = stream->write(value, output);

    EXPECT_EQ(output.str(), "l4:testi42ee");
    EXPECT_EQ(bytes_written, 12UL);
}

TEST(BencodeStreamTest, WriteListAsync) {
    auto stream = create_bencode_stream();

    BencodeValue value(std::vector<BencodeValue>{
        BencodeValue(std::string("test")),
        BencodeValue(42)
    });

    std::ostringstream output;
    auto future = stream->write_async(value, output);
    auto bytes_written = future.get();

    EXPECT_EQ(output.str(), "l4:testi42ee");
    EXPECT_EQ(bytes_written, 12UL);
}

TEST(BencodeStreamTest, WriteDict) {
    auto stream = create_bencode_stream();

    BencodeValue value(std::map<std::string, BencodeValue>{
        {"string", BencodeValue(std::string("test"))},
        {"integer", BencodeValue(42)}
    });

    std::ostringstream output;
    auto bytes_written = stream->write(value, output);

    // Note: The order of keys in a map is sorted, so "integer" comes before "string"
    EXPECT_EQ(output.str(), "d7:integeri42e6:string4:teste");
    EXPECT_EQ(bytes_written, 29UL);
}

TEST(BencodeStreamTest, WriteDictAsync) {
    auto stream = create_bencode_stream();

    BencodeValue value(std::map<std::string, BencodeValue>{
        {"string", BencodeValue(std::string("test"))},
        {"integer", BencodeValue(42)}
    });

    std::ostringstream output;
    auto future = stream->write_async(value, output);
    auto bytes_written = future.get();

    // Note: The order of keys in a map is sorted, so "integer" comes before "string"
    EXPECT_EQ(output.str(), "d7:integeri42e6:string4:teste");
    EXPECT_EQ(bytes_written, 29UL);
}

TEST(BencodeStreamTest, ReadWriteRoundTrip) {
    auto stream = create_bencode_stream();

    BencodeValue original(std::map<std::string, BencodeValue>{
        {"string", BencodeValue(std::string("test"))},
        {"integer", BencodeValue(42)},
        {"list", BencodeValue(std::vector<BencodeValue>{
            BencodeValue(std::string("item1")),
            BencodeValue(std::string("item2"))
        })},
        {"dict", BencodeValue(std::map<std::string, BencodeValue>{
            {"key1", BencodeValue(std::string("value1"))},
            {"key2", BencodeValue(std::string("value2"))}
        })}
    });

    // Write the value to a string
    std::ostringstream output;
    stream->write(original, output);
    std::string encoded = output.str();

    // Read the value back
    std::istringstream input(encoded);
    BencodeValue decoded = stream->read(input);

    // Check that the decoded value matches the original
    EXPECT_TRUE(decoded.is_dict());
    EXPECT_EQ(decoded.as_dict().size(), 4UL);

    // Check string
    EXPECT_TRUE(decoded.as_dict().at("string").is_string());
    EXPECT_EQ(decoded.as_dict().at("string").as_string(), "test");

    // Check integer
    EXPECT_TRUE(decoded.as_dict().at("integer").is_integer());
    EXPECT_EQ(decoded.as_dict().at("integer").as_integer(), 42);

    // Check list
    EXPECT_TRUE(decoded.as_dict().at("list").is_list());
    EXPECT_EQ(decoded.as_dict().at("list").as_list().size(), 2UL);
    EXPECT_TRUE(decoded.as_dict().at("list").as_list()[0].is_string());
    EXPECT_EQ(decoded.as_dict().at("list").as_list()[0].as_string(), "item1");
    EXPECT_TRUE(decoded.as_dict().at("list").as_list()[1].is_string());
    EXPECT_EQ(decoded.as_dict().at("list").as_list()[1].as_string(), "item2");

    // Check dict
    EXPECT_TRUE(decoded.as_dict().at("dict").is_dict());
    EXPECT_EQ(decoded.as_dict().at("dict").as_dict().size(), 2UL);
    EXPECT_TRUE(decoded.as_dict().at("dict").as_dict().at("key1").is_string());
    EXPECT_EQ(decoded.as_dict().at("dict").as_dict().at("key1").as_string(), "value1");
    EXPECT_TRUE(decoded.as_dict().at("dict").as_dict().at("key2").is_string());
    EXPECT_EQ(decoded.as_dict().at("dict").as_dict().at("key2").as_string(), "value2");
}

TEST(BencodeStreamTest, CreateBencodeStream) {
    auto stream = create_bencode_stream();

    EXPECT_NE(stream, nullptr);
}
