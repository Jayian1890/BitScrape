#pragma once

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <memory>

namespace bitscrape::web {

/**
 * @brief Simple JSON implementation using only the C++ standard library
 */
class JSON {
public:
    // Forward declarations
    class Object;
    class Array;

    // JSON value types
    using Null = std::monostate;
    using Boolean = bool;
    using Number = double;
    using String = std::string;
    using ObjectType = std::map<std::string, JSON>;
    using ArrayType = std::vector<JSON>;

    // Value variant
    using ValueType = std::variant<Null, Boolean, Number, String, std::shared_ptr<ObjectType>, std::shared_ptr<ArrayType>>;

    /**
     * @brief Default constructor creates a null JSON value
     */
    JSON() : value_(Null{}) {}

    /**
     * @brief Create a JSON value from a boolean
     */
    JSON(bool value) : value_(value) {}

    /**
     * @brief Create a JSON value from an integer
     */
    JSON(int value) : value_(static_cast<Number>(value)) {}

    /**
     * @brief Create a JSON value from a size_t
     */
    JSON(size_t value) : value_(static_cast<Number>(value)) {}

    /**
     * @brief Create a JSON value from a uint32_t
     */
    JSON(uint32_t value) : value_(static_cast<Number>(value)) {}

    /**
     * @brief Create a JSON value from a uint64_t
     */
    JSON(uint64_t value) : value_(static_cast<Number>(value)) {}

    /**
     * @brief Create a JSON value from a long
     */
    JSON(long value) : value_(static_cast<Number>(value)) {}

    /**
     * @brief Create a JSON value from a double
     */
    JSON(double value) : value_(value) {}

    /**
     * @brief Create a JSON value from a string
     */
    JSON(const std::string& value) : value_(value) {}

    /**
     * @brief Create a JSON value from a C string
     */
    JSON(const char* value) : value_(std::string(value)) {}

    /**
     * @brief Create a JSON value from an object
     */
    JSON(const ObjectType& value) : value_(std::make_shared<ObjectType>(value)) {}

    /**
     * @brief Create a JSON value from an array
     */
    JSON(const ArrayType& value) : value_(std::make_shared<ArrayType>(value)) {}

    /**
     * @brief Check if the JSON value is null
     */
    bool is_null() const {
        return std::holds_alternative<Null>(value_);
    }

    /**
     * @brief Check if the JSON value is a boolean
     */
    bool is_boolean() const {
        return std::holds_alternative<Boolean>(value_);
    }

    /**
     * @brief Check if the JSON value is a number
     */
    bool is_number() const {
        return std::holds_alternative<Number>(value_);
    }

    /**
     * @brief Check if the JSON value is a string
     */
    bool is_string() const {
        return std::holds_alternative<String>(value_);
    }

    /**
     * @brief Check if the JSON value is an object
     */
    bool is_object() const {
        return std::holds_alternative<std::shared_ptr<ObjectType>>(value_);
    }

    /**
     * @brief Check if the JSON value is an array
     */
    bool is_array() const {
        return std::holds_alternative<std::shared_ptr<ArrayType>>(value_);
    }

    /**
     * @brief Get the JSON value as a boolean
     */
    bool as_boolean() const {
        if (!is_boolean()) {
            throw std::runtime_error("JSON value is not a boolean");
        }
        return std::get<Boolean>(value_);
    }

    /**
     * @brief Get the JSON value as a number
     */
    double as_number() const {
        if (!is_number()) {
            throw std::runtime_error("JSON value is not a number");
        }
        return std::get<Number>(value_);
    }

    /**
     * @brief Get the JSON value as a string
     */
    const std::string& as_string() const {
        if (!is_string()) {
            throw std::runtime_error("JSON value is not a string");
        }
        return std::get<String>(value_);
    }

    /**
     * @brief Get the JSON value as an object
     */
    const ObjectType& as_object() const {
        if (!is_object()) {
            throw std::runtime_error("JSON value is not an object");
        }
        return *std::get<std::shared_ptr<ObjectType>>(value_);
    }

    /**
     * @brief Get the JSON value as an array
     */
    const ArrayType& as_array() const {
        if (!is_array()) {
            throw std::runtime_error("JSON value is not an array");
        }
        return *std::get<std::shared_ptr<ArrayType>>(value_);
    }

    /**
     * @brief Access an element of an object by key
     */
    JSON& operator[](const std::string& key) {
        if (!is_object()) {
            value_ = std::make_shared<ObjectType>();
        }
        auto& obj = *std::get<std::shared_ptr<ObjectType>>(value_);
        return obj[key];
    }

