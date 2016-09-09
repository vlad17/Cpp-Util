/*
  Vladimir Feinberg
  util/line_wrap.hpp
  2016-09-08

  Useful for testing, provides automatic indentation for tests.
*/

#ifndef UTIL_LINE_WRAP_HPP_
#define UTIL_LINE_WRAP_HPP_

#include <iostream>
#include <iomanip>

namespace util {

void start(const std::string& s, std::ostream& outs = std::cout) {
  std::size_t start = 0;
  while (start + 40 < s.length()) {
    auto sz = std::min<std::size_t>(s.length() - start, 40);
    outs << "    " << s.substr(start, sz) << std::endl;
    start += sz;
  }
  outs << "    "  << std::left << std::setw(40) << s.substr(start);
  outs.flush();
}

void complete(const std::string& s = "...complete",
              std::ostream& outs = std::cout) {
  outs << std::right << s << std::endl;
}

} // namespace util

#endif /* UTIL_LINE_WRAP_HPP_ */
