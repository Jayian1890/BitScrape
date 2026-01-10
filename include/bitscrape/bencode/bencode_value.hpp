#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace bitscrape::bencode {

/**
 * @brief Represents a bencode value
 *
 * BencodeValue is a variant-based class that can represent any of the four
 * bencode types: string, integer, list, or dictionary.
 */
class BencodeValue {
public:
  /**
   * @brief Bencode value type
   */
  enum class Type {
    STRING,  ///< String value
    INTEGER, ///< Integer value
    LIST,    ///< List value
    DICT     ///< Dictionary value
  };

  /**
   * @brief Default constructor creates a string value
   */
  BencodeValue();

  /**
   * @brief Create a string value
   *
   * @param value String value
   */
  explicit BencodeValue(const std::string &value);

  /**
   * @brief Create a string value from a string literal
   *
   * @param value String literal
   */
  explicit BencodeValue(const char *value);

  /**
   * @brief Create a string value from a string view
   *
   * @param value String view
   */
  explicit BencodeValue(std::string_view value);

  /**
   * @brief Create a string value from a byte vector
   *
   * @param value Byte vector
   */
  explicit BencodeValue(const std::vector<uint8_t> &value);

  /**
   * @brief Create an integer value
   *
   * @param value Integer value
   */
  explicit BencodeValue(int64_t value);

  /**
   * @brief Create a list value
   *
   * @param value List of BencodeValue objects
   */
  explicit BencodeValue(const std::vector<BencodeValue> &value);

  /**
   * @brief Create a dictionary value
   *
   * @param value Dictionary mapping strings to BencodeValue objects
   */
  explicit BencodeValue(const std::map<std::string, BencodeValue> &value);

  /**
   * @brief Copy constructor
   */
  BencodeValue(const BencodeValue &other) = default;

  /**
   * @brief Move constructor
   */
  BencodeValue(BencodeValue &&other) noexcept = default;

  /**
   * @brief Copy assignment operator
   */
  BencodeValue &operator=(const BencodeValue &other) = default;

  /**
   * @brief Move assignment operator
   */
  BencodeValue &operator=(BencodeValue &&other) noexcept = default;

  /**
   * @brief Destructor
   */
  ~BencodeValue() = default;

  /**
   * @brief Get the type of the value
   *
   * @return Type of the value
   */
  Type type() const;

  /**
   * @brief Check if the value is a string
   *
   * @return true if the value is a string, false otherwise
   */
  bool is_string() const;

  /**
   * @brief Check if the value is an integer
   *
   * @return true if the value is an integer, false otherwise
   */
  bool is_integer() const;

  /**
   * @brief Check if the value is a list
   *
   * @return true if the value is a list, false otherwise
   */
  bool is_list() const;

  /**
   * @brief Check if the value is a dictionary
   *
   * @return true if the value is a dictionary, false otherwise
   */
  bool is_dict() const;

  /**
   * @brief Get the string value
   *
   * @return String value
   * @throws std::bad_variant_access if the value is not a string
   */
  const std::string &as_string() const;

  /**
   * @brief Get the integer value
   *
   * @return Integer value
   * @throws std::bad_variant_access if the value is not an integer
   */
  int64_t as_integer() const;

  /**
   * @brief Get the list value
   *
   * @return List value
   * @throws std::bad_variant_access if the value is not a list
   */
  const std::vector<BencodeValue> &as_list() const;

  /**
   * @brief Get the dictionary value
   *
   * @return Dictionary value
   * @throws std::bad_variant_access if the value is not a dictionary
   */
  const std::map<std::string, BencodeValue> &as_dict() const;

  /**
   * @brief Get a value from a dictionary by key
   *
   * @param key Key to look up
   * @return Pointer to the value if found, nullptr otherwise
   * @throws std::bad_variant_access if the value is not a dictionary
   */
  const BencodeValue *get(const std::string &key) const;

  /**
   * @brief Get a value from a list by index
   *
   * @param index Index to look up
   * @return Pointer to the value if found, nullptr otherwise
   * @throws std::bad_variant_access if the value is not a list
   * @throws std::out_of_range if the index is out of range
   */
  const BencodeValue *get(size_t index) const;

  /**
   * @brief Set a value in a dictionary
   *
   * @param key Key to set
   * @param value Value to set
   * @throws std::bad_variant_access if the value is not a dictionary
   */
  void set(const std::string &key, const BencodeValue &value);

  /**
   * @brief Set a value in a list
   *
   * @param index Index to set
   * @param value Value to set
   * @throws std::bad_variant_access if the value is not a list
   * @throws std::out_of_range if the index is out of range
   */
  void set(size_t index, const BencodeValue &value);

  /**
   * @brief Add a value to a list
   *
   * @param value Value to add
   * @throws std::bad_variant_access if the value is not a list
   */
  void add(const BencodeValue &value);

  /**
   * @brief Remove a value from a dictionary
   *
   * @param key Key to remove
   * @return true if the key was found and removed, false otherwise
   * @throws std::bad_variant_access if the value is not a dictionary
   */
  bool remove(const std::string &key);

  /**
   * @brief Remove a value from a list
   *
   * @param index Index to remove
   * @return true if the index was valid and the value was removed, false
   * otherwise
   * @throws std::bad_variant_access if the value is not a list
   */
  bool remove(size_t index);

  /**
   * @brief Equality operator
   */
  bool operator==(const BencodeValue &other) const;

  /**
   * @brief Inequality operator
   */
  bool operator!=(const BencodeValue &other) const;

  /**
   * @brief Create a BencodeValue from a byte vector asynchronously
   *
   * @param data Byte vector
   * @return Future containing the created BencodeValue
   */
  static std::future<BencodeValue>
  from_bytes_async(const std::vector<uint8_t> &data);

  /**
   * @brief Create a BencodeValue from a byte vector
   *
   * @param data Byte vector
   * @return Created BencodeValue
   */
  static BencodeValue from_bytes(const std::vector<uint8_t> &data);

private:
  using StringValue = std::string;
  using IntegerValue = int64_t;
  using ListValue = std::vector<BencodeValue>;
  using DictValue = std::map<std::string, BencodeValue>;

  using ValueVariant =
      std::variant<StringValue, IntegerValue, ListValue, DictValue>;

  ValueVariant value_; ///< The variant holding the value
};

/**
 * @brief Create a new BencodeValue
 *
 * @return Unique pointer to a new BencodeValue
 */
std::unique_ptr<BencodeValue> create_bencode_value();

} // namespace bitscrape::bencode
