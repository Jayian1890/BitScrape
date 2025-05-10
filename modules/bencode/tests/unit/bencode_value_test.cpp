#include <gtest/gtest.h>
#include <string>

#include "bitscrape/bencode/bencode_value.hpp"

using namespace bitscrape::bencode;

// Helper function to convert string literals to std::string
inline std::string s(const char *str) { return std::string(str); }

TEST(BencodeValueTest, DefaultConstructor) {
  BencodeValue value;

  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_list());
  EXPECT_FALSE(value.is_dict());
  EXPECT_EQ(value.type(), BencodeValue::Type::STRING);
  EXPECT_EQ(value.as_string(), "");
}

TEST(BencodeValueTest, StringConstructor) {
  BencodeValue value(std::string("test"));

  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_list());
  EXPECT_FALSE(value.is_dict());
  EXPECT_EQ(value.type(), BencodeValue::Type::STRING);
  EXPECT_EQ(value.as_string(), "test");
}

TEST(BencodeValueTest, StringViewConstructor) {
  std::string_view view = "test";
  BencodeValue value(view);

  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_list());
  EXPECT_FALSE(value.is_dict());
  EXPECT_EQ(value.type(), BencodeValue::Type::STRING);
  EXPECT_EQ(value.as_string(), "test");
}

TEST(BencodeValueTest, ByteVectorConstructor) {
  std::vector<uint8_t> bytes = {'t', 'e', 's', 't'};
  BencodeValue value(bytes);

  EXPECT_TRUE(value.is_string());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_list());
  EXPECT_FALSE(value.is_dict());
  EXPECT_EQ(value.type(), BencodeValue::Type::STRING);
  EXPECT_EQ(value.as_string(), "test");
}

TEST(BencodeValueTest, IntegerConstructor) {
  BencodeValue value(42);

  EXPECT_FALSE(value.is_string());
  EXPECT_TRUE(value.is_integer());
  EXPECT_FALSE(value.is_list());
  EXPECT_FALSE(value.is_dict());
  EXPECT_EQ(value.type(), BencodeValue::Type::INTEGER);
  EXPECT_EQ(value.as_integer(), 42);
}

TEST(BencodeValueTest, ListConstructor) {
  std::vector<BencodeValue> list = {BencodeValue(std::string("test")), BencodeValue(42),
                                    BencodeValue(std::vector<BencodeValue>())};

  BencodeValue value(list);

  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_integer());
  EXPECT_TRUE(value.is_list());
  EXPECT_FALSE(value.is_dict());
  EXPECT_EQ(value.type(), BencodeValue::Type::LIST);
  EXPECT_EQ(value.as_list().size(), 3);
  EXPECT_EQ(value.as_list()[0].as_string(), "test");
  EXPECT_EQ(value.as_list()[1].as_integer(), 42);
  EXPECT_TRUE(value.as_list()[2].is_list());
}

TEST(BencodeValueTest, DictConstructor) {
  std::map<std::string, BencodeValue> dict = {
      {"string", BencodeValue(std::string("test"))},
      {"integer", BencodeValue(42)},
      {"list", BencodeValue(std::vector<BencodeValue>())}};

  BencodeValue value(dict);

  EXPECT_FALSE(value.is_string());
  EXPECT_FALSE(value.is_integer());
  EXPECT_FALSE(value.is_list());
  EXPECT_TRUE(value.is_dict());
  EXPECT_EQ(value.type(), BencodeValue::Type::DICT);
  EXPECT_EQ(value.as_dict().size(), 3);
  EXPECT_EQ(value.as_dict().at("string").as_string(), "test");
  EXPECT_EQ(value.as_dict().at("integer").as_integer(), 42);
  EXPECT_TRUE(value.as_dict().at("list").is_list());
}

TEST(BencodeValueTest, CopyConstructor) {
  BencodeValue original("test");
  BencodeValue copy(original);

  EXPECT_TRUE(copy.is_string());
  EXPECT_EQ(copy.as_string(), "test");
}

TEST(BencodeValueTest, MoveConstructor) {
  BencodeValue original("test");
  BencodeValue moved(std::move(original));

  EXPECT_TRUE(moved.is_string());
  EXPECT_EQ(moved.as_string(), "test");
}

TEST(BencodeValueTest, CopyAssignment) {
  BencodeValue original("test");
  BencodeValue copy;
  copy = original;

  EXPECT_TRUE(copy.is_string());
  EXPECT_EQ(copy.as_string(), "test");
}

TEST(BencodeValueTest, MoveAssignment) {
  BencodeValue original("test");
  BencodeValue moved;
  moved = std::move(original);

  EXPECT_TRUE(moved.is_string());
  EXPECT_EQ(moved.as_string(), "test");
}

