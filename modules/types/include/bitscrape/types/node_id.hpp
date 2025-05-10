#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <future>
#include <functional>

namespace bitscrape::types
{

    /**
     * @brief Represents a 160-bit DHT node ID
     *
     * NodeID is a 160-bit identifier used in the DHT network to identify nodes.
     * It provides methods for creating, comparing, and manipulating node IDs.
     */
    class NodeID
    {
    public:
        /// Size of a NodeID in bytes (160 bits = 20 bytes)
        static constexpr size_t SIZE = 20;

        /// Type for the internal storage of the NodeID
        using IDStorage = std::array<uint8_t, SIZE>;

        /**
         * @brief Default constructor creates a random NodeID
         */
        NodeID();

        /**
         * @brief Create a NodeID from a byte array
         *
         * @param bytes Array of bytes representing the NodeID
         */
        explicit NodeID(const IDStorage &bytes);

        /**
         * @brief Create a NodeID from a byte vector
         *
         * @param bytes Vector of bytes representing the NodeID
         * @throws std::invalid_argument if the vector size is not SIZE
         */
        explicit NodeID(const std::vector<uint8_t> &bytes);

        /**
         * @brief Create a NodeID from a hexadecimal string
         *
         * @param hex Hexadecimal string representing the NodeID
         * @throws std::invalid_argument if the string is not a valid hex string or wrong length
         */
        explicit NodeID(const std::string &hex);

        /**
         * @brief Create a NodeID from a string_view
         *
         * @param hex String view representing the NodeID in hexadecimal
         * @throws std::invalid_argument if the string is not a valid hex string or wrong length
         */
        explicit NodeID(std::string_view hex);

        /**
         * @brief Copy constructor
         */
        NodeID(const NodeID &other) = default;

        /**
         * @brief Move constructor
         */
        NodeID(NodeID &&other) noexcept = default;

        /**
         * @brief Copy assignment operator
         */
        NodeID &operator=(const NodeID &other) = default;

        /**
         * @brief Move assignment operator
         */
        NodeID &operator=(NodeID &&other) noexcept = default;

        /**
         * @brief Destructor
         */
        ~NodeID() = default;

        /**
         * @brief Get the raw bytes of the NodeID
         *
         * @return Const reference to the internal byte array
         */
        const IDStorage &bytes() const { return id_; }

        /**
         * @brief Get the NodeID as a hexadecimal string
         *
         * @return Hexadecimal string representation of the NodeID
         */
        std::string to_hex() const;

        /**
         * @brief Calculate the distance between this NodeID and another
         *
         * The distance is calculated as the XOR of the two NodeIDs.
         *
         * @param other The other NodeID
         * @return A new NodeID representing the distance
         */
        NodeID distance(const NodeID &other) const;

        /**
         * @brief Calculate the distance between this NodeID and another asynchronously
         *
         * @param other The other NodeID
         * @return A future that will contain the distance NodeID
         */
        std::future<NodeID> distance_async(const NodeID &other) const;

        /**
         * @brief Generate a random NodeID
         *
         * @return A new random NodeID
         */
        static NodeID random();

        /**
         * @brief Generate a random NodeID asynchronously
         *
         * @return A future that will contain a random NodeID
         */
        static std::future<NodeID> random_async();

        /**
         * @brief Generate a cryptographically secure random NodeID
         *
         * @return A new secure random NodeID
         */
        static NodeID secure_random();

        /**
         * @brief Generate a cryptographically secure random NodeID asynchronously
         *
         * @return A future that will contain a secure random NodeID
         */
        static std::future<NodeID> secure_random_async();

        /**
         * @brief Equality operator
         */
        bool operator==(const NodeID &other) const;

        /**
         * @brief Inequality operator
         */
        bool operator!=(const NodeID &other) const;

        /**
         * @brief Less than operator for ordering
         */
        bool operator<(const NodeID &other) const;

        /**
         * @brief Greater than operator for ordering
         */
        bool operator>(const NodeID &other) const;

        /**
         * @brief Less than or equal operator for ordering
         */
        bool operator<=(const NodeID &other) const;

        /**
         * @brief Greater than or equal operator for ordering
         */
        bool operator>=(const NodeID &other) const;

    private:
        IDStorage id_; ///< The internal storage for the NodeID
    };

} // namespace bitscrape::types
