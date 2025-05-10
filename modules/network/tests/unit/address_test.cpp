#include "bitscrape/network/address.hpp"

#include <gtest/gtest.h>

namespace bitscrape::network::test {

TEST(AddressTest, ConstructWithPort) {
    Address address(8080);
    EXPECT_EQ(address.port(), 8080);
    EXPECT_EQ(address.family(), AddressFamily::IPv4);
    EXPECT_EQ(address.to_string(), "0.0.0.0");
    EXPECT_TRUE(address.is_valid());
}

TEST(AddressTest, ConstructWithAddressAndPort) {
    Address address("127.0.0.1", 8080);
    EXPECT_EQ(address.port(), 8080);
    EXPECT_EQ(address.family(), AddressFamily::IPv4);
    EXPECT_EQ(address.to_string(), "127.0.0.1");
    EXPECT_TRUE(address.is_valid());
}

TEST(AddressTest, ConstructWithIPv6AddressAndPort) {
    Address address("::1", 8080);
    EXPECT_EQ(address.port(), 8080);
    EXPECT_EQ(address.family(), AddressFamily::IPv6);
    EXPECT_EQ(address.to_string(), "::1");
    EXPECT_TRUE(address.is_valid());
}

TEST(AddressTest, InvalidAddress) {
    Address address("invalid", 8080);
    EXPECT_EQ(address.port(), 8080);
    EXPECT_FALSE(address.is_valid());
}

TEST(AddressTest, IsLoopback) {
    Address address1("127.0.0.1", 8080);
    EXPECT_TRUE(address1.is_loopback());

    Address address2("::1", 8080);
    EXPECT_TRUE(address2.is_loopback());

    Address address3("192.168.1.1", 8080);
    EXPECT_FALSE(address3.is_loopback());
}

TEST(AddressTest, IsMulticast) {
    Address address1("224.0.0.1", 8080);
    EXPECT_TRUE(address1.is_multicast());

    Address address2("ff02::1", 8080);
    EXPECT_TRUE(address2.is_multicast());

    Address address3("192.168.1.1", 8080);
    EXPECT_FALSE(address3.is_multicast());
}

TEST(AddressTest, Resolve) {
    try {
        Address address = Address::resolve("localhost", 8080);
        EXPECT_TRUE(address.is_valid());
        EXPECT_EQ(address.port(), 8080);
        EXPECT_TRUE(address.is_loopback());
    } catch (const std::exception& e) {
        // Resolve might fail in some environments, so we'll skip the test
        GTEST_SKIP() << "Skipping test due to resolve failure: " << e.what();
    }
}

TEST(AddressTest, ResolveAsync) {
    try {
        auto future = Address::resolve_async("localhost", 8080);
        Address address = future.get();
        EXPECT_TRUE(address.is_valid());
        EXPECT_EQ(address.port(), 8080);
        EXPECT_TRUE(address.is_loopback());
    } catch (const std::exception& e) {
        // Resolve might fail in some environments, so we'll skip the test
        GTEST_SKIP() << "Skipping test due to resolve failure: " << e.what();
    }
}

TEST(AddressTest, GetLocalAddress) {
    try {
        Address address = Address::get_local_address("", 8080);
        EXPECT_TRUE(address.is_valid());
        EXPECT_EQ(address.port(), 8080);
    } catch (const std::exception& e) {
        // Getting local address might fail in some environments, so we'll skip the test
        GTEST_SKIP() << "Skipping test due to get_local_address failure: " << e.what();
    }
}

TEST(AddressTest, GetAllLocalAddresses) {
    try {
        std::vector<Address> addresses = Address::get_all_local_addresses(8080);
        EXPECT_FALSE(addresses.empty());
        
        for (const auto& address : addresses) {
            EXPECT_TRUE(address.is_valid());
            EXPECT_EQ(address.port(), 8080);
        }
    } catch (const std::exception& e) {
        // Getting all local addresses might fail in some environments, so we'll skip the test
        GTEST_SKIP() << "Skipping test due to get_all_local_addresses failure: " << e.what();
    }
}

} // namespace bitscrape::network::test
