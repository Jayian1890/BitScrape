#!/bin/bash

# This script updates all test files from Catch2 to Google Test format

# Function to convert a test file
convert_test_file() {
  local file=$1
  echo "Converting $file..."

  # Create a temporary file
  tmp_file=$(mktemp)

  # Get the base name of the file without extension
  base_name=$(basename "$file" .cpp)
  # Remove _test suffix if present
  test_name=${base_name%_test}
  # Convert to CamelCase
  test_suite_name=$(echo "$test_name" | sed -E 's/(^|_)([a-z])/\U\2/g')Test

  # Read the file line by line
  while IFS= read -r line; do
    # Check if the line contains a TEST macro with the old format
    if [[ $line =~ ^[[:space:]]*TEST\([[:space:]]*\"([^\"]+)\"[[:space:]]*,[[:space:]]*\"[^\"]+\"[[:space:]]*\) ]]; then
      # Extract the test name
      old_test_name="${BASH_REMATCH[1]}"
      # Convert to CamelCase
      new_test_name=$(echo "$old_test_name" | sed -E 's/(^|[[:space:]])([a-z])/\U\2/g' | sed 's/[[:space:]]//g')

      # Replace the TEST macro
      echo "TEST($test_suite_name, $new_test_name) {" >> "$tmp_file"
    elif [[ $line =~ EXPECT_TRUE_FALSE ]]; then
      # Replace EXPECT_TRUE_FALSE with EXPECT_FALSE
      echo "${line/EXPECT_TRUE_FALSE/EXPECT_FALSE}" >> "$tmp_file"
    elif [[ $line =~ EXPECT_TRUE_THROWS_AS ]]; then
      # Replace EXPECT_TRUE_THROWS_AS with EXPECT_THROW
      echo "${line/EXPECT_TRUE_THROWS_AS/EXPECT_THROW}" >> "$tmp_file"
    elif [[ $line =~ "EXPECT_TRUE" && $line =~ "==" ]]; then
      # Replace EXPECT_TRUE(a == b) with EXPECT_EQ(a, b)
      echo "${line/EXPECT_TRUE/EXPECT_EQ}" >> "$tmp_file"
    elif [[ $line =~ "EXPECT_TRUE" && $line =~ "!=" ]]; then
      # Replace EXPECT_TRUE(a != b) with EXPECT_NE(a, b)
      echo "${line/EXPECT_TRUE/EXPECT_NE}" >> "$tmp_file"
    elif [[ $line =~ "EXPECT_TRUE" && $line =~ "<" ]]; then
      # Replace EXPECT_TRUE(a < b) with EXPECT_LT(a, b)
      echo "${line/EXPECT_TRUE/EXPECT_LT}" >> "$tmp_file"
    elif [[ $line =~ "EXPECT_TRUE" && $line =~ ">" ]]; then
      # Replace EXPECT_TRUE(a > b) with EXPECT_GT(a, b)
      echo "${line/EXPECT_TRUE/EXPECT_GT}" >> "$tmp_file"
    elif [[ $line =~ WARN ]]; then
      # Replace WARN with std::cerr and SUCCEED
      echo "${line/WARN/std::cerr << }" >> "$tmp_file"
      echo "      SUCCEED();" >> "$tmp_file"
    elif [[ $line =~ \/\/[[:space:]]*\/\/[[:space:]]*SECTION ]]; then
      # Comment out the SECTION lines
      echo "  // ${line#//}" >> "$tmp_file"
    else
      # Keep the line as is
      echo "$line" >> "$tmp_file"
    fi
  done < "$file"

  # Move the temporary file back to the original file
  mv "$tmp_file" "$file"
}

# Find all test files
find bitscrape/modules -name "*_test.cpp" | while read -r file; do
  convert_test_file "$file"
done

echo "All test files have been converted."
