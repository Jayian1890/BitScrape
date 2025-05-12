#include "bitscrape/types/node_id.hpp"

#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

namespace bitscrape::types
{

    NodeID::NodeID()
    {
        // Generate a random NodeID
        *this = random();
    }

    NodeID::NodeID(const IDStorage &bytes) : id_(bytes)
    {
    }

    NodeID::NodeID(const std::vector<uint8_t> &bytes)
    {
        if (bytes.size() != SIZE)
        {
            throw std::invalid_argument("NodeID: Invalid byte vector size");
        }

        std::copy(bytes.begin(), bytes.end(), id_.begin());
    }

    NodeID::NodeID(const std::string &hex)
    {
        if (hex.size() != SIZE * 2)
        {
            throw std::invalid_argument("NodeID: Invalid hex string length");
        }

        for (size_t i = 0; i < SIZE; ++i)
        {
            std::string byte_str = hex.substr(i * 2, 2);
            try
            {
                id_[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
            }
            catch (const std::exception &e)
            {
                throw std::invalid_argument("NodeID: Invalid hex string");
            }
        }
    }

    NodeID::NodeID(std::string_view hex)
    {
        if (hex.size() != SIZE * 2)
        {
            throw std::invalid_argument("NodeID: Invalid hex string length");
        }

        for (size_t i = 0; i < SIZE; ++i)
        {
            std::string byte_str(hex.substr(i * 2, 2));
            try
            {
                id_[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
            }
            catch (const std::exception &e)
            {
                throw std::invalid_argument("NodeID: Invalid hex string");
            }
        }
    }

    std::string NodeID::to_hex() const
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (auto byte : id_)
        {
            oss << std::setw(2) << static_cast<int>(byte);
        }

        return oss.str();
    }

    NodeID NodeID::distance(const NodeID &other) const
    {
        IDStorage result;

        for (size_t i = 0; i < SIZE; ++i)
        {
            result[i] = id_[i] ^ other.id_[i];
        }

        return NodeID(result);
    }

    std::future<NodeID> NodeID::distance_async(const NodeID &other) const
    {
        return std::async(std::launch::async, [this, other]()
                          { return this->distance(other); });
    }

    NodeID NodeID::random()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255);

        IDStorage bytes;
        for (auto &byte : bytes)
        {
            byte = dist(gen);
        }

        return NodeID(bytes);
    }

    std::future<NodeID> NodeID::random_async()
    {
        return std::async(std::launch::async, []()
                          { return secure_random(); });
    }

    NodeID NodeID::secure_random()
    {
        IDStorage bytes;

#ifdef _WIN32
        // Windows implementation using BCryptGenRandom
        BCRYPT_ALG_HANDLE hAlgorithm;
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_RNG_ALGORITHM, NULL, 0);
        if (BCRYPT_SUCCESS(status))
        {
            status = BCryptGenRandom(hAlgorithm, bytes.data(), bytes.size(), 0);
            BCryptCloseAlgorithmProvider(hAlgorithm, 0);
            if (!BCRYPT_SUCCESS(status))
            {
                throw std::runtime_error("Failed to generate secure random bytes");
            }
        }
        else
        {
            throw std::runtime_error("Failed to open cryptographic provider");
        }
#elif defined(__APPLE__)
        // macOS implementation using arc4random
        for (size_t i = 0; i < SIZE; ++i)
        {
            bytes[i] = static_cast<uint8_t>(arc4random() & 0xFF);
        }
#else
        // Linux/Unix implementation using /dev/urandom
        std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);
        if (!urandom)
        {
            throw std::runtime_error("Failed to open /dev/urandom");
        }
        urandom.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        if (urandom.gcount() != static_cast<std::streamsize>(bytes.size()))
        {
            throw std::runtime_error("Failed to read enough random bytes");
        }
#endif

        return NodeID(bytes);
    }

    std::future<NodeID> NodeID::secure_random_async()
    {
        return std::async(std::launch::async, []()
                          { return secure_random(); });
    }

    bool NodeID::operator==(const NodeID &other) const
    {
        return id_ == other.id_;
    }

    bool NodeID::operator!=(const NodeID &other) const
    {
        return !(*this == other);
    }

    bool NodeID::operator<(const NodeID &other) const
    {
        return id_ < other.id_;
    }

    bool NodeID::operator>(const NodeID &other) const
    {
        return other < *this;
    }

    bool NodeID::operator<=(const NodeID &other) const
    {
        return !(other < *this);
    }

    bool NodeID::operator>=(const NodeID &other) const
    {
        return !(*this < other);
    }

    bool NodeID::is_bit_set(size_t bit_index) const
    {
        if (bit_index >= SIZE * 8) {
            throw std::out_of_range("Bit index out of range");
        }

        // Calculate the byte index and bit position within the byte
        size_t byte_index = bit_index / 8;
        size_t bit_position = 7 - (bit_index % 8); // Most significant bit first

        // Check if the bit is set
        return (id_[byte_index] & (1 << bit_position)) != 0;
    }

} // namespace bitscrape::types
