#!/bin/bash

# Update all test files to use Google Test instead of Catch2
for file in tests/unit/*.cpp; do
  if [ "$file" != "tests/unit/node_id_test.cpp" ]; then
    echo "Updating $file"
    sed -i '' 's/#include <catch2\/catch_test_macros.hpp>/#include <gtest\/gtest.h>/g' "$file"
    sed -i '' 's/TEST_CASE/TEST/g' "$file"
    sed -i '' 's/SECTION/\/\/ SECTION/g' "$file"
    sed -i '' 's/REQUIRE/EXPECT_TRUE/g' "$file"
    sed -i '' 's/REQUIRE_FALSE/EXPECT_FALSE/g' "$file"
    sed -i '' 's/REQUIRE_THROWS_AS/EXPECT_THROW/g' "$file"
  fi
done

echo "Done updating test files"
