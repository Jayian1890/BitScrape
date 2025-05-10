#include <gtest/gtest.h>

#include "bitscrape/types/endpoint.hpp"

using namespace bitscrape::types;

TEST(EndpointTest, DefaultConstructor) {
  Endpoint ep;
  EXPECT_FALSE(ep.is_valid());
}

TEST(EndpointTest, ConstructionFromIPv4AddressAndPort) {
  Endpoint ep(std::string("192.168.1.1"), 6881);

  EXPECT_EQ(ep.address(), "192.168.1.1");
  EXPECT_EQ(ep.port(), 6881);
  EXPECT_EQ(ep.type(), Endpoint::AddressType::IPv4);
  EXPECT_TRUE(ep.is_valid());
}

TEST(EndpointTest, ConstructionFromIPv6AddressAndPort) {
  Endpoint ep(std::string("2001:db8::1"), 6881);

  EXPECT_EQ(ep.address(), "2001:db8::1");
  EXPECT_EQ(ep.port(), 6881);
  EXPECT_EQ(ep.type(), Endpoint::AddressType::IPv6);
  EXPECT_TRUE(ep.is_valid());
}

TEST(EndpointTest, ConstructionFromStringView) {
  std::string_view address = "192.168.1.1";
  Endpoint ep(address, 6881);

  EXPECT_EQ(ep.address(), "192.168.1.1");
  EXPECT_EQ(ep.port(), 6881);
  EXPECT_EQ(ep.type(), Endpoint::AddressType::IPv4);
  EXPECT_TRUE(ep.is_valid());
}

TEST(EndpointTest, InvalidIPAddress) {
  EXPECT_THROW(Endpoint(std::string("invalid"), 6881), std::invalid_argument);
}

TEST(EndpointTest, ResolveHostname) {
  // This test may fail if the host is not reachable
  // or if the DNS resolution fails
  try {
    Endpoint ep = Endpoint::resolve("localhost", 6881);

    EXPECT_TRUE(ep.is_valid());
    EXPECT_EQ(ep.port(), 6881);
    EXPECT_EQ(ep.type(), Endpoint::AddressType::IPv4);
  } catch (const std::exception &e) {
    // Skip the test if resolution fails
    std::cerr << "Skipping hostname resolution test: " << e.what() << std::endl;
    SUCCEED();
  }
}

TEST(EndpointTest, ToString) {
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  EXPECT_EQ(ep1.to_string(), "192.168.1.1:6881");

  Endpoint ep_ipv6(std::string("2001:db8::1"), 6881);
  EXPECT_EQ(ep_ipv6.to_string(), "[2001:db8::1]:6881");
}

TEST(EndpointTest, ResolveAsync) {
  // This test may fail if the host is not reachable
  // or if the DNS resolution fails
  try {
    auto future = Endpoint::resolve_async("localhost", 6881);
    Endpoint ep = future.get();

    EXPECT_TRUE(ep.is_valid());
    EXPECT_EQ(ep.port(), 6881);
    EXPECT_EQ(ep.type(), Endpoint::AddressType::IPv4);
  } catch (const std::exception &e) {
    // Skip the test if resolution fails
    std::cerr << "Skipping async hostname resolution test: " << e.what()
              << std::endl;
    SUCCEED();
  }
}

TEST(EndpointTest, ComparisonOperators) {
  Endpoint ep1(std::string("192.168.1.1"), 6881);
  Endpoint ep2(std::string("192.168.1.2"), 6881);
  Endpoint ep3(std::string("192.168.1.1"), 6882);

  EXPECT_EQ(ep1, ep1);
  EXPECT_NE(ep1, ep2);
  EXPECT_NE(ep1, ep3);
  EXPECT_LT(ep1, ep2);
  EXPECT_LT(ep1, ep3);
}
