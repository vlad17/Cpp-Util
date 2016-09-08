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
  int start = 0;
  if (s.length() > 40) {
    outs << "    " << s.substr(start, 40) << std::endl;
    start += 40;
  }
  outs << "    " << std::left << std::setw(40);
  outs << s;
  outs.flush();
}

void complete(const std::string& s = "...complete",
              std::ostream& outs = std::cout) {
  outs << std::right << s << std::endl;
}

} // namespace util

#endif /* UTIL_LINE_WRAP_HPP_ */
