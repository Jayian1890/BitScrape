#include <catch2/catch.hpp>

#include "bitscrape/types/endpoint.hpp"

using namespace bitscrape::types;

TEST_CASE("Endpoint construction", "[types][endpoint]") {
    SECTION("Default constructor creates an invalid endpoint") {
        Endpoint ep;
        
        REQUIRE_FALSE(ep.is_valid());
    }
    
    SECTION("Construction from IPv4 address and port") {
        Endpoint ep("192.168.1.1", 6881);
        
        REQUIRE(ep.address() == "192.168.1.1");
        REQUIRE(ep.port() == 6881);
        REQUIRE(ep.type() == Endpoint::AddressType::IPv4);
        REQUIRE(ep.is_valid());
    }
    
    SECTION("Construction from IPv6 address and port") {
        Endpoint ep("2001:db8::1", 6881);
        
        REQUIRE(ep.address() == "2001:db8::1");
        REQUIRE(ep.port() == 6881);
        REQUIRE(ep.type() == Endpoint::AddressType::IPv6);
        REQUIRE(ep.is_valid());
    }
    
    SECTION("Construction from string_view") {
        std::string_view address = "192.168.1.1";
        
        Endpoint ep(address, 6881);
        
        REQUIRE(ep.address() == "192.168.1.1");
        REQUIRE(ep.port() == 6881);
        REQUIRE(ep.type() == Endpoint::AddressType::IPv4);
        REQUIRE(ep.is_valid());
    }
    
    SECTION("Invalid IP address") {
        REQUIRE_THROWS_AS(Endpoint("invalid", 6881), std::invalid_argument);
    }
    
    SECTION("Resolve hostname") {
        // This test may fail if the host is not reachable
        // or if the DNS resolution fails
        try {
            Endpoint ep = Endpoint::resolve("localhost", 6881);
            
            REQUIRE(ep.is_valid());
            REQUIRE(ep.port() == 6881);
            REQUIRE(ep.type() == Endpoint::AddressType::IPv4);
        } catch (const std::exception& e) {
            // Skip the test if resolution fails
            WARN("Skipping hostname resolution test: " << e.what());
        }
    }
}

TEST_CASE("Endpoint operations", "[types][endpoint]") {
    Endpoint ep1("192.168.1.1", 6881);
    Endpoint ep2("192.168.1.2", 6881);
    Endpoint ep3("192.168.1.1", 6882);
    
    SECTION("to_string") {
        REQUIRE(ep1.to_string() == "192.168.1.1:6881");
        
        Endpoint ep_ipv6("2001:db8::1", 6881);
        REQUIRE(ep_ipv6.to_string() == "[2001:db8::1]:6881");
    }
    
    SECTION("resolve_async") {
        // This test may fail if the host is not reachable
        // or if the DNS resolution fails
        try {
            auto future = Endpoint::resolve_async("localhost", 6881);
            Endpoint ep = future.get();
            
            REQUIRE(ep.is_valid());
            REQUIRE(ep.port() == 6881);
            REQUIRE(ep.type() == Endpoint::AddressType::IPv4);
        } catch (const std::exception& e) {
            // Skip the test if resolution fails
            WARN("Skipping async hostname resolution test: " << e.what());
        }
    }
    
    SECTION("comparison operators") {
        REQUIRE(ep1 == ep1);
        REQUIRE(ep1 != ep2);
        REQUIRE(ep1 != ep3);
        REQUIRE(ep1 < ep2);
        REQUIRE(ep1 < ep3);
    }
}
