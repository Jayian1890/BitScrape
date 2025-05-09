#!/bin/bash

# Update all test files to use Google Test instead of Catch2
for file in modules/*/tests/unit/*.cpp; do
  echo "Checking $file"
  
  # Skip files that are already updated
  if grep -q "TEST(.*," "$file"; then
    echo "  Already using Google Test format, skipping"
    continue
  fi
  
  echo "  Updating $file"
  
  # Create a temporary file
  tmp_file=$(mktemp)
  
  # Replace the TEST macro with proper Google Test format
  sed 's/TEST(\("[^"]*"\), \("[^"]*"\))/TEST(TestSuite, Test)/' "$file" > "$tmp_file"
  
  # Replace EXPECT_TRUE_FALSE with EXPECT_FALSE
  sed -i '' 's/EXPECT_TRUE_FALSE/EXPECT_FALSE/g' "$tmp_file"
  
  # Replace EXPECT_TRUE_THROWS_AS with EXPECT_THROW
  sed -i '' 's/EXPECT_TRUE_THROWS_AS/EXPECT_THROW/g' "$tmp_file"
  
  # Replace EXPECT_TRUE(x == y) with EXPECT_EQ(x, y)
  sed -i '' 's/EXPECT_TRUE(\([^)]*\) == \([^)]*\))/EXPECT_EQ(\1, \2)/g' "$tmp_file"
  
  # Replace EXPECT_TRUE(x != y) with EXPECT_NE(x, y)
  sed -i '' 's/EXPECT_TRUE(\([^)]*\) != \([^)]*\))/EXPECT_NE(\1, \2)/g' "$tmp_file"
  
  # Replace EXPECT_TRUE(x < y) with EXPECT_LT(x, y)
  sed -i '' 's/EXPECT_TRUE(\([^)]*\) < \([^)]*\))/EXPECT_LT(\1, \2)/g' "$tmp_file"
  
  # Replace EXPECT_TRUE(x > y) with EXPECT_GT(x, y)
  sed -i '' 's/EXPECT_TRUE(\([^)]*\) > \([^)]*\))/EXPECT_GT(\1, \2)/g' "$tmp_file"
  
  # Replace WARN with std::cerr and SUCCEED
  sed -i '' 's/WARN(\(.*\))/std::cerr << \1 << std::endl; SUCCEED()/g' "$tmp_file"
  
  # Replace commented SECTION with actual comments
  sed -i '' 's/\/\/ \/\/ SECTION/\/\/ SECTION/g' "$tmp_file"
  
  # Move the updated file back
  mv "$tmp_file" "$file"
done

echo "Done updating test files"
