#include "gtest/gtest.h"
#include "lubm8.cpp"

TEST(LUBM8, LUBM) {
  NUM_THREADS = 4;
  std::unordered_map<std::string, void *> relations;
  std::unordered_map<std::string, Trie<hybrid> *> tries;
  std::unordered_map<std::string, std::vector<void *> *> encodings;
  long result = run(relations, tries, encodings);
  EXPECT_EQ(5920, result);
}