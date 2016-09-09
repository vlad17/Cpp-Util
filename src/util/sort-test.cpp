/*
  Vladimir Feinberg
  util/sort-test.cpp
  2015-04-07

  Tests various sorting algorithms.
*/

#include <algorithm>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "util/radix.hpp"
#include "util/line_wrap.hpp"
#include "util/timer.hpp"
#include "util/uassert.hpp"
#include "util/util.hpp"

using namespace std;
using namespace util;

namespace {

template<class E, class S>
void test_sort(const E& examples, const S& sorter) {
  for (const auto& i : examples) {
    auto user = i;
    auto sys = i;
    sorter(user.begin(), user.end());
    std::sort(sys.begin(), sys.end());
    bool equal = std::equal(user.begin(), user.end(), sys.begin());
    UASSERT(equal) << "User sort (1) <> Sys sort (2)\n"
                   << "\t(1) " << container_print(user) << "\n"
                   << "\t(2) " << container_print(sys) << "\n";
  }
}

vector<vector<string> > string_examples = {
  {}, {""}, {"", ""}, {"a", "a"}, {"a", "", "a"},
  {"ccc", "aaa", "bab", "bbc", "ccc", "ccc", "cdb", "cbd"},
  {"aa1", "bb2", "cc3"}, {"1", "22", "333"},
  {"hello", "!", "my name", "is", "what?", "my", "name", "is", "who?",
   "my", "name is", "..."}
};

vector<vector<uint32_t> > uint32_examples = {
  {}, {0}, {0, 0}, {1, 1}, {1, 0, 1},
  {3, 1, 43, 1, 12345, 1233, 1234, 1235, 1234, 1222, 1224, 52, 2341}
};

const size_t STRING_RADIX = 256;
int string_digit_at(size_t index, const string& str) {
  if (index < str.length()) {
    return static_cast<unsigned char>(str[index]);
  }
  return -1;
}

const size_t UINT32_RADIX = 256;
int uint32_digit_at(int index, uint32_t val) {
  UASSERT(index >= 0) << "index " << index;
  if (index >= 4) return -1;
  unsigned char byte = (val >> 8 * (4 - index - 1)) & 0xFF;
  return byte;
}

void bench_uints() {
  start("1M uint32");

  vector<uint32_t> backup(1000 * 1000, 0);
  for (auto& i : backup) i = std::rand();
  auto uints = backup;

  // warmup before

  TIME_BLOCK(std::chrono::milliseconds, "") {
    msd_in_place_radix<UINT32_RADIX>(
        uints.begin(), uints.end(), uint32_digit_at);
  }

  for (std::size_t i = 0; i < uints.size(); ++i) uints[i] = backup[i];

  start("  std::sort for comparison");
  TIME_BLOCK(std::chrono::milliseconds, "") {
    std::sort(uints.begin(), uints.end());
  }
}

void bench_strings() {
  start("1M rand-len (max 20) string");

  const int max_size = 20;

  vector<std::string> backup(1000 * 1000, "");
  for (auto& i : backup) {
    // true modulo, works for negatives.
    auto len = ((std::rand() % max_size) + max_size) % max_size;
    for (int j = 0; j < len; ++j) {
      i += static_cast<char>(std::rand());
    }
  }
  auto strs = backup;

  TIME_BLOCK(std::chrono::milliseconds, "") {
    msd_in_place_radix<STRING_RADIX>(strs.begin(), strs.end(), string_digit_at);
  }

  for (std::size_t i = 0; i < strs.size(); ++i) strs[i] = backup[i];
  start("  std::sort for comparison");
  TIME_BLOCK(std::chrono::milliseconds, "") {
    std::sort(strs.begin(), strs.end());
  }
}

} // anonymous namespace

int main(int argc, char* argv[]) {

  if (argc == 2 && string(argv[1]) == "bench") {
    cout << endl;
    cout << "Benching MSD in-place radix sort" << endl;
    auto time =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
    cout << "    Using seed: " << time << endl;
    std::srand(time);
    bench_uints();
    bench_strings();
    return 0;
  }

  cout << endl;
  cout << "Testing MSD in-place radix sort" << endl;


  start("String sort");
  test_sort(string_examples, bind(
      msd_in_place_radix<STRING_RADIX, vector<string>::iterator,
      decltype(string_digit_at)>, placeholders::_1, placeholders::_2,
      string_digit_at));
  complete();

  start("Uint32 sort");
  test_sort(uint32_examples, bind(
      msd_in_place_radix<UINT32_RADIX, vector<uint32_t>::iterator,
      decltype(uint32_digit_at)>, placeholders::_1, placeholders::_2,
      uint32_digit_at));
  complete();

  start("");
  complete("...........Success!");
  cout << endl;

  return 0;
}
