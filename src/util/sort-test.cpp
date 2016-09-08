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
  if (index < str.length()) return static_cast<unsigned>(str[index]);
  return -1;
}

const size_t UINT32_RADIX = 256;
int uint32_digit_at(int index, uint32_t val) {
  if (index > 4) return -1;
  unsigned char byte = val >> 8 * (4 - index - 1);
  return byte;
}

void bench_uints() {
  start("Benching 10M uint32 sort");

  TIME_BLOCK(chrono::milliseconds, "") {
  }
}

void bench_strings() {
  start("Benching 1M rand-len string sort");

  TIME_BLOCK(chrono::milliseconds, "") {
  }
}

} // anonymous namespace

int main(int argc, char* argv[]) {

  if (argc == 2 && string(argv[1]) == "bench") {
    cout << endl;
    cout << "Benching MSD in-place radix sort" << endl;
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
