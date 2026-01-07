#pragma once

// Lightweight testing shim: maps a subset of GoogleTest style APIs onto doctest
// so existing tests can keep their structure without pulling in gtest/gmock.

#include <doctest/doctest.h>

namespace testing {
class Test {
public:
    virtual ~Test() = default;
    virtual void SetUp() {}
    virtual void TearDown() {}
};

template <typename T>
struct TestWrapper : public T {
    TestWrapper() { T::SetUp(); }
    ~TestWrapper() override { T::TearDown(); }
};

inline void InitGoogleTest(int*, char**) {}
inline int RUN_ALL_TESTS() {
    doctest::Context context;
    return context.run();
}

} // namespace testing

#define TEST(test_case_name, test_name) \
    DOCTEST_TEST_CASE(#test_case_name "." #test_name)

#define TEST_F(fixture, test_name) \
    DOCTEST_TEST_CASE_FIXTURE(::testing::TestWrapper<fixture>, #fixture "." #test_name)

#define EXPECT_TRUE(condition) DOCTEST_CHECK(condition)
#define ASSERT_TRUE(condition) DOCTEST_REQUIRE(condition)
#define EXPECT_FALSE(condition) DOCTEST_CHECK_FALSE(condition)
#define ASSERT_FALSE(condition) DOCTEST_REQUIRE_FALSE(condition)

#define EXPECT_EQ(val1, val2) DOCTEST_CHECK_EQ(val1, val2)
#define ASSERT_EQ(val1, val2) DOCTEST_REQUIRE_EQ(val1, val2)
#define EXPECT_NE(val1, val2) DOCTEST_CHECK_NE(val1, val2)
#define ASSERT_NE(val1, val2) DOCTEST_REQUIRE_NE(val1, val2)
#define EXPECT_LT(val1, val2) DOCTEST_CHECK_LT(val1, val2)
#define ASSERT_LT(val1, val2) DOCTEST_REQUIRE_LT(val1, val2)
#define EXPECT_GT(val1, val2) DOCTEST_CHECK_GT(val1, val2)
#define ASSERT_GT(val1, val2) DOCTEST_REQUIRE_GT(val1, val2)
#define EXPECT_LE(val1, val2) DOCTEST_CHECK_LE(val1, val2)
#define ASSERT_LE(val1, val2) DOCTEST_REQUIRE_LE(val1, val2)
#define EXPECT_GE(val1, val2) DOCTEST_CHECK_GE(val1, val2)
#define ASSERT_GE(val1, val2) DOCTEST_REQUIRE_GE(val1, val2)

#define EXPECT_THROW(statement, exception_type) DOCTEST_CHECK_THROWS_AS(statement, exception_type)
#define ASSERT_THROW(statement, exception_type) DOCTEST_REQUIRE_THROWS_AS(statement, exception_type)
#define EXPECT_ANY_THROW(statement) DOCTEST_CHECK_THROWS(statement)
#define ASSERT_ANY_THROW(statement) DOCTEST_REQUIRE_THROWS(statement)
#define EXPECT_NO_THROW(statement) DOCTEST_CHECK_NOTHROW(statement)
#define ASSERT_NO_THROW(statement) DOCTEST_REQUIRE_NOTHROW(statement)

#define SUCCEED() DOCTEST_MESSAGE("SUCCEED")