TEST(BencodeValueTest, GetDictValue) {
  std::map<std::string, BencodeValue> dict = {{"string", BencodeValue(std::string("test"))},
                                              {"integer", BencodeValue(42)}};

  BencodeValue value(dict);

  const BencodeValue *string_value = value.get("string");
  EXPECT_NE(string_value, nullptr);
  EXPECT_TRUE(string_value->is_string());
  EXPECT_EQ(string_value->as_string(), "test");

  const BencodeValue *integer_value = value.get("integer");
  EXPECT_NE(integer_value, nullptr);
  EXPECT_TRUE(integer_value->is_integer());
  EXPECT_EQ(integer_value->as_integer(), 42);

  const BencodeValue *missing_value = value.get("missing");
  EXPECT_EQ(missing_value, nullptr);
}

TEST(BencodeValueTest, GetListValue) {
  std::vector<BencodeValue> list = {BencodeValue(std::string("test")), BencodeValue(42)};

  BencodeValue value(list);

  const BencodeValue *string_value = value.get(0);
  EXPECT_NE(string_value, nullptr);
  EXPECT_TRUE(string_value->is_string());
  EXPECT_EQ(string_value->as_string(), "test");

  const BencodeValue *integer_value = value.get(1);
  EXPECT_NE(integer_value, nullptr);
  EXPECT_TRUE(integer_value->is_integer());
  EXPECT_EQ(integer_value->as_integer(), 42);

  EXPECT_THROW(value.get(2), std::out_of_range);
}

TEST(BencodeValueTest, SetDictValue) {
  std::map<std::string, BencodeValue> dict = {{"string", BencodeValue(std::string("test"))}};

  BencodeValue value(dict);

  value.set("string", BencodeValue(std::string("new value")));
  value.set("integer", BencodeValue(42));

  EXPECT_EQ(value.as_dict().size(), 2);
  EXPECT_EQ(value.as_dict().at("string").as_string(), "new value");
  EXPECT_EQ(value.as_dict().at("integer").as_integer(), 42);
}

TEST(BencodeValueTest, SetListValue) {
  std::vector<BencodeValue> list = {BencodeValue(std::string("test")), BencodeValue(42)};

  BencodeValue value(list);

  value.set(0, BencodeValue(std::string("new value")));
  value.set(1, BencodeValue(43));

  EXPECT_EQ(value.as_list().size(), 2);
  EXPECT_EQ(value.as_list()[0].as_string(), "new value");
  EXPECT_EQ(value.as_list()[1].as_integer(), 43);

  EXPECT_THROW(value.set(2, BencodeValue(std::string("out of range"))), std::out_of_range);
}

TEST(BencodeValueTest, AddListValue) {
  std::vector<BencodeValue> list = {BencodeValue(std::string("test"))};

  BencodeValue value(list);

  value.add(BencodeValue(42));

  EXPECT_EQ(value.as_list().size(), 2);
  EXPECT_EQ(value.as_list()[0].as_string(), "test");
  EXPECT_EQ(value.as_list()[1].as_integer(), 42);
}

TEST(BencodeValueTest, RemoveDictValue) {
  std::map<std::string, BencodeValue> dict = {{"string", BencodeValue(std::string("test"))},
                                              {"integer", BencodeValue(42)}};

  BencodeValue value(dict);

  EXPECT_TRUE(value.remove("string"));
  EXPECT_FALSE(value.remove("missing"));

  EXPECT_EQ(value.as_dict().size(), 1);
  EXPECT_EQ(value.as_dict().count("string"), 0);
  EXPECT_EQ(value.as_dict().at("integer").as_integer(), 42);
}

TEST(BencodeValueTest, RemoveListValue) {
  std::vector<BencodeValue> list = {BencodeValue(std::string("test")), BencodeValue(42)};

  BencodeValue value(list);

  EXPECT_TRUE(value.remove(0));
  EXPECT_FALSE(value.remove(1));

  EXPECT_EQ(value.as_list().size(), 1);
  EXPECT_EQ(value.as_list()[0].as_integer(), 42);
}

TEST(BencodeValueTest, Equality) {
  BencodeValue value1("test");
  BencodeValue value2("test");
  BencodeValue value3(42);

  EXPECT_EQ(value1, value2);
  EXPECT_NE(value1, value3);
}

TEST(BencodeValueTest, FromBytes) {
  std::vector<uint8_t> bytes = {'t', 'e', 's', 't'};

  BencodeValue value = BencodeValue::from_bytes(bytes);

  EXPECT_TRUE(value.is_string());
  EXPECT_EQ(value.as_string(), "test");
}

TEST(BencodeValueTest, FromBytesAsync) {
  std::vector<uint8_t> bytes = {'t', 'e', 's', 't'};

  auto future = BencodeValue::from_bytes_async(bytes);
  BencodeValue value = future.get();

  EXPECT_TRUE(value.is_string());
  EXPECT_EQ(value.as_string(), "test");
}

TEST(BencodeValueTest, CreateBencodeValue) {
  auto value = create_bencode_value();

  EXPECT_NE(value, nullptr);
  EXPECT_TRUE(value->is_string());
  EXPECT_EQ(value->as_string(), "");
}
