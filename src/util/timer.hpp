/*
  Vladimir Feinberg
  util/timer.h
  2014-09-08

  Timing macro.
*/

#ifndef UTIL_TIMER_HPP_
#define UTIL_TIMER_HPP_

#include <chrono>
#include <iostream>

// Parameters:
// unit - std::chrono unit type
// msg - message to print
//
// Usage:
// Times the block that directly follows the macro, which is run exactly
// once. Prints 'msg' followed by the count of 'unit' time units that
// have elapsed, appended with a unit name and a std::endl, with no
// spaces between 'msg' and the count.
//
// Example:
// int x;
// TIME_BLOCK(std::chrono::milliseconds, "Time to add 1 + 1: ") {
//   x = 1 + 1;
// }
// The above would print "Time to add 1 + 1: 0ms" followed by a newline.
#define TIME_BLOCK(unit, msg)                                               \
  for (auto start = std::chrono::high_resolution_clock::now(), end = start; \
       start == end ||                                                      \
           util::_timer_internal::print_ret_false<unit>(start, end, msg);   \
       end = std::chrono::high_resolution_clock::now())

namespace util {
namespace _timer_internal {

template<class chrono_unit>
constexpr const char* unit_name();

template<>
constexpr const char* unit_name<std::chrono::nanoseconds>()  { return "ns";  }
template<>
constexpr const char* unit_name<std::chrono::microseconds>() { return "us";  }
template<>
constexpr const char* unit_name<std::chrono::milliseconds>() { return "ms";  }
template<>
constexpr const char* unit_name<std::chrono::seconds>()      { return "s";   }
template<>
constexpr const char* unit_name<std::chrono::minutes>()      { return "min"; }
template<>
constexpr const char* unit_name<std::chrono::hours>()        { return "hrs"; }

template<class chrono_unit, class time_point>
bool print_ret_false(const time_point& start, const time_point& end,
                     const std::string& msg) {
  auto dur = std::chrono::duration_cast<chrono_unit>(end - start);
  std::cout << msg << dur.count() << unit_name<chrono_unit>() << std::endl;
  return false;
}

} // namespace _timer_internal
} // namespace util

#endif /* UTIL_TIMER_HPP_ */