    /**
     * @brief Access an element of an array by index
     */
    JSON& operator[](size_t index) {
        if (!is_array()) {
            throw std::runtime_error("JSON value is not an array");
        }
        auto& arr = *std::get<std::shared_ptr<ArrayType>>(value_);
        if (index >= arr.size()) {
            arr.resize(index + 1);
        }
        return arr[index];
    }

    /**
     * @brief Push a value to an array
     */
    void push_back(const JSON& value) {
        if (!is_array()) {
            value_ = std::make_shared<ArrayType>();
        }
        auto& arr = *std::get<std::shared_ptr<ArrayType>>(value_);
        arr.push_back(value);
    }

    /**
     * @brief Create an empty array
     */
    static JSON array() {
        return JSON(ArrayType{});
    }

    /**
     * @brief Create an empty object
     */
    static JSON object() {
        return JSON(ObjectType{});
    }

    /**
     * @brief Parse a JSON string
     */
    static JSON parse(const std::string& json_str) {
        (void)json_str;
        // Simple JSON parsing implementation
        // This is a placeholder - in a real implementation, you would parse the JSON string
        // For now, we'll just create an empty object to make the code compile
        return JSON::object();
    }

    /**
     * @brief Check if an object contains a key
     */
    bool contains(const std::string& key) const {
        if (!is_object()) {
            return false;
        }
        const auto& obj = as_object();
        return obj.find(key) != obj.end();
    }

    /**
     * @brief Get a value from an object with a default if the key doesn't exist
     */
    template<typename T>
    T value(const std::string& key, T default_value) const {
        if (!is_object()) {
            return default_value;
        }
        const auto& obj = as_object();
        auto it = obj.find(key);
        if (it == obj.end()) {
            return default_value;
        }

        // Try to convert the value to the requested type
        try {
            if constexpr (std::is_same_v<T, std::string>) {
                return it->second.is_string() ? it->second.as_string() : default_value;
            } else if constexpr (std::is_same_v<T, bool>) {
                return it->second.is_boolean() ? it->second.as_boolean() : default_value;
            } else if constexpr (std::is_arithmetic_v<T>) {
                return it->second.is_number() ? static_cast<T>(it->second.as_number()) : default_value;
            } else {
                return default_value;
            }
        } catch (...) {
            return default_value;
        }
    }

    /**
     * @brief Specialization for string literals
     */
    std::string value(const std::string& key, const char* default_value) const {
        if (!is_object()) {
            return default_value;
        }
        const auto& obj = as_object();
        auto it = obj.find(key);
        if (it == obj.end()) {
            return default_value;
        }

        try {
            return it->second.is_string() ? it->second.as_string() : default_value;
        } catch (...) {
            return default_value;
        }
    }

    /**
     * @brief Serialize the JSON value to a string
     */
    std::string dump() const {
        std::ostringstream oss;
        dump_to(oss);
        return oss.str();
    }

private:
    ValueType value_;

    /**
     * @brief Serialize the JSON value to an output stream
     */
    void dump_to(std::ostringstream& oss) const {
        if (is_null()) {
            oss << "null";
        } else if (is_boolean()) {
            oss << (as_boolean() ? "true" : "false");
        } else if (is_number()) {
            oss << as_number();
        } else if (is_string()) {
            dump_string(oss, as_string());
        } else if (is_object()) {
            dump_object(oss);
        } else if (is_array()) {
            dump_array(oss);
        }
    }

    /**
     * @brief Serialize a string to an output stream
     */
    void dump_string(std::ostringstream& oss, const std::string& str) const {
        oss << "\"";
        for (char c : str) {
            switch (c) {
                case '"': oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\b': oss << "\\b"; break;
                case '\f': oss << "\\f"; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default:
                    if (c < 32) {
                        oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    } else {
                        oss << c;
                    }
            }
        }
        oss << "\"";
    }

    /**
     * @brief Serialize an object to an output stream
     */
    void dump_object(std::ostringstream& oss) const {
        const auto& obj = as_object();
        oss << "{";
        bool first = true;
        for (const auto& [key, value] : obj) {
            if (!first) {
                oss << ",";
            }
            first = false;
            dump_string(oss, key);
            oss << ":";
            value.dump_to(oss);
        }
        oss << "}";
    }

    /**
     * @brief Serialize an array to an output stream
     */
    void dump_array(std::ostringstream& oss) const {
        const auto& arr = as_array();
        oss << "[";
        bool first = true;
        for (const auto& value : arr) {
            if (!first) {
                oss << ",";
            }
            first = false;
            value.dump_to(oss);
        }
        oss << "]";
    }
};

} // namespace bitscrape::web
