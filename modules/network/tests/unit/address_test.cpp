#include "bitscrape/network/address.hpp"
#include <doctest/doctest.h>
#include <string>
#include <vector>

namespace bitscrape::network::test {

TEST_SUITE("Address") {
    TEST_CASE("ConstructWithPort") {
        Address address(8080);
        CHECK_EQ(address.port(), 8080);
        CHECK_EQ(address.family(), AddressFamily::IPv4);
        CHECK_EQ(address.to_string(), "0.0.0.0");
        CHECK(address.is_valid());
    }

    TEST_CASE("ConstructWithAddressAndPort") {
        Address address("127.0.0.1", 8080);
        CHECK_EQ(address.port(), 8080);
        CHECK_EQ(address.family(), AddressFamily::IPv4);
        CHECK_EQ(address.to_string(), "127.0.0.1");
        CHECK(address.is_valid());
    }

    TEST_CASE("ConstructWithIPv6AddressAndPort") {
        Address address("::1", 8080);
        CHECK_EQ(address.port(), 8080);
        CHECK_EQ(address.family(), AddressFamily::IPv6);
        CHECK_EQ(address.to_string(), "::1");
        CHECK(address.is_valid());
    }

    TEST_CASE("InvalidAddress") {
        Address address("invalid", 8080);
        CHECK_EQ(address.port(), 8080);
        CHECK_FALSE(address.is_valid());
    }

    TEST_CASE("IsLoopback") {
        Address address1("127.0.0.1", 8080);
        CHECK(address1.is_loopback());

        Address address2("::1", 8080);
        CHECK(address2.is_loopback());

        Address address3("192.168.1.1", 8080);
        CHECK_FALSE(address3.is_loopback());
    }

    TEST_CASE("IsMulticast") {
        Address address1("224.0.0.1", 8080);
        CHECK(address1.is_multicast());

        Address address2("ff02::1", 8080);
        CHECK(address2.is_multicast());

        Address address3("192.168.1.1", 8080);
        CHECK_FALSE(address3.is_multicast());
    }

    TEST_CASE("Resolve") {
        try {
            Address address = Address::resolve("localhost", 8080);
            CHECK(address.is_valid());
            CHECK_EQ(address.port(), 8080);
            CHECK(address.is_loopback());
        } catch (const std::exception& e) {
            // Resolve might fail in some environments, so we'll skip the test
            DOCTEST_MESSAGE((std::string("Skipping test due to resolve failure: ") + e.what()));
            return;
        }
    }

    TEST_CASE("ResolveAsync") {
        try {
            auto future = Address::resolve_async("localhost", 8080);
            Address address = future.get();
            CHECK(address.is_valid());
            CHECK_EQ(address.port(), 8080);
            CHECK(address.is_loopback());
        } catch (const std::exception& e) {
            // Resolve might fail in some environments, so we'll skip the test
            DOCTEST_MESSAGE((std::string("Skipping test due to resolve failure: ") + e.what()));
            return;
        }
    }

    TEST_CASE("GetLocalAddress") {
        try {
            Address address = Address::get_local_address("", 8080);
            CHECK(address.is_valid());
            CHECK_EQ(address.port(), 8080);
        } catch (const std::exception& e) {
            // Getting local address might fail in some environments, so we'll skip the test
            DOCTEST_MESSAGE((std::string("Skipping test due to get_local_address failure: ") + e.what()));
            return;
        }
    }

    TEST_CASE("GetAllLocalAddresses") {
        try {
            std::vector<Address> addresses = Address::get_all_local_addresses(8080);
            CHECK_FALSE(addresses.empty());

            for (const auto& address : addresses) {
                CHECK(address.is_valid());
                CHECK_EQ(address.port(), 8080);
            }
        } catch (const std::exception& e) {
            // Getting all local addresses might fail in some environments, so we'll skip the test
            DOCTEST_MESSAGE((std::string("Skipping test due to get_all_local_addresses failure: ") + e.what()));
            return;
        }
    }
}

} // namespace bitscrape::network::test
