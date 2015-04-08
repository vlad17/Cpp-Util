/*
  Vladimir Feinberg
  util/sort-test.cpp
  2015-04-07

  Tests various sorting algorithms.
*/

#include <algorithm>
#include <iostream>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "util/radix.hpp"
#include "util/uassert.hpp"
#include "util/util.hpp"

using namespace std;
using namespace util;

template<class S>
void test_string_sort(S s) {
  vector<vector<string> > examples = {
    {}, {""}, {"", ""}, {"a", "a"}, {"a", "", "a"},
    {"ccc", "aaa", "bab", "bbc", "ccc", "ccc", "cdb", "cbd"},
    {"aa1", "bb2", "cc3"}, {"1", "22", "333"},
    {"hello", "!", "my name", "is", "what?", "my", "name", "is", "who?",
     "my", "name is", "..."}
  };

  for (const auto& i : examples) {
    auto user = i;
    auto sys = i;
    s(user.begin(), user.end());
    std::sort(sys.begin(), sys.end());
    bool equal = std::equal(user.begin(), user.end(), sys.begin());
    UASSERT(equal) << "User sort (1) <> Sys sort (2)\n"
                   << "\t(1) " << container_print(user) << "\n"
                   << "\t(2) " << container_print(sys) << "\n";
  }
}

int main(int, char**) {
  // String digit extractor
  auto string_digit_at = [](size_t index, const string& str) -> int {
    if (index < str.length()) return static_cast<unsigned>(str[index]);
    return -1;
  };

  cout << endl;
  cout << "Testing MSD in-place radix sort" << endl;
  cout << endl;

  auto my_sort = bind(msd_in_place_radix<256, vector<string>::iterator,
                      decltype(string_digit_at)>, placeholders::_1,
                      placeholders::_2, string_digit_at);
  test_string_sort(my_sort);

  cout << endl;
  cout << "\t...Complete!" << endl;
  cout << endl;

  return 0;
}
