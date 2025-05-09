#include "bitscrape/bencode/bencode_value.hpp"

#include <algorithm>
#include <future>
#include <stdexcept>

namespace bitscrape::bencode {

BencodeValue::BencodeValue() : value_(StringValue()) {}

BencodeValue::BencodeValue(const std::string &value) : value_(value) {}

BencodeValue::BencodeValue(const char *value) : value_(std::string(value)) {}

BencodeValue::BencodeValue(std::string_view value)
    : value_(std::string(value)) {}

BencodeValue::BencodeValue(const std::vector<uint8_t> &value)
    : value_(std::string(value.begin(), value.end())) {}

BencodeValue::BencodeValue(int64_t value) : value_(value) {}

BencodeValue::BencodeValue(const std::vector<BencodeValue> &value)
    : value_(value) {}

BencodeValue::BencodeValue(const std::map<std::string, BencodeValue> &value)
    : value_(value) {}

BencodeValue::Type BencodeValue::type() const {
  if (std::holds_alternative<StringValue>(value_)) {
    return Type::STRING;
  } else if (std::holds_alternative<IntegerValue>(value_)) {
    return Type::INTEGER;
  } else if (std::holds_alternative<ListValue>(value_)) {
    return Type::LIST;
  } else if (std::holds_alternative<DictValue>(value_)) {
    return Type::DICT;
  }

  // This should never happen
  throw std::runtime_error("BencodeValue: Invalid type");
}

bool BencodeValue::is_string() const {
  return std::holds_alternative<StringValue>(value_);
}

bool BencodeValue::is_integer() const {
  return std::holds_alternative<IntegerValue>(value_);
}

bool BencodeValue::is_list() const {
  return std::holds_alternative<ListValue>(value_);
}

bool BencodeValue::is_dict() const {
  return std::holds_alternative<DictValue>(value_);
}

const std::string &BencodeValue::as_string() const {
  return std::get<StringValue>(value_);
}

int64_t BencodeValue::as_integer() const {
  return std::get<IntegerValue>(value_);
}

const std::vector<BencodeValue> &BencodeValue::as_list() const {
  return std::get<ListValue>(value_);
}

const std::map<std::string, BencodeValue> &BencodeValue::as_dict() const {
  return std::get<DictValue>(value_);
}

const BencodeValue *BencodeValue::get(const std::string &key) const {
  if (!is_dict()) {
    throw std::bad_variant_access();
  }

  const auto &dict = as_dict();
  auto it = dict.find(key);

  if (it == dict.end()) {
    return nullptr;
  }

  return &it->second;
}

const BencodeValue *BencodeValue::get(size_t index) const {
  if (!is_list()) {
    throw std::bad_variant_access();
  }

  const auto &list = as_list();

  if (index >= list.size()) {
    throw std::out_of_range("BencodeValue: Index out of range");
  }

  return &list[index];
}

void BencodeValue::set(const std::string &key, const BencodeValue &value) {
  if (!is_dict()) {
    throw std::bad_variant_access();
  }

  auto &dict = std::get<DictValue>(value_);
  dict[key] = value;
}

void BencodeValue::set(size_t index, const BencodeValue &value) {
  if (!is_list()) {
    throw std::bad_variant_access();
  }

  auto &list = std::get<ListValue>(value_);

  if (index >= list.size()) {
    throw std::out_of_range("BencodeValue: Index out of range");
  }

  list[index] = value;
}

void BencodeValue::add(const BencodeValue &value) {
  if (!is_list()) {
    throw std::bad_variant_access();
  }

  auto &list = std::get<ListValue>(value_);
  list.push_back(value);
}

bool BencodeValue::remove(const std::string &key) {
  if (!is_dict()) {
    throw std::bad_variant_access();
  }

  auto &dict = std::get<DictValue>(value_);
  return dict.erase(key) > 0;
}

bool BencodeValue::remove(size_t index) {
  if (!is_list()) {
    throw std::bad_variant_access();
  }

  auto &list = std::get<ListValue>(value_);

  if (index >= list.size()) {
    return false;
  }

  list.erase(list.begin() + index);
  return true;
}

bool BencodeValue::operator==(const BencodeValue &other) const {
  return value_ == other.value_;
}

bool BencodeValue::operator!=(const BencodeValue &other) const {
  return !(*this == other);
}

std::future<BencodeValue>
BencodeValue::from_bytes_async(const std::vector<uint8_t> &data) {
  return std::async(std::launch::async, [data]() { return from_bytes(data); });
}

BencodeValue BencodeValue::from_bytes(const std::vector<uint8_t> &data) {
  // This is a simplified implementation that just creates a string value
  // In a real implementation, we would parse the bencode data
  return BencodeValue(std::string(data.begin(), data.end()));
}

std::unique_ptr<BencodeValue> create_bencode_value() {
  return std::make_unique<BencodeValue>();
}

} // namespace bitscrape::bencode
