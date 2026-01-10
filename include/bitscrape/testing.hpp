#pragma once

#include <doctest.h>

// GoogleTest-like API mapped to doctest macros for convenience
// This allows test code to use familiar GoogleTest-style assertions while using doctest

// Test definition macros
#define TEST(suite_name, test_name) TEST_CASE(#suite_name "::" #test_name)
#define TEST_F(fixture_name, test_name) TEST_CASE(#fixture_name "::" #test_name)

// Assertion macros - basic checks
#define ASSERT_TRUE(condition) REQUIRE(condition)
#define ASSERT_FALSE(condition) REQUIRE(!(condition))
#define EXPECT_TRUE(condition) CHECK(condition)
#define EXPECT_FALSE(condition) CHECK(!(condition))

// Assertion macros - equality
#define ASSERT_EQ(expected, actual) REQUIRE((actual) == (expected))
#define ASSERT_NE(expected, actual) REQUIRE((actual) != (expected))
#define ASSERT_LT(expected, actual) REQUIRE((actual) < (expected))
#define ASSERT_LE(expected, actual) REQUIRE((actual) <= (expected))
#define ASSERT_GT(expected, actual) REQUIRE((actual) > (expected))
#define ASSERT_GE(expected, actual) REQUIRE((actual) >= (expected))

#define EXPECT_EQ(expected, actual) CHECK((actual) == (expected))
#define EXPECT_NE(expected, actual) CHECK((actual) != (expected))
#define EXPECT_LT(expected, actual) CHECK((actual) < (expected))
#define EXPECT_LE(expected, actual) CHECK((actual) <= (expected))
#define EXPECT_GT(expected, actual) CHECK((actual) > (expected))
#define EXPECT_GE(expected, actual) CHECK((actual) >= (expected))

// String comparison macros
#define ASSERT_STREQ(expected, actual) REQUIRE(std::string(actual) == std::string(expected))
#define EXPECT_STREQ(expected, actual) CHECK(std::string(actual) == std::string(expected))

// Throw expectation macros
#define ASSERT_THROW(statement, exception_type) REQUIRE_THROWS_AS(statement, exception_type)
#define EXPECT_THROW(statement, exception_type) CHECK_THROWS_AS(statement, exception_type)
#define ASSERT_NO_THROW(statement) REQUIRE_NOTHROW(statement)
#define EXPECT_NO_THROW(statement) CHECK_NOTHROW(statement)

// Note: FAIL macro is already defined by doctest.h
